#pragma once

#include <memory>
#include <string>
using namespace std::string_literals;

class SvgBasicElement;

class SvgFactory
{
public:
    //static auto createElement(const std::string &name);
    static std::unique_ptr<SvgBasicElement> createBasicElement(const std::string &name);
};
