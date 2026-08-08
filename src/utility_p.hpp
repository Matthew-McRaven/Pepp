#pragma once
#include <algorithm>

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
};
