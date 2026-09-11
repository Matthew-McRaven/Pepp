#pragma once

//  W A R N I N G
//  -------------
//
// This header is not part of the public interface and may
// change without notice.

//	Standard library
#include <format>
#include <list>
#include <ranges>
#include <string>

using namespace std::string_literals;

class XmlAttributes
{
    std::list<std::pair<std::string, std::string>> attrs_;

public:
    XmlAttributes() = default;
    ~XmlAttributes() = default;
    XmlAttributes(const XmlAttributes &) = default;
    XmlAttributes &operator=(const XmlAttributes &) = default;
    XmlAttributes(XmlAttributes &&) noexcept = default;
    XmlAttributes &operator=(XmlAttributes &&) = default;

    const std::list<std::pair<std::string, std::string>> &attributes() const { return attrs_; }

    void add(const std::string &name, const std::string &value)
    { //	Keep order, do not resort
        attrs_.emplace_back(name, value);
    }

    void clear() { attrs_.clear(); }

    void remove(const std::string &name)
    {
        attrs_.remove_if([&name](const std::pair<std::string, std::string> &attr) {
            return name == attr.first;
        });
    }

    size_t size() const { return attrs_.size(); }

    const std::string &find(const std::string &name) const
    {
        static std::string empty;

        //	Search lamdba
        auto query = [&name](const std::pair<std::string, std::string> &attr) {
            return name == attr.first;
        };

        //	Execute search
        auto it = std::ranges::find_if(attrs_.cbegin(), attrs_.cend(), query);

        if (it != attrs_.cend())
            return it->second;

        return empty;
    }

    const std::string &findValue(const std::string &value) const
    {
        static std::string empty;

        //	Search lamdba
        auto query = [&value](const std::pair<std::string, std::string> &attr) {
            return value == attr.second;
        };

        //	Execute search
        auto it = std::ranges::find_if(attrs_.cbegin(), attrs_.cend(), query);

        if (it != attrs_.cend())
            return it->second;

        return empty;
    }

    const std::string write() const
    {
        std::string data;

        //	Output atributes as string
        for (const auto &attr : attrs_) {
            //	Output attributes
            std::string head = std::format(" {0}=\"{1}\"", attr.first, attr.second);

            data.append(head);
        }

        return data;
    }
};
