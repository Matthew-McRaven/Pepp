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

class SvgDocument {
  bool _readOnly = false;
  bool _exists = false;
  std::string _fileName{};
  std::unique_ptr<SvgSvgElement> _svgDocument;

  //  Temporaries for reading and parsing xml file
  std::list<SvgInterface *> _parents; //  Temporary list for parsing
  std::string _streamInput{};
  mutable size_t _fileSize{};

  //  For lookup by id
  std::unordered_map<std::string, SvgInterface *> _idLookup;

  bool read();
  bool save() const;
  std::string flattenRope(SvgRope &rope) const;
  bool parse();

public:
  SvgDocument();
  explicit SvgDocument(const std::string &name);
  ~SvgDocument() = default;
  // Cannot copy, but can move
  SvgDocument(const SvgDocument &) = delete;
  SvgDocument &operator=(const SvgDocument &) = delete;
  SvgDocument(SvgDocument &&) noexcept = default;
  SvgDocument &operator=(SvgDocument &&) noexcept = default;

  //  Document searching
  SvgSvgElement &documentElement();
  const SvgSvgElement &documentElement() const;
  void copyDocument(const SvgDocument &newDoc);

  // SvgElement querySelector(const std::string& element);
  SvgInterface *getElementById(const std::string &id);
  void addElementId(const std::string &id, SvgInterface *element);

  //	File options
  bool fromXml(const std::string &svgData);
  const std::string toXml() const;

  //  Must be public, this is callback from parser template class
  void addFromParser(const std::string &key, const std::string &value, const XmlNode::Type type);

  bool open(const std::string &fileName, bool readOnly = false);
  bool exists() const noexcept;
  bool saveAs(const std::string &fileName);

  size_t fileSize() const;
  const std::string &fileName() const;
};
