#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

//  Library classes
#include "SvgElement.hpp"
#include "SvgParser.hpp"
#include "SvgSvgElement.hpp"

//	Forward declarations

class Document
{
    bool _readOnly = false;
    bool _exists = false;
    std::string _fileName{};
    //SvgSvgElement _svgDocument;
    std::unique_ptr<SvgSvgElement> _svgDocument;

    //  Temporaries from reading xml file
    std::list<SvgInterface *> _parents; //  Temporary list for parsing
    std::string _streamInput{};
    size_t _fileSize{};

    //  For lookup by id
    std::unordered_map<std::string, SvgInterface *> _idLookup;

    bool read();
    bool save() const;
    std::string flattenRope(SvgRope &rope) const;
    bool parse();

public:
    Document();
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
    void copyDocument(const Document &newDoc);

    //SvgElement querySelector(const std::string& element);
    SvgInterface *getElementById(const std::string &id);
    void addElementId(const std::string &id, SvgInterface *element);

    //	File options
    bool fromXml(const std::string &svgData);
    const std::string toXml() const;

    //  Must be public, this is callback from parser template class
    void addFromParser(const std::string &key, const std::string &value, const XmlNode::Type type);

    bool open(const std::string &fileName, bool readOnly = false);
    bool exists(const std::string &fileName);
    void saveAs(const std::string &fileName);
};
