#include "sequenceconverter.hpp"

utils::SequenceConverter::SequenceConverter(QObject *parent) : QObject{parent} {}

QString utils::SequenceConverter::toNativeText(const QVariant &sequence) {
  QKeySequence ks;
  if (auto asI = sequence; asI.canConvert<int>() && asI.convert((QMetaType)QMetaType::Int)) {
    ks = QKeySequence(static_cast<QKeySequence::StandardKey>(asI.value<int>()));
  } else if (auto asS = sequence; asS.canConvert<QString>() && asS.convert((QMetaType)QMetaType::QString)) {
    ks = QKeySequence(sequence.value<QString>());
  } else if (auto asL = sequence; asL.canConvert<QVariantList>() && asL.convert((QMetaType)QMetaType::QVariantList)) {
    QVariantList asList = asL.toList();
    if (asList.length() >= 1) return toNativeText(asList[0]);
    else return "";
  } else return "";
  return ks.toString(QKeySequence::NativeText);
}
