#include "SvgBasicElement.hpp"

//	Standard library
#include <string>

#include "SvgDocument_p.hpp"
#include "SvgInterface.h"
#include "utility_p.hpp"

/*
From w3.org: https://www.w3.org/TR/SVG2/types.html#InterfaceSVGElement
    
All of the SVG DOM interfaces that correspond directly to elements in the
SVG language (such as the SVGPathElement interface for the ‘path’ element)
derive from the SVGElement interface.
*/

//	Public interface
SvgBasicElement::SvgBasicElement()
    : SvgInterface()
{
    //    _impl = std::make_unique<SvgBasicElementImpl>();
    _elementType = SvgInterface::SvgType::SvgBasicElement;
}

//	Public interface
SvgBasicElement::SvgBasicElement(DocumentImpl *d)
    : SvgBasicElement()
{
    _doc = d;
}

SvgBasicElement::SvgBasicElement(const std::string &xmlName, const std::string &value)
    : SvgBasicElement()
{
    _xmlName = xmlName;
    _value = value;
}

//  Generic Dom fields
const std::string &SvgBasicElement::xmlName() const
{
    return _xmlName;
}
void SvgBasicElement::setXmlName(std::string xmlName)
{
    _xmlName = xmlName;
}
const std::string &SvgBasicElement::value() const
{
    return _value;
}
void SvgBasicElement::setValue(std::string value)
{
    _value = value;
}

void SvgBasicElement::toXml(SvgRope &output) const
{
    //  Id currently has element name. Change when attributes are supported
    /*if (_impl->elementType == SvgInterface::SvgType::SvgComment) {
        std::string buffer = std::format("<!--{}-->", _impl->value);
        output.push_back(std::move(buffer));
        //  Comments cannot have children or attributes
        return;
    }*/
    output.push_back("<" + _xmlName);

    //  no value, add end tag
    if (_value.empty()) {
        output.push_back(" />");
        return;
    }
    output.push_back(">");

    //	Save value, if present
    if (!_value.empty())
        //  Create copy
        output.push_back(std::string{_value});

    //  After child elements, add closing element
    output.push_back("</" + _xmlName + ">");
}

SvgInterface::SvgType SvgBasicElement::elementType() const
{
    return _elementType;
}
void SvgBasicElement::setElementType(SvgInterface::SvgType elementType)
{
    _elementType = elementType;
}
