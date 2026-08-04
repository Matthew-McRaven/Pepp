#pragma once

#include <list>
#include <string>
#include <vector>

//	External xml parser
//	3rd Party
#include "../3rdParty/rapidxml/rapidxml.hpp"

namespace XmlNode {
enum class Type { Element = 0, RootElement, RootAttribute, Attribute, EndElement };
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
    bool parse(const std::string &data)
    {
        //  If no characters, there is nothing to parse
        if (data.empty())
            return false;

        //	Clear prior XML and set to current string
        _doc.clear();

        _buffer.assign(data.begin(), data.end());
        _buffer.push_back('\0');

        //	Non destructive to original XML
        _doc.parse<rapidxml::parse_fastest>(&_buffer[0]);

        //	Go through XML starting with root node
        walk(_doc.first_node());

        return true;
    }

private:
    void walk(const rapidxml::xml_node<> *node)
    {
        //	Keep local copy of element
        std::string key;

        const rapidxml::node_type type = node->type();
        switch (type) {
        case rapidxml::node_element:

            //	Convert from character array to string
            _key.clear();
            _value.clear();
            key.append(node->name(), node->name_size());
            key = _key;

            //	Get attributes
            if (node->value_size()) {
                _value.append(node->value(), node->value_size());
            }

            //	No custom callback, use local variable
            _xml->addFromParser(_key,
                                _value,
                                _root ? XmlNode::Type::RootElement : XmlNode::Type::Element);

            //	Loop through attributes, if any
            for (const rapidxml::xml_attribute<> *attr = node->first_attribute(); attr;
                 attr = attr->next_attribute()) {
                //	Convert from character array to string
                _key.clear();
                _value.clear();
                _key.append(attr->name(), attr->name_size());
                _value.append(attr->value(), attr->value_size());

                //	This is a callback function!
                //  Save result
                _xml->addFromParser(_key,
                                    _value,
                                    _root ? XmlNode::Type::RootAttribute : XmlNode::Type::Attribute);
            }

            //	Signal that root element is complete
            _root = false;

            //	Keep traversing
            for (const auto *child = node->first_node(); child; child = child->next_sibling()) {
                walk(child);
            }
            //	This is a callback function!
            //	Signal end of element
            _xml->addFromParser(key, "", XmlNode::Type::EndElement);
            break;

        default:
            break;
        }
    }
};