#include "SvgDocument.hpp"
#include "SvgDocument_p.hpp"

// SvgDom uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>
namespace fs = std::filesystem;

//	private classes
#include "Timer.h"

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
    : _impl(std::make_unique<DocumentImpl>())
{}

Document::Document(const std::string &name)
    : Document()
{
    open(name);
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
Document::~Document() = default;
Document::Document(Document &&) noexcept = default;
Document &Document::operator=(Document &&) noexcept = default;

//  Accessors

void Document::saveAs(const std::string &fileName)
{
    _impl->fileName = fileName;

    Timer<> t1;
    t1.start();
    bool success = _impl->save();
    t1.finish();
    std::cout << "ofstream::write: " << t1.elapsedTime() << (success ? " Pass" : " Fail")
              << std::endl;
}

bool DocumentImpl::save()
{
    //	Try and open sourcefile
    std::ofstream svgFile(fileName, std::ios::out | std::ios::binary);
    if (!svgFile.is_open())
        return false;

    //  Rebuild object tree into xml
    toXml();

    svgFile.write(contents.data(), contents.size());
    svgFile.close();

    return true;
}

bool Document::open(const std::string &fileName, bool readOnly)
{
    _impl->fileName = fileName;
    _impl->readOnly = readOnly;

    //  File open
    if (!fs::exists(_impl->fileName)) {
        //  File doesn't exist, needs to initialize
        _impl->exists = false;
        return false;
    }

    Timer<> t1;
    t1.start();
    if (!_impl->read()) {
        std::cout << "Cannot open file: " << _impl->fileName;
        return false;
    }
    t1.finish();
    std::cout << "ifstream::read: " << t1.elapsedTime() << std::endl;

    //  Parser will callback to this instance using method addFromParser().
    //std::unique_ptr<SvgParser<DocumentImpl>> parser(new SvgParser<DocumentImpl>(*_impl));
    _impl->elements.reset();
    std::unique_ptr<SvgParser<XmlElement>> parser(new SvgParser<XmlElement>(_impl->elements));

    t1.start();
    parser->parse(_impl->contents);
    t1.finish();
    std::cout << "parsing file: " << t1.elapsedTime() << std::endl;

    //  Archive has data
    return true;
}

bool DocumentImpl::read()
{
    //	Try and open sourcefile
    std::ifstream svgFile(fileName, std::ios::in | std::ios::binary);
    fileSize = static_cast<size_t>(std::filesystem::file_size(fileName));

    //  Size to current file
    if (fileSize > contents.size())
        contents.resize(fileSize);

    exists = true;

    //  Copy file contents to string
    svgFile.read(&contents[0], fileSize);

    return true;
}

SvgElement *Document::createElement(const std::string &name)
{
    return _impl->createElement(name);
}
SvgElement *DocumentImpl::createElement(const std::string &name)
{
    return &children.emplace_back(name);
}

//	When parsing, we want parser to return pointer to data
//  structure for these items.
//	This function is a callback from the xml parser
void DocumentImpl::addFromParser(const std::string &key,
                                 const std::string &value,
                                 const XmlNode::Type type)
{
    static SvgElement *element{};

    switch (type) {
    case XmlNode::Type::RootElement:
        //  Root element is already created since it is required.
        svgDocument.setId(key);
        svgDocument.setValue(value);
        parents.push_back(&svgDocument);
        break;
    case XmlNode::Type::RootAttribute:
        //std::cout << "Root Attribute:" << key << " value: " << value << std::endl;
        break;
    case XmlNode::Type::Element:
        element = createElement(key);
        parents.back()->appendChild(element);
        parents.push_back(element);
        //std::cout << "Start Element:" << key << " value: " << value << std::endl;
        break;
    case XmlNode::Type::Attribute:
        //std::cout << "Attribute:" << key << " value: " << value << std::endl;
        break;
    case XmlNode::Type::EndElement:
        parents.pop_back();
        //std::cout << "End Element:" << key << std::endl;
        break;
    }
}

//  Loop throug all elements and get a rope of values.
//  flatten values into a single string that is later
//  persisted.
void DocumentImpl::toXml()
{
    //  Clear previous result
    contents.clear();

    const auto result = elements.write();
    contents = result;
    //  Create in memory rope of Xml structure
    //svgDocument.toXml(rope);

    //  Clear previous result
    /*contents.clear();

    //  Create single string in memory
    for (auto &fragment : rope) {
        contents.append(fragment);
    }

    //  Free up memory
    rope.clear();*/
}
