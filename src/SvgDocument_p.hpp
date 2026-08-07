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

class DocumentImpl
{
public:
    DocumentImpl() { contents.reserve(1'024); }
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
    SvgElement svgDocument;
    std::list<SvgElement *> parents;
    std::list<SvgElement> children;

    bool save();
    bool read();

    void addFromParser(const std::string &key, const std::string &value, const XmlNode::Type type);
    SvgElement *createElement(const std::string &name);

    //	Implementation functions-not visible to user
    //void init();

    //bool writeAll( const std::string& fileName, bool aReadOnly);

    /*
	//	Required fields
	static const std::string file;

	//	Class to capture all data in file.
	//	Track fields with special processing below.
	XmlElement elements;

	bool readOnly = false;
	bool exists = false;
	std::string name{ "Book1.xlsx" };
	std::string password;

	//	List of files in existing Excel file.
	//	No processing is performed on these items.
	//	Cache for later saving
	std::list<XlTempFile> tempFiles;

	//	Shared string table 376-1, 18.4
	XlSharedStrings sharedStr;

	XlWorksheets sheets; // 18.2.19

	//	definedName - 18.2.5
	//	definedNames - 18.2.6
	XlDefinedNames		names;

	//	Doc properties - see P2-8.1 General
	XlPropertiesCore core;
	XlPropertiesApp app;

	//  Fascade class for core and app;
	//  Limit user access to functions visible in
	//  this file.
	XlProperties        props;

	XlContentTypes contentTypes;
	std::optional<XlStyleSheet>	style;
	std::optional<XlTheme>		theme;*/
};
