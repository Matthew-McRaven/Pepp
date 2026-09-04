#include "SvgDocument.hpp"

//	Standard library
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
namespace fs = std::filesystem;
using namespace std::string_literals;

//	private classes
#include "SvgInterface.hpp"
#include "SvgRectElement.hpp"
#include "Timer.h"
#include "XmlAttributes_p.hpp"
/*
From Mozilla: https://developer.mozilla.org/en-US/docs/Web/API/Document
    
The Document interface represents any web page loaded in the browser and serves
as an entry point into the web page's content, which is the DOM tree. SvgDom
uses elements specifically used by SVG

The SVG DOM tree includes elements such as <g> and <circle>, among many others.
It provides functionality globally to the document, like how to parse/stream
svg XML and create new elements in the document.

The Document interface describes the common properties and methods for any
kind of document. Depending on the document's type (e.g., HTML, XML, SVG, …),
a larger SVG API is available: https://www.w3.org/TR/SVG2/Overview.html. XML
and SVG documents implement the XMLDocument interface using MIME type of
"image/svg+xml".
*/

//	Public interface
Document::Document()
{
    _svgDocument = std::make_unique<SvgSvgElement>();
}

Document::Document(const std::string &name)
    : Document()
{
    open(name);
}

//  Accessors
SvgSvgElement &Document::documentElement()
{
    return *_svgDocument.get();
}
const SvgSvgElement &Document::documentElement() const
{
    return *_svgDocument.get();
}

//  File operations
void Document::saveAs(const std::string &fileName)
{
    _fileName = fileName;
    bool success = false;

    Timer t1;
    t1.start();
    success = save();
    t1.finish();
    std::cout << "ofstream::write (flattened): " << t1.elapsedTime()
              << (success ? " Pass" : " Fail") << std::endl;
}

bool Document::save() const
{
    //	Try and open sourcefile
    std::ofstream svgFile(_fileName, std::ios::out | std::ios::binary);
    if (!svgFile.is_open())
        return false;

    //  Rebuild object tree into xml
    const auto contents = std::move(toXml());

    svgFile.write(contents.data(), contents.size());
    svgFile.close();

    return true;
}

bool Document::fromXml(const std::string &svgData)
{
    _streamInput = svgData;
    return parse();
}

bool Document::open(const std::string &fileName, bool readOnly)
{
    _fileName = fileName;
    _readOnly = readOnly;

    //  File open
    if (!fs::exists(_fileName)) {
        //  File doesn't exist, needs to initialize
        _exists = false;
        return false;
    }

    Timer t1;
    t1.start();
    if (!read()) {
        std::cout << "Cannot open file: " << _fileName;
        return false;
    }
    t1.finish();
    std::cout << "ifstream::read: " << t1.elapsedTime() << std::endl;

    return parse();
}

bool Document::parse()
{
    //  Create on heap to avoid stack warnings from compiler
    std::unique_ptr<SvgParser<Document>> parser(new SvgParser<Document>(*this));
    Timer t;
    t.start();
    try {
        parser->parse(_streamInput);
    } catch (...) {
        std::cout << "Error parsing file." << std::endl;
        return false;
    }
    t.finish();
    std::cout << "parsing file: " << t.elapsedTime() << std::endl;
    return true;
}

bool Document::read()
{
    //	Try and open sourcefile
    std::ifstream svgFile(_fileName, std::ios::in | std::ios::binary);
    _fileSize = static_cast<size_t>(std::filesystem::file_size(_fileName));

    //  Size to current file
    if (_fileSize > _streamInput.size())
        _streamInput.resize(_fileSize);

    _exists = true;

    //  Copy file contents to string
    svgFile.read(&_streamInput[0], _fileSize);

    return true;
}

//	When parsing, we want parser to return pointer to data
//  structure for these items.
//	This function is a callback from the xml parser
void Document::addFromParser(const std::string &key,
                             const std::string &value,
                             const XmlNode::Type type)
{
    switch (type) {
    case XmlNode::Type::RootElement:
        //  Root element is already created since it is required.
        _svgDocument->setXmlName(key);
        _svgDocument->setValue(value);
        _svgDocument->setDocument(this);
        _parents.push_back(_svgDocument.get());
        break;
    case XmlNode::Type::RootAttribute:
    case XmlNode::Type::Attribute: //  No current differences in attributes
        _parents.back()->setAttribute(key, value);
        break;
    case XmlNode::Type::Element: {
        auto element = _parents.back()->createElement(key);
        element->setValue(value);
        element->setDocument(this);
        _parents.push_back(element);
        break;
    }
    case XmlNode::Type::Comment: {
        //  Comments have blank key. Comment is in value field
        //  If comment is outside svg element, skip it. Otherwise, this throws
        if (!_parents.empty()) {
            auto element = _parents.back()->createElement("comment"s);
            element->setDocument(this);
            element->setValue(value);
        }
        //  A comment can never be a parent. End element is not called
        //  Do not store value on parent stack.
        break;
    }
    case XmlNode::Type::CData: {
        //  CData has blank key. CData is in value field
        //  If comment is outside svg element, skip it. Otherwise, this throws
        if (!_parents.empty()) {
            auto element = _parents.back()->createElement("cdata"s);
            element->setDocument(this);
            element->setValue(value);
        }
        //  A CData can never be a parent. End element is not called
        //  Do not store value on parent stack.
        break;
    }
    case XmlNode::Type::EndElement:
        _parents.pop_back();
        break;
    }
}

void Document::addElementId(const std::string &id, SvgInterface *element)
{
    _idLookup.emplace(id, element);
}
SvgInterface *Document::getElementById(const std::string &id)
{
    if (auto search = _idLookup.find(id); search != _idLookup.end())
        return search->second;

    //  Not in container
    return nullptr;
}

const std::string Document::toXml() const
{
    SvgRope rope;
    return std::move(flattenRope(rope));
}

//  Loop through all elements and get a rope of values.
//  flatten values into a single string that is later
//  persisted.
std::string Document::flattenRope(SvgRope &rope) const
{
    std::string xml;

    //  Create in memory rope of Xml structure
    _svgDocument->toXml(rope);

    //  Resize string if Xml is longer than current string length
    if (xml.capacity() < rope.size()) {
        xml.resize(rope.size() + 1);
        xml.clear();
    }

    //  Create single string in memory
    for (auto &fragment : rope.rope()) {
        xml.append(fragment);
    }

    return std::move(xml);
}

void Document::copyDocument(const Document &newDoc)
{
    //  Don't allow copies of self to self
    if (&newDoc == this)
        return;
    SvgInterface::setCurrentDocument(this);
    _svgDocument = newDoc.documentElement().clone();
    SvgInterface::setCurrentDocument();
}
