#pragma once

#include <memory>
#include <string>

#include "SvgInterface.h"

//	Forward declarations
class SvgBasicElementImpl;

class SvgBasicElement : public SvgInterface
{
    std::unique_ptr<SvgBasicElementImpl> _impl;

public:
    SvgBasicElement();
    explicit SvgBasicElement(const std::string &xmlName, const std::string &value = "");
    ~SvgBasicElement();
    //Cannot copy, but can move
    SvgBasicElement(const SvgBasicElement &);
    SvgBasicElement &operator=(const SvgBasicElement &);
    SvgBasicElement(SvgBasicElement &&) noexcept;
    SvgBasicElement &operator=(SvgBasicElement &&) noexcept;

    //	User access functions
    //  Standard Xml
    SvgInterface::SvgType elementType() const;
    void setElementType(SvgInterface::SvgType elementType);
    std::string &xmlName() const;
    void setXmlName(std::string xmlName);
    std::string &value() const;
    void setValue(std::string value);

    //  Called by base class
    void toXml(SvgRope &output) const override;
    std::unique_ptr<SvgInterface> clone() const;
};
