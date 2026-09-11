#include "SvgFactory.hpp"

#include <string>
using namespace std::string_literals;

#include "SvgCDataElement.hpp"
#include "SvgCommentElement.hpp"
#include "SvgRectElement.hpp"
#include "SvgSvgElement.hpp"
#include "SvgUseElement.hpp"

bool SvgFactory::registerType(SvgType::Type type, CreatorFunc creator)
{
    // structured binding used here if inspecting insertion results
    auto [iterator, success] = _registry.insert({type, creator});
    return success;
}

// Initialize the static registry map
std::unordered_map<SvgType::Type, SvgFactory::CreatorFunc> SvgFactory::_registry
    = {{SvgType::Type::SvgCommentElement, []() { return std::make_unique<SvgCommentElement>(); }},
       {SvgType::Type::SvgCDataElement, []() { return std::make_unique<SvgCDataElement>(); }},
       {SvgType::Type::SvgRectElement, []() { return std::make_unique<SvgRectElement>(); }},
       {SvgType::Type::SvgUseElement, []() { return std::make_unique<SvgUseElement>(); }},
       {SvgType::Type::SvgDefsElement,
        []() { return createBasicElement(SvgType::Type::SvgDefsElement, "defs"s); }},
       {SvgType::Type::SvgGElement,
        []() { return createBasicElement(SvgType::Type::SvgGElement, "g"s); }},
       {SvgType::Type::SvgDescElement,
        []() { return createBasicElement(SvgType::Type::SvgDescElement, "desc"s); }},
       {SvgType::Type::SvgMetadataElement,
        []() { return createBasicElement(SvgType::Type::SvgMetadataElement, "metadata"s); }},
       {SvgType::Type::SvgTitleElement,
        []() { return createBasicElement(SvgType::Type::SvgTitleElement, "title"s); }}};

std::unique_ptr<SvgInterface> SvgFactory::createElement(const SvgType::Type type)
{
    auto it = _registry.find(type);
    if (it != _registry.end()) {
        // execute the mapped creator function
        return it->second();
    }

    // Fallback default
    return std::make_unique<SvgElement>();
}

//  Helper function when reusing SvgElement for other elements
std::unique_ptr<SvgInterface> SvgFactory::createBasicElement(const SvgType::Type type,
                                                             const std::string name)
{
    auto temp = std::make_unique<SvgElement>(name);
    temp->setElementType(type);
    return std::move(temp);
}
