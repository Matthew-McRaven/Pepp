#pragma once
#include <algorithm> //  For std::max
#include <charconv>  //  For std::from_chars
#include <format>    //  For std::format
#include <ranges>
#include <string>
#include <string_view>
using namespace std::string_literals;

struct SvgUnits
{
    enum class SvgUnit {
        None = 0,
        cm = 1, // Centimeters
        mm,     // Millimeters
        Q,      // Quarter-Millimeters
        in,     // Inches
        pc,     // Pica
        pt,     // Points
        px,     // Pixels
    };
    static SvgUnit fromString(const std::string &value)
    {
        if (value == "cm"s)
            return SvgUnit::cm;
        if (value == "mm"s)
            return SvgUnit::mm;
        if (value == "Q"s)
            return SvgUnit::Q;
        if (value == "in"s)
            return SvgUnit::in;
        if (value == "pc"s)
            return SvgUnit::pc;
        if (value == "pt"s)
            return SvgUnit::pt;
        if (value == "px"s)
            return SvgUnit::px;

        return SvgUnit::None;
    }

    static const std::string toString(SvgUnit v)
    {
        switch (v) {
        case SvgUnit::cm:
            return "cm"s;
        case SvgUnit::mm:
            return "mm"s;
        case SvgUnit::Q:
            return "Q"s;
        case SvgUnit::in:
            return "in"s;
        case SvgUnit::pc:
            return "pc"s;
        case SvgUnit::pt:
            return "pt"s;
        case SvgUnit::px:
            return "px"s;
        }
        return "";
    }
};

struct SvgUnitValue
{
    double value = 0.0;
    SvgUnits::SvgUnit unit = SvgUnits::SvgUnit::None;

    SvgUnitValue(const double v, const SvgUnits::SvgUnit u = SvgUnits::SvgUnit::None)
        : value(v)
        , unit(u)
    {}

    void set(const double v, const SvgUnits::SvgUnit u)
    {
        value = v;
        unit = u;
    }

    bool fromString(const std::string_view &sv)
    {
        double result{};
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);

        //  Check for parsing error, return if error
        if (ec != std::errc())
            return false;

        SvgUnits::SvgUnit u = SvgUnits::SvgUnit::None;
        if (ptr < sv.data() + sv.size())
            u = SvgUnits::fromString(ptr);

        value = result;
        unit = u;

        return true;
    }

    const std::string toString() const
    {
        std::string buffer = std::format("{}{}", value, SvgUnits::toString(unit));
        return std::move(buffer);
    }
};

class SvgRect
{
    SvgUnitValue _x{0.0};
    SvgUnitValue _y{0.0};
    SvgUnitValue _width{-1.0};
    SvgUnitValue _height{-1.0};

public:
    bool empty() const { return _width.value <= 0 || _height.value <= 0; }

    //  Values can be changed, but not units of measure (yet)
    auto x() const { return _x.value; }
    void setX(double x = 0) { _x.value = x; }
    auto y() const { return _y.value; }
    void setY(double y = 0) { _y.value = y; }
    auto width() const { return _width.value; }
    void setWidth(double width = 0) { _width.value = std::max(width, 0.0); }
    auto height() const { return _height; }
    void setHeight(double height = 0) { _height.value = std::max(height, 0.0); }

    bool fromString(const std::string &value)
    {
        static const char delimit(' ');
        auto view = value | std::views::split(delimit);
        int i = 0;
        double result{};
        for (auto &&chunk : view) {
            //  Convert subrange into string view and then double
            std::string_view sv = std::string_view(chunk);

            switch (i) {
            case 0:
                if (!_x.fromString(sv))
                    return false;
                break;
            case 1:
                if (!_y.fromString(sv))
                    return false;
                break;
            case 2:
                if (!_width.fromString(sv))
                    return false;
                break;
            case 3:
                if (!_height.fromString(sv))
                    return false;
                break;
            }
            ++i;
        }

        return true;
    }

    const std::string toString() const
    {
        std::string buffer = std::format("{} {} {} {}",
                                         _x.toString(),
                                         _y.toString(),
                                         _width.toString(),
                                         _height.toString());
        return std::move(buffer);
    }
};
