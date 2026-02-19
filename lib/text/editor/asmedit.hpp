#pragma once
#include "editbase.hpp"

class AsmEdit : public EditBase {
  Q_OBJECT
  QML_ELEMENT
public:
  AsmEdit(QQuickItem *parent = 0);

private:
  const int symbolStyle = SCE_PEPASM_SYMBOL_DECL;
  const int mnemonicStyle = SCE_PEPASM_MNEMONIC;
  const int directiveStyle = SCE_PEPASM_DIRECTIVE;
  const int macroStyle = SCE_PEPASM_MACRO;
  const int charStyle = SCE_PEPASM_CHARACTER;
  const int stringStyle = SCE_PEPASM_STRING;
  const int commentStyle = SCE_PEPASM_COMMENT;
public slots:
  void applyStyles() override;
};
