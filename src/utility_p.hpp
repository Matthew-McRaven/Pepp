#pragma once
#include <algorithm> //  For std::max
#include <charconv>  //  For std::from_chars
#include <format>    //  For std::format
#include <ranges>
#include <string>
#include <string_view>

class SvgRect
{
    double _x = 0;
    double _y = 0;
    double _width = -1;
    double _height = -1;

public:
    bool empty() const { return _width <= 0 || _height <= 0; }

    auto x() const { return _x; }
    void setX(double x = 0) { _x = x; }
    auto y() const { return _y; }
    void setY(double y = 0) { _y = y; }
    auto width() const { return _width; }
    void setWidth(double width = 0) { _width = std::max(width, 0.0); }
    auto height() const { return _height; }
    void setHeight(double height = 0) { _height = std::max(height, 0.0); }

    bool fromString(const std::string &value)
    {
        auto view = value | std::views::split(' ');
        int i = 0;
        double result{};
        for (auto &&chunk : view) {
            //  Convert subrange into string view and then double
            std::string_view sv = std::string_view(chunk);
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);

            //  Check for parsing error, return if error
            if (ec != std::errc())
                return false;

            switch (i) {
            case 0:
                _x = result;
                break;
            case 1:
                _y = result;
                break;
            case 2:
                _width = result;
                break;
            case 3:
                _height = result;
                break;
            }
            ++i;
        }
        return true;
    }

    const std::string toString() const
    {
        std::string buffer = std::format("{} {} {} {}", _x, _y, _width, _height);
        return std::move(buffer);
    }
};
