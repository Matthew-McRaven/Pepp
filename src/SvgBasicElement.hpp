#pragma once

#include <string>

#include "SvgInterface.h"

//	Forward declarations
class SvgBasicElementImpl;
class DocumentImpl;

class SvgBasicElement final : public SvgInterface
{
    //  Pointer to document for callbacks
    DocumentImpl *_doc{};

    //  Standard Xml Data
    SvgInterface::SvgType _elementType = SvgInterface::SvgType::SvgUnknownElement;
    std::string _xmlName;
    std::string _value;

public:
    SvgBasicElement();
    explicit SvgBasicElement(DocumentImpl *d);

    explicit SvgBasicElement(const std::string &xmlName, const std::string &value = "");
    ~SvgBasicElement() = default;
    //Cannot copy, but can move
    SvgBasicElement(const SvgBasicElement &) = default;
    SvgBasicElement &operator=(const SvgBasicElement &) = default;
    SvgBasicElement(SvgBasicElement &&) noexcept = default;
    SvgBasicElement &operator=(SvgBasicElement &&) noexcept = default;

    //	User access functions
    //  Standard Xml
    const std::string &xmlName() const;
    void setXmlName(std::string xmlName);
    const std::string &value() const;
    void setValue(std::string value);

    //  Called by base class
    void toXml(SvgRope &output) const override;
    bool setAttribute(const std::string &key, const std::string &value) override { return true; };
    SvgInterface::SvgType elementType() const override;
    void setElementType(SvgInterface::SvgType elementType) override;

    //std::unique_ptr<SvgInterface> clone() const;
};
