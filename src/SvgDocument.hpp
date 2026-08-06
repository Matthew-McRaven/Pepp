#pragma once

#include <cassert>
#include <memory>
#include <string>

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
    //SvgElement querySelector(const std::string& element);
    //SvgElement getElementById(const std::string& element);
    //List children();

    //	User access functions

    //	Document properties.
    //	accessors
    //const std::string &name() const;
    //void SetName( const std::string& value );	//	VBA does not allow file name change

    //	File options
    //bool fromSvg(const std::string &fileName);
    //const std::string& toSvg();

    bool open(const std::string &fileName, bool readOnly = false);
    bool exists(const std::string &fileName);
    void saveAs(const std::string &fileName);
    /*void save();
    void close();*/
};
