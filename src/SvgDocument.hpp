#pragma once

#include <cassert>
#include <memory>
#include <string>

//  Library classes
#include "SvgElement.hpp"
#include "SvgRectElement.hpp"
#include "SvgSvgElement.hpp"

//	Forward declarations
class DocumentImpl;

class Document
{
    std::unique_ptr<DocumentImpl> _impl;

public:
    Document();
    explicit Document(const std::string &name);
    ~Document();
    //Cannot copy, but can move
    Document(const Document &) = delete;
    Document &operator=(const Document &) = delete;
    Document(Document &&) noexcept;
    Document &operator=(Document &&) noexcept;

    //  Document searching
    SvgSvgElement &documentElement() const;
    SvgElement *createElement(const std::string &name = "");

    //SvgElement querySelector(const std::string& element);
    //SvgElement getElementById(const std::string& element);
    //List children();

    //	User access functions

    //	Document properties.
    //	accessors

    //	File options
    bool fromXml(const std::string &svgData);
    const std::string &toXml();

    bool open(const std::string &fileName, bool readOnly = false);
    bool exists(const std::string &fileName);
    void saveAs(const std::string &fileName);
    /*void save();
    void close();*/
};
