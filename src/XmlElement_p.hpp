#pragma once

//  W A R N I N G
//  -------------
//
// This header is not part of the public interface and may
// change without notice.

//	This class is used to capture all xml elements in a file
//	It does not perform any transformation. Only used for
//	Excel files without special access logic.

//	Standard library
#include <list>
#include <map>
#include <string>

using namespace std::string_literals;

#include "SvgParser.hpp" //	For XmlNode::Type
#include "XmlAttributes_p.hpp"

class XmlElement
{
    //	Required header
    bool trimEmpty_{true}; //	Trim empty elements

    std::string name_;
    std::string value_;
    XmlAttributes attrs_;
    std::list<XmlElement> children_;

public:
    void update(const std::string &key, XmlElement *value) {};

    XmlElement(const std::string &name, const std::string &value)
        : name_(name)
        , value_(value) {};

    XmlElement() = default;
    ~XmlElement() = default;
    XmlElement(const XmlElement &) = default;
    XmlElement &operator=(const XmlElement &) = default;
    XmlElement(XmlElement &&) noexcept = default;
    XmlElement &operator=(XmlElement &&) noexcept = default;

    void reset()
    {
        name_.clear();
        value_.clear();
        attrs_.clear();
        children_.clear();
    }
    void addAttribute(const std::string &name, const std::string &value)
    {
        attrs_.add(name, value);
    }

    void clearAttribute() { attrs_.clear(); }

    void removeAttribute(const std::string &name) { attrs_.remove(name); }

    const XmlAttributes &attributes() const { return attrs_; }

    size_t countAttribute() const { return attrs_.size(); }

    const std::string &findAttribute(const std::string &name) const { return attrs_.find(name); }

    const std::string &findAttributeValue(const std::string &value) const
    {
        return attrs_.findValue(value);
    }

    XmlElement &addChild(const std::string &name, const std::string &value)
    {
        XmlElement &child = children_.emplace_back(name, value);
        child.setTrim(trimEmpty_);
        return child;
    }

    void clearChildren() { children_.clear(); }

    void removeChild(const std::string &name)
    {
        std::list<XmlElement>::iterator it;

        for (it = children_.begin(); it != children_.end(); ++it) {
            if (it->name() == name) {
                children_.erase(it);
                return;
            }
        }
    }

    size_t countChildren() const { return children_.size(); }

    const XmlElement *findChild(const std::string &name) const
    {
        const XmlElement *result = nullptr;

        for (const auto &child : children_) {
            if (child.name() == name) {
                result = &child;
                break;
            }
        }

        return result;
    }

    XmlElement *findChild(const std::string &name)
    {
        XmlElement *result = nullptr;

        for (auto &child : children_) {
            if (child.name() == name) {
                result = &child;
                break;
            }
        }

        return result;
    }

    XmlElement *findChild(const size_t index)
    {
        //	Protect against invalid index
        if (index < 0 || index > children_.size())
            return nullptr;

        auto it = children_.begin();
        std::advance(it, index);

        return &(*it);
    }

    const XmlElement *findChild(const size_t index) const
    {
        //	Protect against invalid index
        if (index < 0 || index > children_.size())
            return nullptr;

        auto it = children_.cbegin();
        std::advance(it, index);

        return &(*it);
    }

    XmlElement *findChild(const XmlElement *search)
    {
        //	Protect against invalid index
        if (search == nullptr)
            return nullptr;

        //	Search through all elements for match
        for (auto it = children_.begin(); it != children_.end(); ++it) {
            if (&(*it) == search) {
                return &(*it);
            }
        }

        return nullptr;
    }

    bool remove(const XmlElement *search)
    {
        //	Search through all elements for match
        for (auto it = children_.cbegin(); it != children_.cend(); ++it) {
            if (&(*it) == search) {
                children_.erase(it);
                return true;
            }
        }

        return false;
    }

    //	Accessors
    const std::string &name() const { return name_; }
    void setName(const std::string &value) { name_ = value; }
    const std::string &value() const { return value_; }
    void setValue(const std::string &value) { value_ = value; }
    bool trim() const { return trimEmpty_; }
    void setTrim(const bool value)
    {
        //	Cascade down to children
        trimEmpty_ = value;

        for (auto &child : children_) {
            //	Use same trim flag as parent
            child.setTrim(trimEmpty_);
        }
    }

    std::list<XmlElement> &children() { return children_; }
    const std::list<XmlElement> &children() const { return children_; }

    const std::string write() const
    {
        std::string data;

        //	Check if there is data, if not return nothing
        if (!trimEmpty_ || attrs_.size() > 0 || !children_.empty() || !value_.empty()) {
            //	Keep local copy
            data.append("<" + name_);

            //	Output attributes, skip if no attributes
            if (attrs_.size() > 0)
                data.append(attrs_.write());

            //	close element differently depending on existing children
            if (children_.empty() && value_.empty())
                //	Close element, we are done
                data.append("/>");
            else {
                //	Close parent element
                data.append(">");

                //	Save value, if present
                if (!value_.empty())
                    data.append(value_);

                //	Output children and close tag
                //	This is recursive. Children call children
                for (const auto &child : children_) {
                    //	Use same trim flag as parent
                    data.append(child.write());
                }

                //	Add closing element
                data.append("</" + name_ + ">");
            }
        }

        //	Return element and children
        return std::move(data);
    }

    //	Parser call back function to map fields to class
    void addFromParser(const std::string &name, const std::string &value, const XmlNode::Type type)
    {
        static XmlElement *current = nullptr;
        static std::list<XmlElement *> parents;

        switch (type) {
        case XmlNode::Type::RootElement:
            //	Header elements have name value pair
            setName(name);
            setValue(value);
            break;
        case XmlNode::Type::RootAttribute:
            //	Header elements have name value pair
            addAttribute(name, value);
            break;
        case XmlNode::Type::Element:
            if (current) {
                //  Save current parent
                parents.push_back(current);

                //  This is a child of another element
                current = &current->addChild(name, value);
            } else {
                //  This is direct descendent of current element
                current = &addChild(name, value);
            }

            // Cascade trim flag to children
            current->setTrim(trim());

            break;
        case XmlNode::Type::Attribute:
            if (current)
                current->addAttribute(name, value);
            else
                addAttribute(name, value);
            break;
        case XmlNode::Type::EndElement:
            if (parents.empty()) {
                current = nullptr;
            } else {
                //  return to previous parent
                current = parents.back();
                parents.pop_back();
            }
            break;
        }
    }
};
