#pragma once
#include <algorithm> //  For std::max
#include <cassert>   //  For std::assert
#include <charconv>  //  For std::from_chars
#include <format>    //  For std::format
#include <limits>    //  For std::numeric_limits
#include <list>      //  For std::list
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
        pct,    // %
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
        if (value == "%"s)
            return SvgUnit::pct;
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
        case SvgUnit::pct:
            return "%"s;
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
    double value = std::numeric_limits<double>::denorm_min();
    SvgUnits::SvgUnit unit = SvgUnits::SvgUnit::None;

    SvgUnitValue() {}
    SvgUnitValue(const double v, const SvgUnits::SvgUnit u = SvgUnits::SvgUnit::None)
        : value(v)
        , unit(u)
    {}

    void set(const double v, const SvgUnits::SvgUnit u)
    {
        value = v;
        unit = u;
    }

    bool empty() const { return value == std::numeric_limits<double>::denorm_min(); }
    bool fromString(const std::string_view sv) {
      double result;
#if PEPP_HAS_DOUBLE_FROM_CHARS
      // macOS 10.15 (Catalina) or later
      auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), result);
#else
      char *endptr;
      result = std::strtod(sv.data(), &endptr);
      std::errc ec = (endptr == sv.data()) ? std::errc::invalid_argument : std::errc();
      const char *ptr = endptr;
#endif

      //  Check for parsing error, return if error
      if (ec != std::errc()) return false;

      SvgUnits::SvgUnit u = SvgUnits::SvgUnit::None;
      if (ptr < sv.data() + sv.size()) u = SvgUnits::fromString(ptr);

      value = result;
      unit = u;

      return true;
    }

    const std::string toString() const
    {
        std::string buffer;

        //  If value wa never set, do not output value
        if (value != std::numeric_limits<double>::denorm_min())
            buffer = std::format("{}{}", value, SvgUnits::toString(unit));
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
    SvgRect() = default;
    SvgRect(const double x, const double y, const double width, const double height)
        : _x(x)
        , _y(y)
        , _width(width)
        , _height(height)
    {}
    ~SvgRect() = default;
    SvgRect(const SvgRect &) = default;
    SvgRect &operator=(const SvgRect &) = default;
    SvgRect(SvgRect &&) noexcept = default;
    SvgRect &operator=(SvgRect &&) noexcept = default;

    bool empty() const { return _width.value <= 0 || _height.value <= 0; }

    //  Values can be changed, but not units of measure (yet)
    SvgUnitValue x() const { return _x.value; }
    void setX(double x = 0) { _x.value = x; }
    void setX(const std::string_view sv) { _x.fromString(sv); }
    SvgUnitValue y() const { return _y.value; }
    void setY(double y = 0) { _y.value = y; }
    void setY(const std::string_view sv) { _y.fromString(sv); }
    SvgUnitValue width() const { return _width.value; }
    void setWidth(double width = 0) { _width.value = std::max(width, 0.0); }
    void setWidth(const std::string_view sv) { _width.fromString(sv); }
    SvgUnitValue height() const { return _height; }
    void setHeight(double height = 0) { _height.value = std::max(height, 0.0); }
    void setHeight(const std::string_view sv) { _height.fromString(sv); }

    bool fromString(const std::string &value)
    {
        static const char delimit(' ');
        auto view = value | std::views::split(delimit);
        auto it = view.begin();

        std::string_view sv = std::string_view(*it++);
        if (!_x.fromString(sv))
            return false;
        assert(it != view.end());
        if (!_y.fromString(std::string_view{*it++}))
            return false;
        assert(it != view.end());
        if (!_width.fromString(std::string_view{*it++}))
            return false;
        assert(it != view.end());
        if (!_height.fromString(std::string_view{*it}))
            return false;
        assert(it != view.end());

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

class SvgRope
{
    size_t _size = 0;
    std::list<std::string> _rope;

public:
    size_t size() const { return _size; }
    const auto &rope() const { return _rope; }
    void clear()
    {
        _size = 0;
        _rope.clear();
    }
    void push_back(std::string &&value)
    {
        _rope.push_back(value);
        _size += value.size();
    }
    void push_back(std::string &value)
    {
        _rope.push_back(value);
        _size += value.size();
    }
};
