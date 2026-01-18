#pragma once

#include <QString>
namespace bits {
bool startsWithHexPrefix(const QString &string);
qsizetype escapedStringLength(const QString &string);
bool escapedStringToBytes(const QString &string, QByteArray &output);

} // namespace bits
