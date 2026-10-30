#pragma once

#include <cctype>
#include <expected>
#include <string>
#include <vector>

//	External xml parser
//	3rd Party
#include <rapidxml.hpp>
#include "utility_p.hpp" //  For Conversion::parseToNumber

namespace XmlNode {
enum class Type { Element = 0, RootElement, RootAttribute, Attribute, EndElement, Comment, CData };
}

//	Schema
template<typename Xml>
class SvgParser
{
    rapidxml::xml_document<> _doc; //  Move constructor disabled here
    std::vector<char> _buffer;
    std::string _key;
    std::string _value;
    Xml *_xml = nullptr;
    bool _root = true;

public:
    SvgParser() = default;
    SvgParser(Xml &result)
        : _xml(&result) {};
    ~SvgParser() = default;

    //	No copying
    SvgParser(const SvgParser &) = delete;
    SvgParser &operator=(const SvgParser &) = delete;
    //	Allow moving
    SvgParser(SvgParser &&) = default;
    SvgParser &operator=(SvgParser &&) = default;

    //	Parse result
    bool parse(const std::string &data) {
      //  If no characters, there is nothing to parse
      if (data.empty())
        return false;

      //	Clear prior XML and set to current string
      _doc.clear();

      _buffer.assign(data.begin(), data.end());
      _buffer.push_back('\0');

      //	Non destructive to original XML
      //  fastest skips comment, pi nodes, and declaration.
      //  Add comments to parsing
      //  Add CData by removing parse_no_data_nodes
      _doc.parse<rapidxml::parse_comment_nodes>(&_buffer[0]);

      //	Rapidxml does not return xml header element by default. Inkscape adds comment after
      //  xml header, and we must traverse all headers to get to svg root element
      for (auto child = _doc.first_node(); child; child = child->next_sibling()) walk(child.get());

      return true;
    }

private:
  void walk(const flxml::xml_node<> *node) {
    //	Keep local copy of element. This is a recursive function, and we need to
    //  know the current element when signalling the end of the element
    std::string key;

    const rapidxml::node_type type = node->type();
    switch (type) {
    case rapidxml::node_type::node_element:

      //	Convert from character array to string
      _key.clear();
      _value.clear();
      key = _key.append(node->name());

      //	Get value
      if (node->value().size() > 0) {
        _value.append(node->value());
      }

      //	Signal callback that this is an element
      _xml->addFromParser(_key, _value, _root ? XmlNode::Type::RootElement : XmlNode::Type::Element);

      //	Loop through attributes, if any. Call back on each name/value pair.
      for (auto attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
        //	Convert from character array to string
        _key.clear();
        _value.clear();
        _key.append(attr->name());
        _value.append(attr->value());

        //	This is a callback function!
        //  Forward attribute data
        _xml->addFromParser(_key, _value, _root ? XmlNode::Type::RootAttribute : XmlNode::Type::Attribute);
      }

      //	Signal that root element is complete
      _root = false;

      //	Keep traversing
      for (auto child = node->first_node(); child; child = child->next_sibling()) walk(child.get());

      //	This is a callback function!
      //	Signal end of element
      _xml->addFromParser(key, "", XmlNode::Type::EndElement);
      break;

    case rapidxml::node_type::node_comment:

      //	Convert from character array to string
      _key.clear();
      _value.clear();

      //  Key of comment is usually blank
      key = _key.append(node->name());

      //	Get Comment
      if (node->value().size() > 0) {
        _value.append(node->value());
      }

      //	Signal callback that this is an element
      _xml->addFromParser(_key, _value, XmlNode::Type::Comment);
      break;
    case rapidxml::node_type::node_cdata:

      //	Convert from character array to string
      _key.clear();
      _value.clear();

      //  Key of CData is usually blank
      key = _key.append(node->name());

      //	Get Cdata
      if (node->value().size() > 0) {
        _value.append(node->value());
      }

      //	Signal callback that this is an element
      _xml->addFromParser(_key, _value, XmlNode::Type::CData);
      break;
    default:
      //	Convert from character array to string
      _key.clear();
      key = _key.append(node->name());
      break;
    }
  }
};

