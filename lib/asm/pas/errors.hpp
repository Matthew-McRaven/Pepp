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

namespace pas::errors::pepp {
// Macro issues
inline const QString macroLoop = "Macro invocation loop detected.";
inline const QString noSuchMacro = "No such macro \"%1\"";
inline const QString invalidMacro = "Invalid macro.";
inline const QString macroWrongArity = "%1 expected %2 args, received %3.";
inline const QString expectedAMacro = "Can only apply this operation to a macro node.";
// Bad instructions and command;
inline const QString invalidMnemonic = "Invalid mnemonic.";
inline const QString invalidDirective = "Invalid dot command.";
inline const QString invalidSection = "Invalid section directive.";
inline const QString illegalAddrMode = "Illegal addressing mode for this instruction";
inline const QString invalidAddrMode = "Invalid addressing mode.";
inline const QString requiredAddrMode = "Addressing mode required for this instruction";
inline const QString badLineStart = "";
inline const QString equateRequiresSymbol = ".EQUATE must have a symbol definition.";
inline const QString dotRequiresString = "%1 requires a string constant argument.";
inline const QString argAfterMnemonic = "";
inline const QString expectedNumeric = "Expected a decimal, hexadecimal, or symbolic argument.";
inline const QString expectNArguments = "Expected %1 argument(s).";
inline const QString expectNMArguments = "Expected %1 to %2 argument(s).";
inline const QString expectedSymbolic = "Expected a symbolic argument.";
inline const QString sectionFlagsString = "SECTION flags must be a string.";
inline const QString illegalDirective = "%1 is not a valid directive.";
inline const QString illegalInUser = "%1 cannot be used in a user program.";
// Address problems
inline const QString objTooBig = "Object code must fit within 65536 bytes.";
// Bad program END
inline const QString missingEnd = "Missing .END.";
inline const QString onlyCommentAfterEnd = "";
// Symbol problems
inline const QString undefinedSymbol = "Undefined symbol %1.";
inline const QString multiplyDefinedSymbol = "Multiply defined symbol %1.";
inline const QString symbolNeedsDotInst = "";
inline const QString previouslyDefinedSymbol = "";
inline const QString tooLongSymbol = "";
inline const QString noDefineSymbol = "%1 may not define a symbol.";
// Burn problems
inline const QString onlyOSBurn = "";
inline const QString os1Burn = "";
inline const QString requiresHex = "%1 requires a hexadecimal argument in range of [0x0000,0xFFFF].";
// Malformed values
inline const QString malformedChar = "";
inline const QString malformedString = "";
// Values too big
inline const QString strTooLong1 = "String operands must have length 1.";
inline const QString strTooLong2 = "String operands must have length 2.";
inline const QString decTooBig1 = "Decimal constant is out of range [-128,255]";
inline const QString decTooBig2 = "Decimal constant is out of range [-32768,65535]";
inline const QString decUnsigned2 = "Decimal constant is out of range [0,65535]";
inline const QString hexTooBig1 = "Hexadecimal constant is out of range [0x00, 0xFF]";
inline const QString hexTooBig2 = "Hexadecimal constant is out of range [0x0000, 0xFFFF]";
inline const QString alignPow2 = ".ALIGN requires a decimal constant in (2, 4, 8).";

}; // namespace pas::errors::pepp
