#pragma once

#include <memory>
#include <string>
using namespace std::string_literals;

#include "SvgColor.hpp"
#include "SvgType.hpp"

class SvgRope;
class SvgDocument;

// CRTP helper class that implements clone() automatically
template<class Derived, class Base>
class Cloneable : public Base
{
    friend class SvgInterface;
    Base *cloneImpl() const override { return new Derived(static_cast<Derived const &>(*this)); }

public:
    //  Hides Base::clone() so the result keeps the caller's static type.
    std::unique_ptr<Derived> clone() const
    {
        return std::unique_ptr<Derived>(static_cast<Derived *>(this->cloneImpl()));
    }

    //  Get pointer to leaf class
    // Derived *derived() { return static_cast<Derived *>(this); }
};

class SvgInterface
{
    //  Base class used to enforce enterface used by derived Svg elements
    inline static SvgDocument *_docs = nullptr;

  protected:
    virtual SvgInterface *cloneImpl() const = 0;
    //  Type cannot be externally changed
    virtual void setElementType(const SvgType::Type elementType) = 0;

  public:
    virtual ~SvgInterface() = default;

    //  Overrides
    virtual void toXml(SvgRope &output) const = 0;

    //  Values in every class
    virtual SvgType::Type elementType() const = 0;
    virtual const std::string &xmlName() const = 0;
    virtual void setXmlName(const std::string &xmlName) = 0;
    virtual const std::string &value() const = 0;
    virtual void setValue(const std::string &key) = 0;
    virtual void setDocument(SvgDocument *doc) = 0;
    virtual bool setAttribute(const std::string &key, const std::string &value) = 0;
    virtual void appendChild(SvgInterface *child) = 0;

    //  Attributes
    virtual const SvgColor::Color fill() const = 0;
    virtual void setFill(const SvgColor::Color color) = 0;
    virtual const double &opacity() const = 0;
    virtual void setOpacity(const double &opacity) = 0;

    static void setCurrentDocument(SvgDocument *doc = nullptr) { _docs = doc; }
    static SvgDocument *currentDocument() { return _docs; }

    virtual SvgInterface *createElement(const std::string &name) = 0;
    virtual SvgInterface *createElement(const SvgType::Type type) = 0;

    template <class Derived> Derived *createElementAs(const std::string &name) {
      return dynamic_cast<Derived *>(createElement(name));
    }
    template <class Derived> Derived *createElementAs(const SvgType::Type type) {
      return dynamic_cast<Derived *>(createElement(type));
    }

    //  Cloneable hides this with a version returning the derived type.
    std::unique_ptr<SvgInterface> clone() const { return std::unique_ptr<SvgInterface>(cloneImpl()); }

    template <class Derived> std::unique_ptr<Derived> cloneAs() const {
      return std::unique_ptr<Derived>(dynamic_cast<Derived *>(cloneImpl()));
    }
};
