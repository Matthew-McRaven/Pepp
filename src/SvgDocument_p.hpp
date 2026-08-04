#pragma once

//  W A R N I N G
//  -------------
//
// SvgDom uses PIMPL pattern to manage data access. Data and functions
// in this header are not part of the public interface and may
// change without notice.

//	Standard Library

//#include <list>
//#include <optional>
#include <string>

//	private classes
#include "SvgParser.hpp"

class DocumentImpl
{
public:
    DocumentImpl() {}

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

    bool read();

    void addFromParser(const std::string &key, const std::string &value, const XmlNode::Type type);

    //	Implementation functions-not visible to user
    //void init();

    //bool writeAll( const std::string& fileName, bool aReadOnly);

    /*void readAll( ZipIt::Archive& archive );
	void read( ZipIt::Archive& xlFileContents );
	void add( const std::string& key, const std::string& value,
		const XmlNode::Type type );

	XlWorksheet& find( const std::string& name );
	XlWorksheet& find( const size_t index );

	//	Required fields
	static const std::string file;

	//	Class to capture all data in file.
	//	Track fields with special processing below.
	XmlElement elements;

	bool readOnly = false;
	bool exists = false;
	std::string name{ "Book1.xlsx" };
	std::string password;

	//	Add relation file for root element
	XlRelation<XlApp> root{ std::string( "_rels/.rels" ) };

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

    //	Some elements are not available for processing. Store
    //	elements for later saving

    //	bookView structure - 18.2.1 - in blank
	//	calcPr - 18.17.6.1 - in blank
	//	calcProperties - 18.2.2 - in blank
	//	customWorkbookView - 18.2.3
	//	customWorkbookViews - 18.2.4
	//	ext(ension) - 18.2.7 - in blank
	//	externalReference - 18.2.8 
	//	externalReferences - 18.2.9
	//	extLst - 18.2.10 - in blank
	//	fileRecoveryPr - 18.2.11
	//	fileVersion - 18.2.13 - in blank
	//	revisionPtr(operties) - Find site - in blank
	//	Skip 18.2.14 through 16, 21-26
	//	pivotCache(PivotCache) - 18.2.17 
	//	pivotCaches (PivotCaches) - 18.2.18 
	//	workbookPr(operties) - 18.2.28 - in blank
	//	workbookProtection - 18.2.29
	//	workbookView - 18.2.30 - in blank
    //std::vector<XmlElement> notUsed;
};