class FunctionParser {
public:
  using Argument = std::pair<std::string, std::vector<double>>;
  using ParseResults = std::vector<Argument>;
  static auto parse(std::string_view &view) -> std::expected<ParseResults, bool> {
    //  No data, no parsing
    if (view.empty()) return std::unexpected(false);

    ParseResults results;

    //  Loop through all functions
    while (view.contains(')')) {
      auto result = parseElement(view);
      if (result.has_value()) {
        Argument &arg = *result;
        results.push_back(arg);
      }
    }

    if (results.empty()) return std::unexpected(false);
    return std::move(results);
  }

  //  Trim spaces on left
  static void trimLeft(std::string_view &view) {
    while (!view.empty() && std::isspace(view.front())) {
      view.remove_prefix(1);
    }
  }
  //  Trim to search character
  static void trimLeft(std::string_view &view, char search) {
    trimLeft(view); //  Remove whitespace
    while (!view.empty() && view.front() == search) {
      //  Remove search character
      view.remove_prefix(1);
    }
  }

  //  Trim spaces on right
  static void trimRight(std::string_view &view) {
    while (!view.empty() && std::isspace(view.back())) {
      view.remove_suffix(1);
    }
  }

  //  Trim to search character
  static void trimRight(std::string_view &view, char search) {
    trimRight(view); //  Remove whitespace
    while (!view.empty() && view.back() == search) {
      //  Remove search character
      view.remove_suffix(1);
    }
  }

  static auto parseArguments(std::string_view &view) -> std::expected<std::vector<double>, bool> {
    //  No data, no parsing
    if (view.empty()) return std::unexpected(false);

    //  Remove leading spaces, if any
    trimLeft(view);

    //  Character not found, parsing error
    if (view.empty()) return std::unexpected(false);

    //  Trim trailing space
    std::vector<double> values;

    while (!view.empty() && isDigit(view.front())) {
      values.push_back(value(view));
      trimLeft(view, ',');
      trimLeft(view);
    }

    //  arguments not found, parsing error
    if (values.empty()) return std::unexpected(false);
    return std::move(values);
  }

private:
  static auto parseElement(std::string_view &view) -> std::expected<Argument, bool> {
    //  No data, no parsing
    if (view.empty()) return std::unexpected(false);

    trimLeft(view);
    std::string name{text(view)};
    //  Function not found, parsing error
    if (name.empty()) return std::unexpected(false);

    //  Delete left paran, and leading and trailing spaces.
    trimLeft(view, '(');
    /*trimLeft(view);

    //  Character not found, parsing error
    if (view.empty()) return std::unexpected(false);

    //  Trim trailing space
    std::vector<double> values;

    while (isDigit(view.front())) {
      values.push_back(value(view));
      trimLeft(view, ',');
      trimLeft(view);
    }*/

    auto values = parseArguments(view);
    trimLeft(view, ')');
    //  arguments not found, parsing error
    if (!values.has_value() || (*values).empty()) return std::unexpected(false);

    //  Return result
    return std::move(std::make_pair(name, (*values)));
  }

  //  Extract function name
  static std::string_view text(std::string_view &view) {
    size_t pos{};
    while (!view.empty() && std::isalpha(view.at(pos))) {
      ++pos;
    }
    std::string_view temp = view.substr(0, pos);
    view.remove_prefix(pos);

    return temp;
  }

  // Extract function variables
  static double value(std::string_view &view) {
    size_t pos{};
    bool flag = true;
    while (flag && !view.empty() && pos < view.size()) {
      auto c = view.at(pos);
      if (isDigit(c)) ++pos;
      else flag = false;
    }
    double value{};
    std::string_view v = view.substr(0, pos);

    if (const auto num = Conversion::parseToNumber(v); num.has_value()) {
      value = *num;
    }
    view.remove_prefix(pos);

    return value;
  }

  // Extract function variables
  static double isDigit(const char c) {
    if (std::isdigit(c) || c == '+' || c == '-' || c == '.') return true;
    return false;
  }
};