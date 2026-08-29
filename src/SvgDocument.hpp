#pragma once

#include <cassert>
#include <string>

//  Library classes
#include "SvgElement.hpp"
#include "SvgParser.hpp"
#include "SvgRectElement.hpp"
#include "SvgSvgElement.hpp"

//	Forward declarations
class DocumentImpl;
class SvgInterface;

class Document
{
    bool _readOnly = false;
    bool _exists = false;
    std::string _fileName{};
    SvgSvgElement _svgDocument;
    std::list<SvgElement *> _parents;

    //  Temporaries from reading xml file
    std::string _streamInput{};
    size_t _fileSize{};

    bool read();
    bool save() const;
    std::string flattenRope(SvgRope &rope) const;
    bool parse();

public:
    Document() = default;
    explicit Document(const std::string &name);
    ~Document() = default;
    //Cannot copy, but can move
    Document(const Document &) = delete;
    Document &operator=(const Document &) = delete;
    Document(Document &&) noexcept = default;
    Document &operator=(Document &&) noexcept = default;

    //  Document searching
    SvgSvgElement &documentElement();
    const SvgSvgElement &documentElement() const;

    //SvgElement querySelector(const std::string& element);
    //SvgElement getElementById(const std::string& element);
    //List children();

    //	User access functions

    //	Document properties.
    //	accessors

    //	File options
    bool fromXml(const std::string &svgData);
    const std::string toXml() const;

    void addFromParser(const std::string &key, const std::string &value, const XmlNode::Type type);

    bool open(const std::string &fileName, bool readOnly = false);
    bool exists(const std::string &fileName);
    void saveAs(const std::string &fileName);
    /*void save();
    void close();*/
};
