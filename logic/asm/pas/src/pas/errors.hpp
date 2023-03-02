#pragma once
#include <QtCore>
namespace pas::errors::pepp {
// Bad instructions and command;
inline const QString invalidMnemonic = u"Invalid mnemonic."_qs;
inline const QString invalidDirective = u"Invalid dot command."_qs;
inline const QString invalidSection = u"Invalid section directive."_qs;
inline const QString illegalAddrMode =
    u"Illegal addressing mode for this instruction"_qs;
inline const QString invalidAddrMode = u"Invalid addressing mode."_qs;
inline const QString requiredAddrMode =
    u"Addressing mode required for this instruction"_qs;
inline const QString badLineStart = u""_qs;
inline const QString equateRequiresSymbol =
    u".EQUATE must have a symbol definition."_qs;
inline const QString dotRequiresString =
    u"%1 requires a string constant argument."_qs;
inline const QString argAfterMnemonic = u""_qs;
inline const QString expectedNumeric =
    u"Expected a decimal, hexadecimal, os symbolic argument."_qs;
inline const QString expectNArguments = u"Expected %1 argument(s)."_qs;
inline const QString expectedSymbolic = u"Expected a symbolic argument."_qs;
// Address problems
inline const QString objTooBig = u""_qs;
// Bad program END
inline const QString missingEnd = u""_qs;
inline const QString onlyCommentAfterEnd = u""_qs;
// Symbol problems
inline const QString undefinedSymbol = u""_qs;
inline const QString symbolNeedsDotInst = u""_qs;
inline const QString previouslyDefinedSymbol = u""_qs;
inline const QString tooLongSymbol = u""_qs;
inline const QString noDefineSymbol = u"%1 may not define a symbol."_qs;
// Burn problems
inline const QString onlyOSBurn = u""_qs;
inline const QString os1Burn = u""_qs;
inline const QString burnRequiresHex =
    u".BURN requires a hexadecimal argument in range of [0x0000,0xFFFF]."_qs;
// Malformed values
inline const QString malformedChar = u""_qs;
inline const QString malformedString = u""_qs;
// Values too big
inline const QString strTooLong1 = u"String operands must have length 1."_qs;
inline const QString strTooLong2 = u"String operands must have length 2."_qs;
inline const QString decTooBig1 =
    u"Decimal constant is out of range [-128,255]"_qs;
inline const QString decTooBig2 =
    u"Decimal constant is out of range [-32678,65535]"_qs;
inline const QString hexTooBig1 =
    u"Hexadecimal constant is out of range [0x00, 0xFF]"_qs;
inline const QString hexTooBig2 =
    u"Hexadecimal constant is out of range [0x0000, 0xFFFF]"_qs;
inline const QString alignPow2 =
    u".ALIGN requires a decimal constant in (2, 4, 8)."_qs;

}; // namespace pas::errors::pepp
