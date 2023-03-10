#pragma once

#include <QSharedPointer>
#include <QString>
#include <QtCore>
#include <optional>
#include "builtins_globals.hpp"
Q_MOC_INCLUDE("builtins/figure.hpp")
namespace builtins {
Q_NAMESPACE_EXPORT(BUILTINS_EXPORT)

//! Describe which architecture a help item is to be used with.
enum class Architecture {
  PEP8 = 80,    //! The figure must be used with the Pep/8 toolchain.
  PEP9 = 90,    //! The figure must be used with the Pep/9 toolchain.
  PEP10 = 100,  //! The figure must be use with the Pep/10 toolchain
  RISCV = 1000, //! The figure must be used with the RISC-V toolchain, which is
                //! undefined as of 2023-02-14.
};
Q_ENUM_NS(Architecture);

class Figure;
/*!
 * \brief Contains a unit of content that makes up a help item
 */
struct BUILTINS_EXPORT Element : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(bool generated MEMBER generated);
  Q_PROPERTY(QString language MEMBER language);
  Q_PROPERTY(QString content MEMBER contents);
  Q_PROPERTY(QWeakPointer<Figure> figure MEMBER figure);

public:
  //! Is the element created dynamicaly at runtime (e.g., pepo/pepb/peph/pepl),
  //! or is it "baked in" to the QRC (pep/c)
  bool generated;
  //! The programming language this element is written in
  QString language;
  //! The textual contents of the element
  QString contents;
  //! The figure which contains this element. Needed to access default OS / test
  //! items.
  QWeakPointer<Figure> figure;
};

/*!
 * \brief A single input:output pair that can be used to unit test an
 * figure.
 */
struct BUILTINS_EXPORT Test : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(QVariant input MEMBER input);
  Q_PROPERTY(QVariant output MEMBER output);

public:
  //! If present, it is a string containing the input on which the figure should
  //! be run.
  QVariant input;
  //! If present, it is the required output of the figure when run on the
  //! supplied input.
  QVariant output;
};

struct BUILTINS_EXPORT Macro : public QObject {
private:
  Q_OBJECT
  Q_PROPERTY(Architecture arch MEMBER arch);
  Q_PROPERTY(QString name MEMBER name);
  Q_PROPERTY(QString text MEMBER text);

public:
  Architecture arch;
  QString name;
  QString text;
};
} // end namespace builtins
