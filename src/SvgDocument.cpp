#include "SvgDocument.hpp"
#include "SvgDocument_p.hpp"

// SvgDom uses PIMPL pattern to manage data access. Data and functions
// in Impl struct are not part of the public interface and may
// change without notice.

//	Standard library
//#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
//#include <optional>
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
    std::unique_ptr<SvgParser<DocumentImpl>> parser(new SvgParser<DocumentImpl>(*_impl));

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
    //  Create in memory rope of Xml structure
    svgDocument.toXml(rope);

    //  Clear previous result
    contents.clear();

    //  Create single string in memory
    for (auto &fragment : rope) {
        contents.append(fragment);
    }

    //  Free up memory
    rope.clear();
}

/*
//	Public interface
//	Persistence functions


bool XlWorkbookImpl::writeAll( const std::string& xlFileName, bool aReadOnly,
    const std::string& aPassword )
{
    ZipIt::Archive archive( xlFileName );
  
    //  Write root _rel/.rels file
    root.write( archive );

    //  Save shared strings before worksheets. Reindexing will change numbering
    //  if strings are deleted.
    if( sharedStr.size() > 0 )
    {
        sharedStr.reindex(); 
        sharedStr.write( archive.addFile( XlSharedStrings::file() ) );//  xl/sharedStrings.xml
    }
    
    //  Worksheet name is variable. Save first
    sheets.write( archive );         //  xl/worksheets/sheet#.xml

    //  Write results to Zip file
    write( archive.addFile( XlWorkbookImpl::file ) );                //  xl/workbook.xml (required)

    contentTypes.setSharedString( sharedStr.size() > 0 );
    //  Toggle xl/sharedStrings.xml file

    contentTypes.setTheme( theme.has_value() ); //  Toggle themes
    contentTypes.setStyle( style.has_value() ); //  Toggle style
    contentTypes.setSheetCnt( sheets.Count() ); //  Set number of sheets
    contentTypes.write( archive );  //  [Content_Types].xml (required) and _rel files

    core.write( archive.addFile( XlPropertiesCore::file() ) );  //  docProps/core.xml (required)
    app.write( archive.addFile( XlPropertiesApp::file() ) );    //  docProps/app.xml (required)

    if( style.has_value() )                 //  Styles are optional. Skip if missing
    {
        style->write( archive.addFile( XlStyleSheet::file() ) );     //  xl/styles.xml
    }
    if( theme.has_value() )                 //  Themes are optional. Skip if missing
    { 
        theme->write( archive.addFile( XlTheme::file() ) );     //  xl/theme/theme1.xml
    }

   //  This will only be executed if editing an existing file
    for( const auto& file : tempFiles )
    {
        file.write( archive.addFile( file.file() ) );
    }

    //  Save zip file
    archive.save();

    //  Close also saves. Separate logic for close and save.
    archive.close();
    return true;
}

//  Output xl/workbook.xml
void XlWorkbookImpl::write( ZipIt::ZOstream& zipstream ) const
{
    XmlStream strm( zipstream );

    strm.writeStartDocument( true );
    strm.writeStartElement( "workbook", "" );

    //  Load required headers
    strm.writeAttribute( "xmlns", "http://schemas.openxmlformats.org/spreadsheetml/2006/main" );
    strm.writeAttribute( "xmlns:r", "http://schemas.openxmlformats.org/officeDocument/2006/relationships" );
    strm.writeAttribute( "xmlns:mc", "http://schemas.openxmlformats.org/markup-compatibility/2006" );
    strm.writeAttribute( "mc:Ignorable", "x15 xr xr6 xr10 xr2" );
    strm.writeAttribute( "xmlns:x15", "http://schemas.microsoft.com/office/spreadsheetml/2010/11/main" );
    strm.writeAttribute( "xmlns:xr", "http://schemas.microsoft.com/office/spreadsheetml/2014/revision" );
    strm.writeAttribute( "xmlns:xr6", "http://schemas.microsoft.com/office/spreadsheetml/2016/revision6" );
    strm.writeAttribute( "xmlns:xr10", "http://schemas.microsoft.com/office/spreadsheetml/2016/revision10" );
    strm.writeAttribute( "xmlns:xr2", "http://schemas.microsoft.com/office/spreadsheetml/2015/revision2" );

    //  Save elements that were not used from
    //  opening an existing file
    for( const auto& ele : notUsed )
    {
        ele.write( strm );
    }

    //  Write sheet updates to workbook.xml
    sheets.writeSummary( strm );

    //  Output defined names
    names.write( strm );
    
    //	Close any open Xml tags
    strm.writeEndDocument();

    zipstream.close();
}
*/

/*
 * //	Persistence functions
void DocumentImpl::fromSvg( const std::string& svg )
{
    //  Read attributes from file
    for( const auto& file : archive.fileList() )
    {
        exists = true;
        if( contentTypes.read( archive, file.name() ) )
            //  If true, relations file processed archive
            continue;
        else if( XlWorkbookImpl::file == file.name() )
            read( archive );                            //  xl/workbook.xml 
        else if( XlPropertiesCore::file() == file.name() )
            //  required field
            core.read( archive );                       //  docProps/core.xml
        else if( XlPropertiesApp::file() == file.name() )
            //  required field
            app.read( archive );                        //  docProps/app.xml
        else if( XlSharedStrings::file() == file.name() )
            //  Shared strings are processed below. SKip file or
            //  it will be loaded twice
            continue;                  //  xl/sharedStrings.xml
        else if( XlStyleSheet::file() == file.name() )
        {
            style = XlStyleSheet();
            style->read( archive );                     //  xl/styles.xm
        }
        else if( XlTheme::file() == file.name() )
        {
            //  Optional files need to be constructed
            theme = XlTheme();
            theme->read( archive );                     //  xl/theme/theme.xml
        }
        else if( file.name().find("xl/worksheets/sheet") != std::string::npos )
        { 
            //  Shard strings need to be loaded before sheets are read
            if( !sharedStr.isLoaded() )
            {
                sharedStr.read( archive );              //  xl/sharedStrings.xml
            }

            sheets.read( archive, file.name() );      //  xl/_rels/.rels
        }
        else
        {
            //  Just archive files we find but don't care about
            auto& temp = tempFiles.emplace_back( file.name() );
            temp.read( archive );
        }
    }
}

void XlWorkbookImpl::read( ZipIt::Archive& archive )
{
    //	Get file details
    auto& file = archive.getFile( XlWorkbookImpl::file );

    if( file.uncompressedSize() == 0 ) return;

    //  Clear previous unused nodes
    notUsed.clear();
    
    //  Turn stream into string
    auto& stream = archive.getFileStream( file );
    std::string data( file.uncompressedSize() + 1, '\0' );

    stream.read( &data[0], file.uncompressedSize() );

    //	Parse will call back to add below for each element found
    //	Constructing class give stack warning, move to heap
    std::unique_ptr<XmlParser2<XlWorkbookImpl>>
        parser( new XmlParser2<XlWorkbookImpl>( *this ) );
    parser->parse( data );

    //  Read related .rels file
    root.read( archive );
}

 */