#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include "SvgInterface.hpp"

class SvgFactory
{
public:
    //  Do not create an instance of this class
    SvgFactory() = delete;

    using CreatorFunc = std::function<std::unique_ptr<SvgInterface>()>;

    // Registers a new type dynamically
    static bool registerType(SvgType::Type type, CreatorFunc creator);

    //static std::unique_ptr<SvgInterface> createElement(const std::string &name);
    static std::unique_ptr<SvgInterface> createElement(const SvgType::Type type);

private:
    static std::unique_ptr<SvgInterface> createBasicElement(const SvgType::Type type,
                                                            const std::string name);
    // A registry map to hold our creation logic
    static std::unordered_map<SvgType::Type, CreatorFunc> _registry;
};
