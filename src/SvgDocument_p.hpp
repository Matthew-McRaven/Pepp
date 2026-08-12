#pragma once

//  W A R N I N G
//  -------------
//
// SvgDom uses PIMPL pattern to manage data access. Data and functions
// in this header are not part of the public interface and may
// change without notice.

//	Standard Library

#include <list>
//#include <optional>
#include <string>

//	private classes
#include "SvgElement.hpp"
#include "SvgParser.hpp"
#include "SvgSvgElement.hpp"
//#include "XmlElement_p.hpp"

class DocumentImpl
{
public:
    DocumentImpl() { contents.resize(1'024); }
    ~DocumentImpl() = default;

    DocumentImpl(const DocumentImpl &) = default;
    DocumentImpl &operator=(const DocumentImpl &) = default;
    DocumentImpl(DocumentImpl &&) noexcept = default;
    DocumentImpl &operator=(DocumentImpl &&) noexcept = default;

    bool readOnly = false;
    bool exists = false;
    std::string fileName{};
    std::string contents{};
    size_t fileSize{};
    SvgSvgElement svgDocument;
    std::list<SvgElement *> parents;
    std::list<SvgElement> children;
    std::list<std::string> rope; //  Temporary for persisting structure to Xml

    //XmlElement elements;

    bool save();
    bool read();

    void addFromParser(const std::string &key, const std::string &value, const XmlNode::Type type);
    SvgElement *createElement(const std::string &name);
    void toXml();
};
