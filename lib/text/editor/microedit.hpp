#pragma once

#include "editbase.hpp"

class MicroEdit : public EditBase {
  Q_OBJECT
  QML_ELEMENT
public:
  MicroEdit(QQuickItem *parent = 0);

private:
  const int symbolStyle = SCE_PEPMICRO_SYMBOL_DECL;
  const int commentStyle = SCE_PEPMICRO_COMMENT;
  const int integerStyle = SCE_PEPMICRO_INTEGER;
  const int prepostStyle = SCE_PEPMICRO_PREPOST;
  const int signalStyle = SCE_PEPMICRO_IDENTIFIER;
public slots:
  void applyStyles() override;
};
