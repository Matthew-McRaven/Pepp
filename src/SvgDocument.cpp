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
    : _impl(std::make_unique<DocumentImpl>())
{
    //  Do not initialize sheets with blank sheets
    //Open( name );
}

//	Need to move implementation after Impl structure so unique_ptr will see full
//	definition. Otherwise, compiler error
Document::~Document() = default;
Document::Document(Document &&) noexcept = default;
Document &Document::operator=(Document &&) noexcept = default;

//  Accessors
/*const std::string& XlWorkbook::Name() const
{    return impl_->name; }

XlProperties& XlWorkbook::Properties() 
{    return impl_->props; }
const XlProperties& XlWorkbook::Properties() const
{    return impl_->props; }

//  Return single worksheet
XlWorksheet& XlWorkbook::Sheets( const std::string& name )
{   return impl_->find( name ); }
XlWorksheet& XlWorkbook::Sheets( const size_t index )
{   return impl_->find( index ); }

XlWorksheet& XlWorkbookImpl::find( const std::string& name )
{   return sheets.find( name ); }
XlWorksheet& XlWorkbookImpl::find( const size_t index )
{   return sheets.find( index ); }

//  Return list of worksheets
XlWorksheets& XlWorkbook::Sheets()
{   return impl_->sheets; }
const XlWorksheets& XlWorkbook::Sheets() const
{   return impl_->sheets; }

XlWorksheets& XlWorkbook::Worksheets()
{   return impl_->sheets; }
const XlWorksheets& XlWorkbook::Worksheets() const
{   return impl_->sheets; }

//  Return list of defined range names
XlDefinedNames& XlWorkbook::Names()
{   return impl_->names; }
const XlDefinedNames& XlWorkbook::Names() const
{   return impl_->names; }

*/

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

    svgFile.write(contents.data(), contents.size());
    svgFile.close();

    return true;
}

bool Document::open(const std::string &fileName, bool readOnly)
{
    _impl->fileName = fileName;
    _impl->readOnly = readOnly;

    //  File open
    if (!fs::exists(fileName)) {
        //  File doesn't exist, needs to initialize
        _impl->exists = false;
        return false;
    }

    Timer<> t1;
    t1.start();
    if (!_impl->read()) {
        std::cout << "Cannot open file: " << fileName;
        return false;
    }
    t1.finish();
    std::cout << "ifstream::read: " << t1.elapsedTime() << std::endl;

    std::unique_ptr<SvgParser<DocumentImpl>> parser(new SvgParser<DocumentImpl>(*_impl));
    //parser->parse(_impl->contents);

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

    //std::cout << "File size: " << fileSize << std::endl;

    exists = true;

    //  Copy file contents to string
    svgFile.read(&contents[0], fileSize);

    return true;
}

//	When parsing, we want parser to return pointer to data
//  structure for these items.
//	Callback on xml parser
void DocumentImpl::addFromParser(const std::string &key,
                                 const std::string &value,
                                 const XmlNode::Type type)
{
    static std::string element;
    //static XlDefinedName *name = nullptr;
    static uint32_t xmlType = 0;

    switch (type) {
    case XmlNode::Type::RootElement:
        assert(key == "svg");
        break;
    case XmlNode::Type::RootAttribute:
        break;
    case XmlNode::Type::Element:
        break;
    case XmlNode::Type::Attribute:
        break;
    case XmlNode::Type::EndElement:
        break;
    }
    //  Elements to skip
    /*if (key == "definedNames" || key == "sheets") {
            xmlType = 1;
        } else if (key == "definedName") {
            //  //DefinedNames uses range name for key value
            element = value;
            xmlType = 2;
        } else if (key == "sheet") {
            element = value;
            sheets.Add();
            xmlType = 3;
        } else {
            //  Make sure we aren't processing child of parent element
            if (xmlType == 0) {
                //  Flag for terminating processing
                element = key;
                xmlType = 99;

                //  Initialize parent element
                notUsed.emplace_back(key, value);
            } else {
                //  Add children nodes
                notUsed.back().add(key, value, type);
            }
        }
        break;
    case XmlNode::Type::Attribute:

        //  Loop through nodes with special processing
        switch (xmlType) {
        case 2:
            if (name == nullptr)
                name = &names.Add(value, element);
            else
                name->Add(key, value);
            break;
        case 3:

            sheets.list().back().add(key, value);

            break;
        case 99:
            notUsed.back().add(key, value, type);
            break;
        }
        break;
    case XmlNode::Type::EndElement:
        if (xmlType == 99) {
            //  Unwind XmlElement stack
            notUsed.back().add(key, value, type);

            //  Only clear if this is parent element
            if (element == key) {
                xmlType = 0;
                element.clear();
            }
        } else {
            name = nullptr;
            xmlType = 0;
            element.clear();
        }
        break;
    }*/
}
/*
//	Public interface
void XlWorkbook::Close()
{
    //  Does not save, just resets to blank sheet
    impl_.reset( new XlWorkbookImpl() );
}

//	Persistence functions
void XlWorkbookImpl::readAll( ZipIt::Archive& archive )
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

//	When parsing, we want parser to return pointer to data
//  structure for these items.
//	Callback on xml parser
void XlWorkbookImpl::add( const std::string& key, const std::string& value,
    const XmlNode::Type type )
{
    static std::string element;
    static XlDefinedName* name = nullptr;
    static uint32_t xmlType = 0;

    switch( type )
    {
    case XmlNode::Type::RootElement:
        assert( key == "workbook" );
        break;
    case XmlNode::Type::RootAttribute:
        break;
    case XmlNode::Type::Element:
        //  Elements to skip
        if( key == "definedNames" ||
            key == "sheets" )
        {
            xmlType = 1;
        }
        else if( key == "definedName" )
        {
            //  //DefinedNames uses range name for key value
            element = value;
            xmlType = 2;
        }
        else if( key == "sheet" )
        {
            element = value;
            sheets.Add();
            xmlType = 3;
        }
        else
        {
            //  Make sure we aren't processing child of parent element
            if( xmlType == 0 )
            {
                //  Flag for terminating processing
                element = key;
                xmlType = 99;

                //  Initialize parent element
                notUsed.emplace_back( key, value );
            }
            else
            {
                //  Add children nodes
                notUsed.back().add( key, value, type );
            }
        }
        break;
    case XmlNode::Type::Attribute:

        //  Loop through nodes with special processing
        switch( xmlType )
        {
        case 2:
            if( name == nullptr )
                name = &names.Add( value, element );
            else
                name->Add( key, value );
            break;
        case 3:

            sheets.list().back().add(key, value);

            break;
        case 99:
            notUsed.back().add( key, value, type );
            break;
        }
        break;
    case XmlNode::Type::EndElement:
        if( xmlType == 99  )
        {
            //  Unwind XmlElement stack
            notUsed.back().add( key, value, type );

            //  Only clear if this is parent element
            if( element == key )
            {
                xmlType = 0;
                element.clear();
            }
        }
        else
        {
            name = nullptr;
            xmlType = 0;
            element.clear();
        }
        break;
    }
}

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