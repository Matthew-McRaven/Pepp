#pragma once

#include <string>
#include <vector>

//	External xml parser
//	3rd Party
#include <rapidxml.hpp>

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