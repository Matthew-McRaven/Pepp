#include "sequenceconverter.hpp"

utils::SequenceConverter::SequenceConverter(QObject *parent) : QObject{parent} {}

QString utils::SequenceConverter::toNativeText(const QVariant &sequence) {
  QKeySequence ks;
  if(sequence.canConvert<int>()) {
    ks = QKeySequence(static_cast<QKeySequence::StandardKey>(sequence.value<int>()));
  } else if(sequence.canConvert<QString>()) {
    ks = QKeySequence(sequence.value<QString>());
  } else if(sequence.canConvert<QVariantList>()) {
    QVariantList asList = sequence.toList();
    if(asList.length() >= 1) return toNativeText(asList[0]);
      else return "";
  }
  else return "";
  return ks.toString(QKeySequence::NativeText);
}
