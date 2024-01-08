/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <QtCore>
#include "asm/pas/pas_globals.hpp"

namespace pas::errors::pepp {
// Macro issues
inline const QString macroLoop = u"Macro invocation loop detected."_qs;
inline const QString noSuchMacro = u"No such macro \"%1\""_qs;
inline const QString macroWrongArity = u"%1 expected %2 args, received %3."_qs;
inline const QString expectedAMacro =
    u"Can only apply this operation to a macro node."_qs;
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
    u"Expected a decimal, hexadecimal, or symbolic argument."_qs;
inline const QString expectNArguments = u"Expected %1 argument(s)."_qs;
inline const QString expectNMArguments = u"Expected %1 to %2 argument(s)."_qs;
inline const QString expectedSymbolic = u"Expected a symbolic argument."_qs;
inline const QString sectionFlagsString = u"SECTION flags must be a string."_qs;
inline const QString illegalDirective = u"%1 is not a valid directive."_qs;
inline const QString illegalInUser = u"%1 cannot be used in a user program."_qs;
// Address problems
inline const QString objTooBig = u"Object code must fit within 65536 bytes."_qs;
// Bad program END
inline const QString missingEnd = u"Missing .END."_qs;
inline const QString onlyCommentAfterEnd = u""_qs;
// Symbol problems
inline const QString undefinedSymbol = u"Undefined symbol %1."_qs;
inline const QString multiplyDefinedSymbol = u"Multiply defined symbol %1."_qs;
inline const QString symbolNeedsDotInst = u""_qs;
inline const QString previouslyDefinedSymbol = u""_qs;
inline const QString tooLongSymbol = u""_qs;
inline const QString noDefineSymbol = u"%1 may not define a symbol."_qs;
// Burn problems
inline const QString onlyOSBurn = u""_qs;
inline const QString os1Burn = u""_qs;
inline const QString requiresHex =
    u"%1 requires a hexadecimal argument in range of [0x0000,0xFFFF]."_qs;
// Malformed values
inline const QString malformedChar = u""_qs;
inline const QString malformedString = u""_qs;
// Values too big
inline const QString strTooLong1 = u"String operands must have length 1."_qs;
inline const QString strTooLong2 = u"String operands must have length 2."_qs;
inline const QString decTooBig1 =
    u"Decimal constant is out of range [-128,255]"_qs;
inline const QString decTooBig2 =
    u"Decimal constant is out of range [-32768,65535]"_qs;
inline const QString decUnsigned2 =
    u"Decimal constant is out of range [0,65535]"_qs;
inline const QString hexTooBig1 =
    u"Hexadecimal constant is out of range [0x00, 0xFF]"_qs;
inline const QString hexTooBig2 =
    u"Hexadecimal constant is out of range [0x0000, 0xFFFF]"_qs;
inline const QString alignPow2 =
    u".ALIGN requires a decimal constant in (2, 4, 8)."_qs;

}; // namespace pas::errors::pepp
