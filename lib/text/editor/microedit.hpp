#pragma once

#include "editbase.hpp"
#include "micro_line_numbers.hpp"

class MicroEdit : public EditBase {
  Q_OBJECT
  Q_PROPERTY(pepp::LineNumbers *cycleNumbers READ lineNumbers WRITE setLineNumbers NOTIFY lineNumberChanged)
  QML_ELEMENT
public:
  MicroEdit(QQuickItem *parent = 0);
  pepp::LineNumbers *lineNumbers() const;
  void setLineNumbers(pepp::LineNumbers *lineNumber);

public slots:
  void applyStyles() override;
  void toggleComment();
signals:
  void lineNumberChanged();

private:
  const int symbolStyle = SCE_PEPMICRO_SYMBOL_DECL;
  const int commentStyle = SCE_PEPMICRO_COMMENT;
  const int integerStyle = SCE_PEPMICRO_INTEGER;
  const int prepostStyle = SCE_PEPMICRO_PREPOST;
  const int signalStyle = SCE_PEPMICRO_IDENTIFIER;
  const int cycleNumStyle = warningStyle + 1;
  pepp::LineNumbers *_lineNumber = nullptr;
};
