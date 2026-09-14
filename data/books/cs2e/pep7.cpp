//  pep7.cpp
//  December 17, 2002

//  Version UNIX/7.2

//  Version history: 
//  UCSD/2.0  Written by John Rooker as an undergraduate computer science
//  project.         

//  October 19, 1987
//  UNIX/3.0  Ported by Stan Warford from Version UCSD/2.0
//  Modified eA_INDEXED addressing mode to eliminate the special case with
//  zero operand.         
//  UNIX/3.1 modified by Gerry St. Romain to correct bug in hex address
//  specification for Dump address.  Procedures <Parse> and <DecodeAddress>
//  were replaced.  Function <HexToDec> was deleted.

//  May 6, 1988
//  UNIX/4.0 modified by Stan Warford to change CHARI and CHARO to be byte
//  instructions instead of word instructions.  Also changed the identifiers
//  for the values of eRegSpecType and eAddrModeType enumerated types.
//  Changed the newline character from <CR> to <LF> on CHARI.  Changed the
//  reset and interrupt routines to implement a separate stack area for the
//  operating system.  Compatible with versions 4.x of assembler and 4.x of
//  operating system.

//  May 9, 1997
//  UNIX/6.0 modified by Stan Warford to change the instruction set from
//  Pep/5 to Pep/6.
//  Deleted ADDB, BRN, BRZ, BRNZ, SUBSP, and NOP and added BRLE,
//  BRLT, BREQ, BRNE, BRGE, and BRGT.  Switched eA_INDEXED and stack relative
//  addressing mode specifiers.  eA_INDEXED addressing is now defined as
//  Oprnd = Mem [B + X].  Instructions with eA_INDEXED addressing are now unary.
//  Compatible with versions 6.x of assembler and 6.x of operating system.

//  UNIX/7.0 modified by Stan Warford from Version UNIX/6.0.
//  Changed the file name types to be compatible with the GNU gpc compiler.

//  March 4, 2002
//  UNIX/7.1 translated from Pascal to C++ by Scott Mace as an undergraduate
//  project. Eliminated the nontext os ROM file, which is now simply the
//  text object output "os.o" of the asem7 translation of file "os".

//  December 17, 2002
//  UNIX/7.2 Increased file name length from 32 to 64. Changed usage.
//  Changed CHARO to allow the printing of an 8-bit character. Previous
//  versions masked out the most significant bit allowing only a 7-bit
//  ASCII character to be output. COMPr instruction now sets NZ correctly
//  even when the comparison subtraction overflows. Stan Warford

//  January 29, 2003
//  UNIX/7.3 Fixed an error message misspelling. Fixed a bug in an if
//  statement that tested with = instead of == when incrementing PC by two.
//  Stan Warford

//  February 11, 2003
//  UNIX/7.4 Fixed an error that prevented files from being reopened without
//  the clear() statement.
//  John Grogg
//
//  September 14, 2026
//  Adapted to use current C++ headers.
//  Matthew McRaven

#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

const int MEMORY_SIZE        = 32768;
const int TOP_OF_MEMORY      = 32767;
const int MAX_HEADER_LENGTH  = 40;    //Maximum length of ROM ID header
const int USER_SP            = 32760; //User stack pointer vector.
const int SYSTEM_SP          = 32762; //System stack pointer vector.
const int LOADER_PC          = 32764; //Program counter vector.
const int INTR_PC            = 32766; //Interrupt program counter vector.
const int FILE_NAME_LENGTH   = 64;
const int HEX_BYTE_LENGTH    = 2;
const int HEX_WORD_LENGTH    = 4;
const int UNARY_OPCODES      = 9;
const int BYTE_INSTRUCTION   = 4;
const int LINE_FEED          = 10;
const int CARRIAGE_RETURN    = 13;
const int LINE_LENGTH        = 128;   //Maximum length of a line of code
const int INTERRUPTS         = 3;     //Number of Interrupts
const int MNEMON_LENGTH      = 6;
const int LOW_BYTE           = 255;
const int PAGE_LINES         = 24;
const int HEX3               = 4096;
const int HEX2               = 256;
const int HEX                = 16;

//Enumerated Types
enum MnemonicOpcodes   //All possible opcodes
{
   eM_STOP, eM_LOADR, eM_STORER, eM_ADDR,  eM_SUBR, eM_ANDR, eM_ORR, eM_NOTR,
   eM_ASLR, eM_ASRR, eM_LDBYTR, eM_STBYTR, eM_LOADB, eM_ADDSP, eM_BR, eM_BRLE,
   eM_BRLT, eM_BREQ, eM_BRNE, eM_BRGE, eM_BRGT, eM_BRV, eM_BRC,  eM_COMPR,
   eM_JSR, eM_RTS, eM_RTI, eM_CHARI, eM_CHARO, eM_UNIMP0, eM_UNIMP1, eM_UNIMP2
};
enum eIntrupt { eI_INTRUPT0, eI_INTRUPT1, eI_INTRUPT2, eI_INTRUPT_END };
enum eRegSpecType  { eR_R_IS_ACCUMULATOR, eR_R_IS_INDEX_REG }; // 8 bits, unsigned
enum eAddrModeType { eA_IMMEDIATE, eA_DIRECT, eA_STACKREL, eA_INDEXED };
enum eTraceMd { eT_TR_OFF, eT_TR_PROGRAM, eT_TR_INTERRUPTS, eT_TR_LOADER };

//**** Global Records
struct sRegisterType                  // internal CPU registers
{
      int iHigh;                      // most significant byte
      int iLow;                       // least significant byte
};
struct sIRRecType
{
      int iOpcode;                    //  5 bits
      eRegSpecType eR_RegSpec;        //  1 bit
      eAddrModeType eA_AddrMode;      //  2 bits
      sRegisterType sR_OprndSpec;     // 16 bits
};

//**** Global Variables
char cHexTable[HEX];
eTraceMd eTraceMode;
int iMemory[MEMORY_SIZE];
int iRomStartAddr;
char IntrptMnemon[INTERRUPTS][MNEMON_LENGTH + 1];
int iUnaryOpcodes[UNARY_OPCODES];
int ByteInstructions[BYTE_INSTRUCTION];
char cCommand[LINE_LENGTH];
bool bLoading, bMachineReset;
//**** Constant registers
sRegisterType sR_AtZero, sR_One, sR_Two, sR_NegOne, sR_NegTwo, sR_NegThree;
ifstream in_file;
ofstream out_file;
char cInFileName[FILE_NAME_LENGTH];
char cOutFileName[FILE_NAME_LENGTH];
bool bInputSet, bOutputSet, bInputOpen, bOutputOpen, bError;
bool bChariInitialized = false;

//**** Keyboard buffer global variables for unbuffering the
//**** UNIX buffered line on interactive input
char cKeyBuffer[LINE_LENGTH];
int iKeyBufIndex;
int iKeyBufMax;
char cLine[LINE_LENGTH]; //Array of characters for a line of code
int iLineIndex; //Index of line array

//**** Pep/7 CPU registers
sRegisterType sR_Accumulator, sR_IndexRegister, sR_BaseRegister,
   sR_StackPointer, sR_ProgramCounter; // 16 bits
sIRRecType sIR_InstrRegister; // 24 bits
bool bStatusN, bStatusZ, bStatusV, bStatusC;

void vGetKeyboardChar (istream& input, char& ch)
{
   if (iKeyBufIndex == iKeyBufMax) //The cKeyBuffer is empty, so read a line from the keyboard
   {
      input.getline(cKeyBuffer, LINE_LENGTH);
      if (input.gcount() > 0)
      {
         iKeyBufMax = input.gcount();
      }
      else
      {
         iKeyBufMax = input.gcount() - 1;
      }
      iKeyBufIndex = 0;
   }
   ch = cKeyBuffer[iKeyBufIndex++];
}

//**** Stores the next line of assembly language code to be translated in global cLine[].
void vGetLine(istream& input)
{
   input.getline(cLine, LINE_LENGTH);
   if ((!input.eof ()) && (input.gcount() > 0))
   {
      cLine[input.gcount() - 1] = '\n';
   }
   else
   {
      cLine[input.gcount()] = '\n';
   }
   iLineIndex = 0;
}

//**** Gets the next character to be processed by vGetToken().
void vAdvanceInput (char& ch)
{
   ch = cLine[iLineIndex++];
}

//**** Backs up the input to the current character to be processed by vGetToken().
void vBackUpInput ()
{
   iLineIndex--;
}

bool bIsHexDigit (char cChar)
{
   return (((cChar >= 'A') && (cChar <= 'F')) ||
           ((cChar >= 'a') && (cChar <= 'f')) || isdigit(cChar));
}

bool bSearchIntArray (int iArray[], int iArrayLength, int iNum)
{
   int i = 0;
   iArray[iArrayLength] = iNum;
   while (iArray[i] != iNum)
   {
      i++;
   }
   if (i == iArrayLength)
   {
      return false;
   }
   else
   {
      return true;
   }
}

void PrntMnemon (ostream& output)
{
   switch (sIR_InstrRegister.iOpcode)
   {
      case eM_STOP  : output << "STOP  "; break;
      case eM_LOADR : output << "LOAD"; break;
      case eM_STORER: output << "STORE"; break;
      case eM_ADDR  : output << "ADD"; break;
      case eM_SUBR  : output << "SUB"; break;
      case eM_ANDR  : output << "AND"; break;
      case eM_ORR   : output << "OR"; break;
      case eM_NOTR  : output << "NOT"; break;
      case eM_ASLR  : output << "ASL"; break;
      case eM_ASRR  : output << "ASR"; break;
      case eM_LDBYTR: output << "LDBYT"; break;
      case eM_STBYTR: output << "STBYT"; break;
      case eM_LOADB : output << "LOADB "; break;
      case eM_ADDSP : output << "ADDSP "; break;
      case eM_BR    : output << "BR    "; break;
      case eM_BRLE  : output << "BRLE  "; break;
      case eM_BRLT  : output << "BRLT  "; break;
      case eM_BREQ  : output << "BREQ  "; break;
      case eM_BRNE  : output << "BRNE  "; break;
      case eM_BRGE  : output << "BRGE  "; break;
      case eM_BRGT  : output << "BRGT  "; break;
      case eM_BRV   : output << "BRV   "; break;
      case eM_BRC   : output << "BRC   "; break;
      case eM_COMPR : output << "COMP"; break;
      case eM_JSR   : output << "JSR   "; break;
      case eM_RTS   : output << "RTS   "; break;
      case eM_RTI   : output << "RTI   "; break;
      case eM_CHARI : output << "CHARI "; break;
      case eM_CHARO : output << "CHARO "; break;
      case eM_UNIMP0: output << IntrptMnemon[eI_INTRUPT0]; break;
      case eM_UNIMP1: output << IntrptMnemon[eI_INTRUPT1]; break;
      case eM_UNIMP2: output << IntrptMnemon[eI_INTRUPT2]; break;
   }
   if (((sIR_InstrRegister.iOpcode >= eM_LOADR) && (sIR_InstrRegister.iOpcode <= eM_STBYTR))
       || (sIR_InstrRegister.iOpcode == eM_COMPR))
   {
      switch (sIR_InstrRegister.eR_RegSpec)
      {
         case eR_R_IS_ACCUMULATOR: output << "A"; break;
         case eR_R_IS_INDEX_REG: output << "X"; break;
      }
   }
   if (sIR_InstrRegister.iOpcode == eM_ORR)                       //Column alignment adjustment
   {
      output << "   ";
   }
   else if (((sIR_InstrRegister.iOpcode >= eM_ADDR) && (sIR_InstrRegister.iOpcode <= eM_ANDR))
            || ((sIR_InstrRegister.iOpcode >= eM_NOTR) && (sIR_InstrRegister.iOpcode <= eM_ASRR)))
   {
      output << "  ";
   }
   else if ((sIR_InstrRegister.iOpcode == eM_LOADR) || (sIR_InstrRegister.iOpcode == eM_COMPR))
   {
      output << " ";
   }
}

//**** iOpcode procedures ****

//**** Adds 2 byte pairs and returns in result.  (One word adder)
void Adder (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result,
            bool& Carry, bool& Ovflw)
{
   int Temp;
   Temp = Op1.iLow + Op2.iLow;
   if (Temp > LOW_BYTE)
   {
      Result.iLow = Temp - LOW_BYTE - 1;
      Temp = Op1.iHigh + Op2.iHigh + 1;                 //Carry from low order
   }
   else
   {
      Result.iLow = Temp;
      Temp = Op1.iHigh + Op2.iHigh;
   }
   if (Temp > LOW_BYTE)
   {
      Result.iHigh = Temp - LOW_BYTE - 1;
      Carry = true;
   }
   else
   {
      Result.iHigh = Temp;
      Carry = false;
   }
   Ovflw = (((Op1.iHigh <= 127) && (Op2.iHigh <= 127) &&   // Pos/Pos/Neg
             (Result.iHigh > 127)) ||                       //     or
            ((Op1.iHigh >= 128) && (Op2.iHigh >= 128) &&   // Neg/Neg/Pos
             (Result.iHigh < 128)));
}

//**** Adds 2 byte pairs and returns in result.  (One word adder)
//**** Same as Adder except carry and overflow are not detected.
void FastAdder (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result)
{
   int iTemp;
   iTemp = Op1.iLow + Op2.iLow;
   if (iTemp > LOW_BYTE)
   {
      Result.iLow = iTemp - LOW_BYTE - 1;
      iTemp = Op1.iHigh + Op2.iHigh + 1;                 // Carry from low order
   }
   else
   {
      Result.iLow = iTemp;
      iTemp = Op1.iHigh + Op2.iHigh;
   }
   if (iTemp > LOW_BYTE)
   {
      Result.iHigh = iTemp - LOW_BYTE - 1;
   }
   else
   {
      Result.iHigh = iTemp;
   }
}

//**** Subtracts Op2 from Op1 and returns in result.  (One word)
void Subtractor (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result,
                 bool& Carry, bool& Ovflw)
{
   int iTemp;
   iTemp = Op1.iLow - Op2.iLow;
   if (iTemp < 0)
   {
      Result.iLow = iTemp + LOW_BYTE + 1;
      iTemp = Op1.iHigh - Op2.iHigh - 1;               // Borrow from high order
   }
   else
   {
      Result.iLow = iTemp;
      iTemp = Op1.iHigh - Op2.iHigh;
   }
   if (iTemp < 0)
   {
      Result.iHigh = iTemp + LOW_BYTE + 1;
      Carry = true;
   }
   else
   {
      Result.iHigh = iTemp;
      Carry = false;
   }
   Ovflw = (((Op1.iHigh <= 127) && (Op2.iHigh >= 128) &&   // Pos/Neg/Neg
             (Result.iHigh > 127)) ||                       //     or
            ((Op1.iHigh >= 128) && (Op2.iHigh <= 127) &&   // Neg/Pos/Pos
             (Result.iHigh < 128)));
}

//**** Determine operand based on addressing mode in instr. register
void AddrProcessor (sRegisterType& Operand)
{
   switch (sIR_InstrRegister.eA_AddrMode)
   {
      case eA_IMMEDIATE : Operand = sIR_InstrRegister.sR_OprndSpec; break;
      case eA_DIRECT    : Operand = sIR_InstrRegister.sR_OprndSpec; break;
      case eA_STACKREL  : FastAdder (sR_StackPointer, sIR_InstrRegister.sR_OprndSpec, Operand); break;
      case eA_INDEXED   : FastAdder (sR_IndexRegister, sR_BaseRegister, Operand); break;
   }
}

//**** Reads one word:  Rslt.iHigh = Mem [Loc], Rslt.iLow = Mem [Loc + 1]
void MemRead (sRegisterType Loc, sRegisterType& Rslt)
{
   int iTemp;
   if (Loc.iHigh < MEMORY_SIZE / (LOW_BYTE + 1))
   {
      iTemp = Loc.iHigh * (LOW_BYTE + 1) + Loc.iLow;
      Rslt.iHigh = iMemory[iTemp];
      if (iTemp < TOP_OF_MEMORY)
      {
         Rslt.iLow = iMemory[iTemp + 1];
      }
      else
      {
         Rslt.iLow = 0;
      }
   }
   else
   {
      Rslt.iHigh = 0;
      Rslt.iLow = 0;
   }
}

//**** Reads one byte from Mem [Loc] and returns in Byte
void MemByteRead (sRegisterType Loc, int& iByte)
{
   int iTemp;
   if (Loc.iHigh < MEMORY_SIZE / (LOW_BYTE + 1))
   {
      iTemp = Loc.iHigh * (LOW_BYTE + 1) + Loc.iLow;
      iByte = iMemory[iTemp];
   }
   else
   {
      iByte = 0;
   }
}

//**** Writes one word:  Reg.iHigh to Mem [Loc] and Reg.iLow to Mem[Loc + 1]
void MemWrite (sRegisterType Reg, sRegisterType Loc)
{
   int iTemp;
   if (Loc.iHigh < MEMORY_SIZE / (LOW_BYTE + 1))
   {
      iTemp = Loc.iHigh * (LOW_BYTE + 1) + Loc.iLow;
      if (iTemp < iRomStartAddr)
      {
         iMemory[iTemp] = Reg.iHigh;
      }
      if (iTemp < iRomStartAddr - 1)
      {
         iMemory[iTemp + 1] = Reg.iLow;
      }
   }
}

//**** Writes one byte to Mem [Loc]
void MemByteWrite (int iByte, sRegisterType Loc)
{
   int iTemp;
   if (Loc.iHigh < MEMORY_SIZE / (LOW_BYTE + 1))
   {
      iTemp = Loc.iHigh * (LOW_BYTE + 1) + Loc.iLow;
      if (iTemp < iRomStartAddr)
      {
         iMemory[iTemp] = iByte;
      }
   }
}

void LoadReg (sRegisterType& Reg)
{
   sRegisterType Operand;
   AddrProcessor (Operand);
   if (sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE)
   {
      Reg = Operand;
   }
   else
   {
      MemRead (Operand, Reg);
   }
}

void SetNZBits (sRegisterType Reg)
{
   bStatusN = (Reg.iHigh > 127);
   bStatusZ = ((Reg.iHigh == 0) && (Reg.iLow == 0));
}

//**** Prints program counter value of instruction that caused machine
//**** error.  Message is 20 characters long.
void PrntRunLoc()
{
   sRegisterType LastLoc;
   if (bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, sIR_InstrRegister.iOpcode))
   {  // undo increment step
      FastAdder (sR_ProgramCounter, sR_NegOne, LastLoc);
   }
   else
   {
      FastAdder (sR_ProgramCounter, sR_NegThree, LastLoc);
   }
   cout << "Runtime error at " << cHexTable[LastLoc.iHigh / HEX] <<
      cHexTable[LastLoc.iHigh % HEX] << cHexTable[LastLoc.iLow / HEX] <<
      cHexTable[LastLoc.iLow % HEX] << ":  ";
}

void IllegalAddr (bool& bError)
{
   bError = true;
   PrntRunLoc();
   cout << "Illegal addressing mode ";
   switch (sIR_InstrRegister.eA_AddrMode)
   {
      case eA_IMMEDIATE : cout << "immediate "; break;
      case eA_DIRECT    : cout << "direct "; break;
      case eA_STACKREL  : cout << "stack relative "; break;
      case eA_INDEXED   : cout << "indexed "; break;
   }
   cout << "with ";
   PrntMnemon (cout);
   cout << endl;
}

void SimSTOP (bool& Halt)
{
   Halt = true;
}

void SimLOADR()
{
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         LoadReg (sR_Accumulator);
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         LoadReg (sR_IndexRegister);
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void SimSTORER (bool& bError)
{
   sRegisterType Operand;
   if (sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE)
   {
      IllegalAddr (bError);
   }
   else  // addressing mode is valid
   {
      AddrProcessor (Operand);
      switch (sIR_InstrRegister.eR_RegSpec)
      {
         case eR_R_IS_ACCUMULATOR:
            MemWrite (sR_Accumulator, Operand);
            break;
         case eR_R_IS_INDEX_REG:
            MemWrite (sR_IndexRegister, Operand);
            break;
      }
   }
}

void SimADDR()
{
   sRegisterType R0;
   LoadReg (R0);
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         Adder (sR_Accumulator, R0, sR_Accumulator, bStatusC, bStatusV);
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         Adder (sR_IndexRegister, R0, sR_IndexRegister, bStatusC, bStatusV);
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void ANDReg (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result)
{
   int iPwr;
   iPwr = 1;
   Result.iHigh = 0;
   Result.iLow = 0;
   while (((Op1.iHigh != 0) && (Op2.iHigh != 0)) ||         // while more to do
          ((Op1.iLow != 0) && (Op2.iLow != 0)))
   {
      if ((Op1.iHigh % 2 != 0) && (Op2.iHigh % 2 != 0))
      {
         Result.iHigh = Result.iHigh + iPwr;
      }
      if ((Op1.iLow % 2 != 0) && (Op2.iLow % 2 != 0))
      {
         Result.iLow = Result.iLow + iPwr;
      }
      Op1.iHigh = Op1.iHigh / 2;
      Op1.iLow = Op1.iLow / 2;
      Op2.iHigh = Op2.iHigh / 2;
      Op2.iLow = Op2.iLow / 2;
      iPwr = iPwr + iPwr;
   }
}

void SimANDR()
{
   sRegisterType R0;
   LoadReg (R0);
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         ANDReg (sR_Accumulator, R0, sR_Accumulator);
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         ANDReg (sR_IndexRegister, R0, sR_IndexRegister);
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void SimSUBR()
{
   sRegisterType R0;
   LoadReg (R0);
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         Subtractor (sR_Accumulator, R0, sR_Accumulator, bStatusC, bStatusV);
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         Subtractor (sR_IndexRegister, R0, sR_IndexRegister, bStatusC, bStatusV);
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void ORReg (sRegisterType Op1, sRegisterType Op2, sRegisterType& Result)
{
   int Pwr;
   Pwr = 1;
   Result.iHigh = 0;
   Result.iLow = 0;
   while (((Op1.iHigh != 0) || (Op2.iHigh != 0)) ||          // while more to do
          ((Op1.iLow != 0) || (Op2.iLow != 0)))
   {
      if ((Op1.iHigh % 2 != 0) || (Op2.iHigh % 2 != 0))
      {
         Result.iHigh = Result.iHigh + Pwr;
      }
      if ((Op1.iLow % 2 != 0) || (Op2.iLow % 2 != 0))
      {
         Result.iLow = Result.iLow + Pwr;
      }
      Op1.iHigh = Op1.iHigh / 2;
      Op1.iLow = Op1.iLow / 2;
      Op2.iHigh = Op2.iHigh / 2;
      Op2.iLow = Op2.iLow / 2;
      Pwr = Pwr + Pwr;
   }
}

void SimORR()
{  // SimORR
   sRegisterType R0;
   LoadReg (R0);
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         ORReg (sR_Accumulator, R0, sR_Accumulator);
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         ORReg (sR_IndexRegister, R0, sR_IndexRegister);
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void SimNOTR()
{
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         sR_Accumulator.iHigh = LOW_BYTE - sR_Accumulator.iHigh;
         sR_Accumulator.iLow = LOW_BYTE - sR_Accumulator.iLow;
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         sR_IndexRegister.iHigh = LOW_BYTE - sR_IndexRegister.iHigh;
         sR_IndexRegister.iLow = LOW_BYTE - sR_IndexRegister.iLow;
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void SimASLR()
{
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         Adder (sR_Accumulator, sR_Accumulator, sR_Accumulator, bStatusC, bStatusV);
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         Adder (sR_IndexRegister, sR_IndexRegister, sR_IndexRegister, bStatusC, bStatusV);
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void SimASRR()
{
   int Sign, Carry;
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         Sign = sR_Accumulator.iHigh / 128;
         Carry = sR_Accumulator.iHigh % 2;
         bStatusC = (sR_Accumulator.iLow % 2 != 0);
         sR_Accumulator.iHigh = (sR_Accumulator.iHigh / 2) + (Sign * 128);
         sR_Accumulator.iLow = (sR_Accumulator.iLow / 2) + (Carry * 128);
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         Sign = sR_IndexRegister.iHigh / 128;
         Carry = sR_IndexRegister.iHigh % 2;
         bStatusC = (sR_IndexRegister.iLow % 2 != 0);
         sR_IndexRegister.iHigh = (sR_IndexRegister.iHigh / 2) + (Sign * 128);
         sR_IndexRegister.iLow = (sR_IndexRegister.iLow / 2) + (Carry * 128);
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void SimLDBYTR()
{
   int Temp;
   sRegisterType Operand;
   AddrProcessor (Operand);
   if (sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE)
   {
      Temp = Operand.iLow;
   }
   else
   {
      MemByteRead (Operand, Temp);
   }
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         sR_Accumulator.iLow = Temp;
         SetNZBits (sR_Accumulator);
         break;
      case eR_R_IS_INDEX_REG:
         sR_IndexRegister.iLow = Temp;
         SetNZBits (sR_IndexRegister);
         break;
   }
}

void SimSTBYTR (bool& bError)
{
   sRegisterType Operand;
   if (sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE)
   {
      IllegalAddr (bError);
   }
   else  // addressing mode is valid
   {
      AddrProcessor (Operand);
      switch (sIR_InstrRegister.eR_RegSpec)
      {
         case eR_R_IS_ACCUMULATOR:
            MemByteWrite (sR_Accumulator.iLow, Operand); break;
         case eR_R_IS_INDEX_REG:
            MemByteWrite (sR_IndexRegister.iLow, Operand); break;
      }
   }
}

void SimLOADB()
{
   LoadReg (sR_BaseRegister);
   SetNZBits (sR_BaseRegister);
}

void SimADDSP (bool& bError)
{
   if (sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE)
   {
      Adder (sR_StackPointer, sIR_InstrRegister.sR_OprndSpec, sR_StackPointer, bStatusC, bStatusV);
      SetNZBits (sR_StackPointer);
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBR (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      LoadReg (sR_ProgramCounter);
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBRLE (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (bStatusN || bStatusZ)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBRLT (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (bStatusN)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBREQ (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (bStatusZ)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBRNE (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (!bStatusZ)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBRGE (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (!bStatusN)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBRGT (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) || (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (!bStatusN && !bStatusZ)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBRV (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (bStatusV)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimBRC (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) ||
       (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      if (bStatusC)
      {
         LoadReg (sR_ProgramCounter);
      }
   }
   else
   {
      IllegalAddr (bError);
   }
}

//**** Same as SimSUBR except sR_Accumulator or index reg are not changed.
void SimCOMPR()
{
   sRegisterType R0, R1, R2;
   switch (sIR_InstrRegister.eR_RegSpec)
   {
      case eR_R_IS_ACCUMULATOR:
         R0 = sR_Accumulator; break;
      case eR_R_IS_INDEX_REG:
         R0 = sR_IndexRegister; break;
   }
   LoadReg (R1);
   Subtractor (R0, R1, R2, bStatusC, bStatusV);
   if ((R0.iHigh <= 127) && (R1.iHigh >= 128)) //Pos minus Neg
   {
      bStatusN = false;
      bStatusZ = false;
   }
   else if ((R0.iHigh >= 128) & (R1.iHigh <= 127)) //Neg minus Pos
   {
      bStatusN = true;
      bStatusZ = false;
   }
   else
   {
      SetNZBits (R2);
   }
}

void SimJSR (bool& bError)
{
   if ((sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE) || (sIR_InstrRegister.eA_AddrMode == eA_INDEXED))
   {
      FastAdder (sR_StackPointer, sR_NegTwo, sR_StackPointer);
      MemWrite (sR_ProgramCounter, sR_StackPointer);     // Mem [SP] = PC
      LoadReg (sR_ProgramCounter);
   }
   else
   {
      IllegalAddr (bError);
   }
}

void SimRTS()
{
   MemRead (sR_StackPointer, sR_ProgramCounter);         // PC = Mem [SP]
   FastAdder (sR_StackPointer, sR_Two, sR_StackPointer); // SP = SP + 2
}

void Pop (sRegisterType& Reg, sRegisterType Size)
{
   MemRead (sR_StackPointer, Reg);
   FastAdder (sR_StackPointer, Size, sR_StackPointer);
}

void SimRTI()
{  // SimRTI
   sRegisterType R0;
   int Flags;
   Pop (R0, sR_One);                                     // get status flags
   Flags = R0.iHigh % HEX;
   bStatusC = (Flags % 2 != 0);
   bStatusV = ((Flags == 2) || (Flags == 3) || (Flags == 6) ||
               (Flags == 7) || (Flags == 10) || (Flags == 11) ||
               (Flags == 14) || (Flags == 15));
   bStatusZ = (((Flags >= 4) && (Flags <= 7)) || ((Flags >= 12) && (Flags <= 15)));
   bStatusN = (Flags >= 8);
   Pop (sR_Accumulator, sR_Two);
   Pop (sR_IndexRegister, sR_Two);
   Pop (sR_BaseRegister, sR_Two);
   Pop (sR_ProgramCounter, sR_Two);
   MemRead(sR_StackPointer, sR_StackPointer); //Pop
}

void SimCHARI (bool& bError)
{
   char Ch;
   char FileName[FILE_NAME_LENGTH];
   int iTemp;
   sRegisterType Operand;
   bool ResetError = false;
   if (bLoading && !bInputOpen)
   {
      cout << "Enter object file name (do not include .o): ";
      cin.getline(FileName, FILE_NAME_LENGTH);
      iTemp = cin.gcount() - 1;
      FileName[iTemp++] = '.';
      FileName[iTemp++] = 'o';
      FileName[iTemp] = '\0';
      in_file.close();
      in_file.clear();
      in_file.open(FileName);
      if (in_file.fail())
      {
         ResetError = true;
      }
      else
      {
         ResetError = false;
         vGetLine(in_file);
      }
      if (ResetError)
      {
         bError = true;
         cout << "Could not open object file " << FileName << endl;
      }
      else
      {
         bInputOpen = true;
         bMachineReset = true;
      }
   }
   if (bInputSet && !bInputOpen && !bError)
   {
      in_file.close();
      in_file.clear();
      in_file.open(cInFileName);
      if (in_file.fail())
      {
         ResetError = true;
      }
      else
      {
         ResetError = false;
         bInputOpen = true;
      }
      if (ResetError)
      {
         bError = true;
         cout << "Could not open input data file " << cInFileName << endl;
      }
   }
   if (bInputOpen && !bError)
   {
      if (bLoading)
      {
         vAdvanceInput(Ch);
      }
      else
      {
         vGetKeyboardChar(in_file, Ch);
      }
      if (Ch == '\n')
      {
         vGetLine(in_file);
         if (in_file.eof())
         {
            bError = true;
            PrntRunLoc();
            cout << "File read error or read past end of file." << endl;
         }
         else
         {
            Ch = '\n';
         }
      }
      else
      {
         //vGetKeyboardChar(in_file, Ch); //Debug
         if (in_file.eof())
         {
            bError = true;
            PrntRunLoc();
            cout << "File read error or read past end of file." << endl;
         }
      }
   }
   else if (!bError)
   {
      vGetKeyboardChar(cin, Ch);
   }
   if (!bError)
   {
      if (sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE)
      {
         IllegalAddr (bError);
      }
      else  //Addressing mode is valid
      {
         AddrProcessor (Operand);
         MemByteWrite (Ch, Operand);
      }
   }
   //iTemp = static_cast <int> (Ch); //Debug
   //cout << iTemp << endl; //Debug
}

void SimCHARO (bool& bError)
{
   sRegisterType Operand;
   int iData;
   bool RewriteError;
   if (eTraceMode == eT_TR_OFF)
   {
      if (bOutputSet && !bOutputOpen)
      {
         out_file.open (cOutFileName);
         if (out_file.fail())
         {
            RewriteError = true;
         }
         else
         {
            RewriteError = false;
         }
         if (RewriteError)
         {
            bError = true;
            cout << "Could not open output file " << cOutFileName << "." << endl;
         }
         else
         {
            bOutputOpen = true;
         }
      }
      if (!bError)
      {
         AddrProcessor (Operand);
         if (sIR_InstrRegister.eA_AddrMode == eA_IMMEDIATE)
         {
            iData = Operand.iLow;
         }
         else
         {
            MemByteRead (Operand, iData);
         }
         if (bOutputOpen)
         {
            if (iData == LINE_FEED || iData == CARRIAGE_RETURN)
            {
               out_file << endl;
            }
            else
            {
               out_file << static_cast <char> (iData);
            }
         }
         else
         {
            if (iData == LINE_FEED || iData == CARRIAGE_RETURN)
            {
               cout << endl;
            }
            else
            {
               cout << static_cast <char> (iData);
            }
         }
      }
   }
}

void Push (sRegisterType Reg)
{
   FastAdder (sR_StackPointer, sR_NegTwo, sR_StackPointer);
   MemWrite (Reg, sR_StackPointer);
}

void SimINTR()
{
   sRegisterType R0;
   sRegisterType TempSP;
   TempSP = sR_StackPointer;          // Save initial SP value to push later
   sR_StackPointer.iHigh = iMemory[SYSTEM_SP];    // Get system SP value
   sR_StackPointer.iLow  = iMemory[SYSTEM_SP + 1]; // Transfer instr spec to R0
   R0.iLow = sIR_InstrRegister.iOpcode * 8;
   if (sIR_InstrRegister.eR_RegSpec == eR_R_IS_INDEX_REG)
   {
      R0.iLow = R0.iLow + 4;
   }
   switch (sIR_InstrRegister.eA_AddrMode)
   {
      case eA_IMMEDIATE : R0.iLow = R0.iLow + 0; break;
      case eA_DIRECT    : R0.iLow = R0.iLow + 1; break;
      case eA_STACKREL  : R0.iLow = R0.iLow + 2; break;
      case eA_INDEXED   : R0.iLow = R0.iLow + 3; break;
   };
   FastAdder (sR_StackPointer, sR_NegOne, sR_StackPointer);
   MemByteWrite (R0.iLow, sR_StackPointer);      // Push instruction specifier
   Push (TempSP);
   Push (sR_ProgramCounter);
   Push (sR_BaseRegister);
   Push (sR_IndexRegister);
   Push (sR_Accumulator);
   if (bStatusN)
   {
      R0.iLow = 8;
   }
   else
   {
      R0.iLow = 0;
   }
   if (bStatusZ)
   {
      R0.iLow = R0.iLow + 4;
   }
   if (bStatusV)
   {
      R0.iLow = R0.iLow + 2;
   }
   if (bStatusC)
   {
      R0.iLow = R0.iLow + 1;
   }
   FastAdder (sR_StackPointer, sR_NegOne, sR_StackPointer);
   MemByteWrite (R0.iLow, sR_StackPointer);               // Push status flags
   sR_ProgramCounter.iHigh = iMemory[INTR_PC];            // Branch to Pep/7 OS
   sR_ProgramCounter.iLow = iMemory[INTR_PC + 1];
}
//**** End of Opcode procedures ****

void Initialize (bool& bError)
{
   int j;
   char Ch;
   ifstream mnemonFile;
   char cMnemon[MNEMON_LENGTH + 1];
   eIntrupt Intrpt;
   bool ResetError;
   bError = false;
   mnemonFile.open("mnemon");
   if (mnemonFile.fail())
   {
      bError = true;
      cout << "Could not open file mnemon" << endl;
   }
   else
   {
      Intrpt = eI_INTRUPT0;
      do
      {
         vGetLine(mnemonFile);
         j = 0;
         vAdvanceInput(Ch);                 // Skip U/N ident.
         vAdvanceInput(Ch);                 // Skip blank space
         vAdvanceInput(Ch);
         while (( Ch != ' ') && (Ch != '\n') && (j < MNEMON_LENGTH) && !bError)
         {
            cMnemon[j++] = toupper (Ch);
            vAdvanceInput(Ch);
         }
         while (j < MNEMON_LENGTH)
         {
            cMnemon[j++] = ' ';
         }
         if (j <= MNEMON_LENGTH)
         {
            cMnemon[j] = '\0';
         }
         if (!bError)
         {
            strncpy(IntrptMnemon[Intrpt], cMnemon, MNEMON_LENGTH + 1);
            Intrpt = eIntrupt (Intrpt + 1);
         }
      }
      while ((Intrpt != eI_INTRUPT_END) && !bError);
      if (!bError)
      {
         eTraceMode = eT_TR_OFF;
         bLoading = false;
         bMachineReset = false;
         bInputSet = false;
         bInputOpen = false;
         bOutputSet = false;
         bOutputOpen = false;
         cHexTable[0] = '0';
         cHexTable[1] = '1';
         cHexTable[2] = '2';
         cHexTable[3] = '3';
         cHexTable[4] = '4';
         cHexTable[5] = '5';
         cHexTable[6] = '6';
         cHexTable[7] = '7';
         cHexTable[8] = '8';
         cHexTable[9] = '9';
         cHexTable[10] = 'A';
         cHexTable[11] = 'B';
         cHexTable[12] = 'C';
         cHexTable[13] = 'D';
         cHexTable[14] = 'E';
         cHexTable[15] = 'F';
         iUnaryOpcodes[0] = eM_STOP;
         iUnaryOpcodes[1] = eM_NOTR;
         iUnaryOpcodes[2] = eM_ASLR;
         iUnaryOpcodes[3] = eM_ASRR;
         iUnaryOpcodes[4] = eM_RTS;
         iUnaryOpcodes[5] = eM_RTI;
         iUnaryOpcodes[6] = eM_UNIMP0;
         iUnaryOpcodes[7] = eM_UNIMP1;
         iUnaryOpcodes[8] = eM_UNIMP2;
         ByteInstructions[0] = eM_LDBYTR;
         ByteInstructions[1] = eM_STBYTR;
         ByteInstructions[2] = eM_CHARI;
         ByteInstructions[3] = eM_CHARO;
         sR_AtZero.iHigh = 0;
         sR_AtZero.iLow = 0;
         sR_One.iHigh = 0;
         sR_One.iLow = 1;
         sR_Two.iHigh = 0;
         sR_Two.iLow = 2;
         sR_NegOne.iHigh = LOW_BYTE;
         sR_NegOne.iLow = LOW_BYTE;
         sR_NegTwo.iHigh = LOW_BYTE;
         sR_NegTwo.iLow = LOW_BYTE - 1;
         sR_NegThree.iHigh = LOW_BYTE;
         sR_NegThree.iLow = LOW_BYTE - 2;
         sR_Accumulator = sR_AtZero;           // Must be initialized if trace used
         sR_IndexRegister = sR_AtZero;
         sR_BaseRegister = sR_AtZero;
      }
      mnemonFile.close();
      mnemonFile.clear();
   }
}

//**** Converts a HEX number to a decimal number and returns the decimal number.
int iHexToDec (char ch)
{
   switch (ch)
   {
      case '0':  return 0; break;
      case '1':  return 1; break;
      case '2':  return 2; break;
      case '3':  return 3; break;
      case '4':  return 4; break;
      case '5':  return 5; break;
      case '6':  return 6; break;
      case '7':  return 7; break;
      case '8':  return 8; break;
      case '9':  return 9; break;
      case 'A':  return 10; break;
      case 'B':  return 11; break;
      case 'C':  return 12; break;
      case 'D':  return 13; break;
      case 'E':  return 14; break;
      case 'F':  return 15; break;
   }
}

//**** Converts a decimal value between -256 to 255 to a HEX array of characters
//**** Used to convert opcodes to hex
void vDecToHexByte (int iDec, char cHex[HEX_BYTE_LENGTH + 1])
{
   cHex[0] = cHexTable[iDec / HEX];
   cHex[1] = cHexTable[iDec % HEX];
   cHex[2] = '\0';
}

//**** Converts a HEX byte to a positive decimal integer
int iHexByteToDecInt (char cHex[HEX_BYTE_LENGTH + 1])
{
   return HEX * iHexToDec(cHex[0]) + iHexToDec(cHex[1]);
}

//**** Converts a 16 bit register into a 4 digit HEX no.
void RegToHex (sRegisterType Reg, char HexNum[])
{
   HexNum[0] = cHexTable[Reg.iHigh / HEX];
   HexNum[1] = cHexTable[Reg.iHigh % HEX];
   HexNum[2] = cHexTable[Reg.iLow / HEX];
   HexNum[3] = cHexTable[Reg.iLow % HEX];
   HexNum[4] = '\0';
}

//**** Initialize RAM using data from os.o file
void InstallRom (bool& bError)
{
   ifstream ROMFile;
   int iNumBytes = 0;
   char cByte[HEX_BYTE_LENGTH + 1];
   bool bResetError;
   int iCounter = 0;
   char cNext;
   ROMFile.open("os.o");
   if (ROMFile.fail())
   {
      bError = true;
      cout << "Could not open file os.o" << endl;
   }
   else
   {
      iCounter = 0;
      bool bEnd = false;
      int i;
      vGetLine(ROMFile);
      vAdvanceInput(cNext);
      while (!ROMFile.eof())
      {
         if (bIsHexDigit(cNext))
         {
            iCounter++;
         }
         else if (cNext == '\n')
         {
            vGetLine(ROMFile);
         }
         vAdvanceInput(cNext);
      }
      iNumBytes = iCounter / 2;
      ROMFile.close();
      ROMFile.clear();
      if (iNumBytes >= MEMORY_SIZE)
      {
         bError = true;
         cout << "OS is too big to fit into main memory." << endl;
         cout << "NumBytes = " << iNumBytes;
         cout << ", MemorySize = " << MEMORY_SIZE << endl;
      }
      else
      {
         ROMFile.open ("os.o");
         iRomStartAddr = TOP_OF_MEMORY - iNumBytes + 1;
         bool bIsEnd = false;
         vGetLine(ROMFile);
         vAdvanceInput(cNext);
         iCounter = 0;
         i = iRomStartAddr;
         while (!ROMFile.eof() && !bIsEnd)
         {
            if (iCounter == 2)
            {
               cByte[iCounter] = '\0';
               iMemory[i++] = iHexByteToDecInt(cByte);
               iCounter = 0;
               vBackUpInput();
            }
            else if (bIsHexDigit(cNext))
            {
               cByte[iCounter++] = cNext;
            }
            else if (cNext == '\n')
            {
               vGetLine(ROMFile);
            }
            else if (cNext == 'z')
            {
               bIsEnd = true;
            }
            else if (cNext != ' ')
            {
               cout << "Invalid input in os.o" << endl;
               return;
            }
            vAdvanceInput(cNext);
         }
         if (bIsEnd)
         {
            if (cNext != 'z')
            {
               cout << "File must end in 'zz'" << endl;
            }
         }
         ROMFile.close();
	 ROMFile.clear();
         cout << iRomStartAddr << " bytes RAM free." << endl;
      }
   }
}

void PrintLine (ostream& output)
{
   output << "--------------------------------------------------";
   output << "----------------------------" << endl;
}

void PrintHeading (ostream& output)
{
   PrintLine (output);
   output << "              Oprnd     Instr           Index  ";
   output << "Base   Stack   Status" << endl;
   output << "Addr  Mnemon  Spec       Reg     Accum   Reg   ";
   output << "Reg   Pointer  N Z V C  Operand" << endl;
   PrintLine (output);
}

void PrintDataLine (ostream& output, sRegisterType Address, bool SingleStep)
{
   char cHexByte[HEX_BYTE_LENGTH + 1];
   char cHexWord[HEX_WORD_LENGTH + 1];
   sRegisterType R0;
   int iTemp;
   RegToHex (Address, cHexWord);
   output << cHexWord << "  ";           // Print address
   PrntMnemon (output);                  // Print mnemonic
   if (bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, sIR_InstrRegister.iOpcode))
   {
      output << "        ";
   }
   else
   {
      switch (sIR_InstrRegister.eA_AddrMode)
      {
         case eA_IMMEDIATE :
            RegToHex (sIR_InstrRegister.sR_OprndSpec, cHexWord);
            output << "  " << cHexWord << ",";
            output << "i";
            break;
         case eA_DIRECT    :
            RegToHex (sIR_InstrRegister.sR_OprndSpec, cHexWord);
            output << "  " << cHexWord << ",";
            output << "d";
            break;
         case eA_STACKREL  :
            RegToHex (sIR_InstrRegister.sR_OprndSpec, cHexWord);
            output << "  " << cHexWord << ",";
            output << "s";
            break;
         case eA_INDEXED   :
            output << "      ,x"; break;
      }
   }
   iTemp = sIR_InstrRegister.iOpcode * 8;
   if (sIR_InstrRegister.eR_RegSpec == eR_R_IS_INDEX_REG)
   {
      iTemp = iTemp + 4;
   }
   switch (sIR_InstrRegister.eA_AddrMode)
   {
      case eA_IMMEDIATE : break;
      case eA_DIRECT    : iTemp++; break;
      case eA_STACKREL  : iTemp = iTemp + 2; break;
      case eA_INDEXED   : iTemp = iTemp + 3; break;
   }
   vDecToHexByte (iTemp, cHexByte);
   output << "    " << cHexByte;       // Print instruction spec.
   RegToHex (sIR_InstrRegister.sR_OprndSpec, cHexWord);
   output << cHexWord << "   ";        // Print operand specifier
   RegToHex (sR_Accumulator, cHexWord);
   output << cHexWord << "   ";           // Print accumulator
   RegToHex (sR_IndexRegister, cHexWord);
   output << cHexWord << "   ";           // Print index register
   RegToHex (sR_BaseRegister, cHexWord);
   output << cHexWord << "   ";           // Print base register
   RegToHex (sR_StackPointer, cHexWord);
   output << cHexWord << "    ";          // Print stack pointer
   if (bStatusN)
   {
      output << "1 ";                     // Print status flags
   }
   else
   {
      output << "0 ";
   }
   if (bStatusZ)
   {
      output << "1 ";
   }
   else
   {
      output << "0 ";
   }
   if (bStatusV)
   {
      output << "1 ";
   }
   else
   {
      output << "0 ";
   }
   if (bStatusC)
   {
      output << "1   ";
   }
   else
   {
      output << "0   ";
   }
   if (bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, sIR_InstrRegister.iOpcode))
   {
      for (int i = 0; i < HEX_WORD_LENGTH; i++)
      {
         cHexWord[i] = '0';
      }
   }
   else
   {
      LoadReg (R0);                       // calculate operand
      RegToHex (R0, cHexWord);
   }
   if (SingleStep)
   {
      output << cHexWord;                 // Print iMemory [Oprnd Spec]
   }
   else
   {
      output << cHexWord << endl;
   }
}

void Trace (sRegisterType Address, int& LineCount, char cResponse[], bool& SingleStep)
{
   int iTempAddr;
   char ch = toupper(cResponse[0]);
   if (Address.iHigh < MEMORY_SIZE / (LOW_BYTE + 1))
   {
      iTempAddr = Address.iHigh * (LOW_BYTE + 1) + Address.iLow;
   }
   else
   {
      iTempAddr = TOP_OF_MEMORY;
   }
   if (iTempAddr < iRomStartAddr
      || (iTempAddr >= iRomStartAddr && eTraceMode == eT_TR_INTERRUPTS)
      || eTraceMode == eT_TR_LOADER)
   {
      if (bOutputOpen)
      {
         PrintDataLine (out_file, Address, SingleStep);
      }
      else
      {
         PrintDataLine (cout, Address, SingleStep);
      }
      if (ch != 'S')
      {
         if (!SingleStep)
         {
            LineCount++;
         }
         if (LineCount == 22 || SingleStep)
         {
            do
            {
               if (!SingleStep)
               {
                  cout << endl;
                  cout << "(n)ext page  s(c)roll  (s)ingle step  (q)uit trace: ";
               }
               cin.getline(cResponse, LINE_LENGTH);
               ch = toupper(cResponse[0]);
               if (ch != 'N' && ch != 'S' && ch != 'Q' && ch != ' ')
               {
                  cout << "Invalid response" << endl;
               }
            }
            while (ch != 'N' && ch != 'S' && ch != 'Q' && ch != ' ');
            SingleStep = (ch == ' ');
            if (!SingleStep)
            {
               cout << endl;
            }
            if (ch == 'N')
            {
               PrintHeading (cout);
            }
            LineCount = 4;
         }
      }
   }
}

//****  The von Neumann execution cycle
void FetchIncrPC()
{
   sRegisterType R0;
   //**** Fetch instruction spec.
   MemByteRead (sR_ProgramCounter, R0.iHigh);  // R0.iHigh = Instruction spec.
   sIR_InstrRegister.iOpcode = R0.iHigh / 8;
   if ((R0.iHigh % 8 - R0.iHigh % 4) == 0)
   {
      sIR_InstrRegister.eR_RegSpec = eR_R_IS_ACCUMULATOR;
   }
   else
   {
      sIR_InstrRegister.eR_RegSpec = eR_R_IS_INDEX_REG;
   }
   switch (R0.iHigh % 4)
   {
      case 0: sIR_InstrRegister.eA_AddrMode = eA_IMMEDIATE; break;
      case 1: sIR_InstrRegister.eA_AddrMode = eA_DIRECT; break;
      case 2: sIR_InstrRegister.eA_AddrMode = eA_STACKREL; break;
      case 3: sIR_InstrRegister.eA_AddrMode = eA_INDEXED; break;
   }
   //**** Increment ProgramCounter by one
   if (sR_ProgramCounter.iLow == LOW_BYTE)
   {
      sR_ProgramCounter.iLow = 0;
      if (sR_ProgramCounter.iHigh == LOW_BYTE)
      {
         sR_ProgramCounter.iHigh = 0;
      }
      else
      {
         sR_ProgramCounter.iHigh++;
      }
   }
   else
   {
      sR_ProgramCounter.iLow = sR_ProgramCounter.iLow + 1;
   }
   if (!bSearchIntArray(iUnaryOpcodes, UNARY_OPCODES, sIR_InstrRegister.iOpcode)
       && sIR_InstrRegister.eA_AddrMode != eA_INDEXED)
   {
      MemRead (sR_ProgramCounter, sIR_InstrRegister.sR_OprndSpec);
      //**** Increment sR_ProgramCounter by sR_Two
      if (sR_ProgramCounter.iLow == LOW_BYTE - 1)
      {
         sR_ProgramCounter.iLow = 0;
         if (sR_ProgramCounter.iHigh == LOW_BYTE)
         {
            sR_ProgramCounter.iHigh = 0;
         }
         else
         {
            sR_ProgramCounter.iHigh++;
         }
      }
      else if (sR_ProgramCounter.iLow == LOW_BYTE)
      {
         sR_ProgramCounter.iLow = 1;
         if (sR_ProgramCounter.iHigh == LOW_BYTE)
         {
            sR_ProgramCounter.iHigh = 0;
         }
         else
         {
            sR_ProgramCounter.iHigh++;
         }
      }
      else
      {
         sR_ProgramCounter.iLow = sR_ProgramCounter.iLow + 2;
      }
   }
}

void Execute (bool& bHalt)
{
   switch (sIR_InstrRegister.iOpcode)
   {
      case 0 : SimSTOP (bHalt); break;
      case 1 : SimLOADR(); break;
      case 2 : SimSTORER (bHalt); break;
      case 3 : SimADDR(); break;
      case 4 : SimSUBR(); break;
      case 5 : SimANDR(); break;
      case 6 : SimORR(); break;
      case 7 : SimNOTR(); break;
      case 8 : SimASLR(); break;
      case 9 : SimASRR(); break;
      case 10 : SimLDBYTR(); break;
      case 11 : SimSTBYTR (bHalt); break;
      case 12 : SimLOADB(); break;
      case 13 : SimADDSP (bHalt); break;
      case 14 : SimBR (bHalt); break;
      case 15 : SimBRLE (bHalt); break;
      case 16 : SimBRLT (bHalt); break;
      case 17 : SimBREQ (bHalt); break;
      case 18 : SimBRNE (bHalt); break;
      case 19 : SimBRGE (bHalt); break;
      case 20 : SimBRGT (bHalt); break;
      case 21 : SimBRV (bHalt); break;
      case 22 : SimBRC (bHalt); break;
      case 23 : SimCOMPR(); break;
      case 24 : SimJSR (bHalt); break;
      case 25 : SimRTS(); break;
      case 26 : SimRTI(); break;
      case 27 : SimCHARI (bHalt); break;
      case 28 : SimCHARO (bHalt); break;
      case 29 : SimINTR(); break;
      case 30 : SimINTR(); break;
      case 31 : SimINTR(); break;
   }
}

void StartExecution (sRegisterType StartAddress)
{
   bool Halt;
   sRegisterType TraceAddr;
   int iLineCount;
   char cResponse[LINE_LENGTH];
   bool bSingleStep;
   bool bFileError;
   bool bRewriteError;
   if (!bMachineReset && !bLoading)
   {
      cout << "Execution error: Machine state not initialized." << endl;
      cout << "Use (l)oad command." << endl;
   }
   else
   {
      iKeyBufIndex = 0;
      iKeyBufMax = 0;
      bFileError = false;
      if (eTraceMode != eT_TR_OFF)
      {  // Trace is on
         if (bOutputSet && !bOutputOpen)
         {
            out_file.open(cOutFileName);
            if (out_file.fail())
            {
               bRewriteError = true;
            }
            else
            {
               bRewriteError = false;
            }
            if (bRewriteError)
            {
               bFileError = true;
               cout << "Could not open output file " << cOutFileName << "." << endl;
               bOutputOpen = false;
            }
            else
            {
               bOutputOpen = true;
            }
         }
         if (bOutputOpen)
         {
            PrintHeading (out_file);
            cResponse[0] = 'S';                 // Always scroll to disk file
         }
         else if (!bFileError)
         {
            switch (eTraceMode)
            {
               case eT_TR_PROGRAM    : cout << "User Program Trace:" << endl; break;
               case eT_TR_INTERRUPTS : cout << "User Program Trace with Interrupts:" << endl; break;
               case eT_TR_LOADER     : cout << "Loader Trace of Operating System:" << endl; break;
            }
            cout << endl;
            PrintHeading (cout);
            cResponse[0] = ' ';
         }
         iLineCount = 6;
         bSingleStep = false;
      }
      //**** End of trace
      if (!bFileError)
      {
         Halt = false;
         sR_ProgramCounter = StartAddress;
         //**** The execution cycle
         do
         {
            TraceAddr = sR_ProgramCounter;
            FetchIncrPC();
            Execute (Halt);
            if (eTraceMode != eT_TR_OFF)
            {
               Trace (TraceAddr, iLineCount, cResponse, bSingleStep);
            }
         }
         while (!Halt && (cResponse[0] != 'Q') && (cResponse[0] != 'q'));
         if (eTraceMode != eT_TR_OFF)
         {
            if (bOutputOpen)
            {
               PrintLine (out_file);
            }
            else
            {
               PrintLine (cout);
            }
         }
         bInputOpen = false;
         bOutputOpen = false;
         out_file.close();
	 out_file.clear();
      }
   }
}
//**** End of the von Neumann execution cycle. ****

void LoaderCommand()
{
   sRegisterType PCAddr;
   bLoading = true;
   sR_StackPointer.iHigh = iMemory[SYSTEM_SP];
   sR_StackPointer.iLow = iMemory[SYSTEM_SP + 1];
   PCAddr.iHigh = iMemory[LOADER_PC];
   PCAddr.iLow = iMemory[LOADER_PC + 1];
   StartExecution (PCAddr);
   bLoading = false;
}

void ExecuteCommand()
{
   sR_StackPointer.iHigh = iMemory[USER_SP];
   sR_StackPointer.iLow = iMemory[USER_SP + 1];
   StartExecution (sR_AtZero);
}

void DecodeAddress (char Digits[HEX_BYTE_LENGTH + 1], int& Value)
{
   int i,j;
   Value = 0;
   for (i = 0; i < 2; i++)
   {
      if (Digits[i] > '9')
      {
         j =  Digits[i] - 'A' + 10;
      }
      else
      {
         j = Digits[i] - '0';
      }
      Value = HEX*Value + j;
   }
   Digits[i] = '\0';
}

//**** Input starting and ending addresses for DUMP cCommand
void Parse (sRegisterType& StartAddress, sRegisterType& EndAddress)
{
   char Hex[HEX_WORD_LENGTH + 1][HEX_BYTE_LENGTH + 1];
   char c;
   int i,j;
   bool NoError;
   do
   {
      NoError = true;
      cout << endl;
      cout << "Enter address range of dump (HEX)" << endl;
      cout << "Example, 0020-0140: ";
      vGetLine(cin);
      for (i = 0; i < 2; i++)
      {
         for (j = 0; j < 2; j++)
         {
            vAdvanceInput(c);
            Hex[i][j] = c;
         }
         Hex[i][j] = '\0';
      }
      vAdvanceInput(c);
      for (i = 2; i < 4; i++)
      {
         for (j = 0;j < 2; j++)
         {
            vAdvanceInput(c);
            Hex[i][j] = c;
         }
         Hex[i][j] = '\0';
      }
      for (i = 0; i < 4; i++)
      {
         for (j = 0; j < 2; j++)
         {
            Hex[i][j] = toupper(Hex[i][j]);
            if (!bIsHexDigit(Hex[i][j]))
            {
               cout << "Error in hex specification. Enter Again." << endl;
               NoError = false;
            }
            else
            {
               DecodeAddress(Hex[0], StartAddress.iHigh);
               DecodeAddress(Hex[1], StartAddress.iLow);
               DecodeAddress(Hex[2], EndAddress.iHigh);
               DecodeAddress(Hex[3], EndAddress.iLow);
            }
         }
      }
   }
   while (!NoError);
}

void Dump (ostream& output, sRegisterType StartAddress, sRegisterType EndAddress)
{
   int Address;
   int LineAddress;
   char cHexByte[HEX_BYTE_LENGTH + 1];
   char cHexWord[HEX_WORD_LENGTH + 1];
   int i, LineCount;
   char cResponse[LINE_LENGTH];
   sRegisterType Sixteen;
   bool Carry, Ovflw;
   Sixteen.iHigh = 0;
   Sixteen.iLow = HEX;
   StartAddress.iLow = (StartAddress.iLow / HEX) * HEX; // Start with new line
   output << "DUMP    0  1  2  3  4  5  6  7  8  9  ";
   output << "A  B  C  D  E  F       ASCII" << endl << endl;
   LineCount = 2;
   if (StartAddress.iHigh < (MEMORY_SIZE / (LOW_BYTE + 1)))
   {
      Address = StartAddress.iHigh * (LOW_BYTE + 1) + StartAddress.iLow;
   }
   else
   {
      Address = MEMORY_SIZE;
   }
   Carry = false;
   while (((StartAddress.iHigh < EndAddress.iHigh)
      || (StartAddress.iHigh == EndAddress.iHigh && StartAddress.iLow <= EndAddress.iLow))
      && !(Carry && StartAddress.iHigh == 0))
   {
      LineAddress = Address;
      RegToHex (StartAddress, cHexWord);
      output << cHexWord << ":  ";
      for (i = 0; i < HEX; i++)
      {
         if (Address < MEMORY_SIZE)
         {
            vDecToHexByte (iMemory[Address++], cHexByte);
         }
         else
         {
            cHexByte[0] = 0;
            cHexByte[1] = 0;
            cHexByte[2] = '\0';
         }
         output << cHexByte << " ";
      }
      output << " ";
      char cTemp;
      for (i = 0; i < HEX; i++)
      {
         cTemp = static_cast <char> (iMemory[LineAddress]);
         if (LineAddress < MEMORY_SIZE)
         {
            if ((iMemory[LineAddress] >= ' ') &&
                (iMemory[LineAddress] <= '~'))
            {
               output << cTemp;
            }
            else
            {
               output << ".";
            }
            LineAddress++;
         }
         else
         {
            output << ".";
         }
      }
      output << endl;
      if (!bOutputOpen)
      {
         if (++LineCount == 22)
         {
            cout << endl;
            cout << "Press <return> to continue, Q to quit memory dump: ";
            cin.getline(cResponse, LINE_LENGTH);
            if (!((cResponse[0] == 'Q') || (cResponse[0] == 'q')))
            {
               cout << "DUMP    0  1  2  3  4  5  6  7  8  9  ";
               cout << "A  B  C  D  E  F       ASCII" << endl << endl;
               LineCount = 2;
            }
            else
            {
               StartAddress = EndAddress;
            }
         }
      }
      Adder (StartAddress, Sixteen, StartAddress, Carry, Ovflw);
   }
}

void DumpCommand()
{
   sRegisterType StartAddress;
   sRegisterType EndAddress;
   bool bRangeOK;
   bool bRewriteError;  // DumpCommand
   cout << "Pep/7 memory dump:  ";
   do
   {
      bRangeOK = true;
      Parse (StartAddress, EndAddress);
      if ((EndAddress.iHigh == 0) && (EndAddress.iLow == 0))
      {
         EndAddress = StartAddress;
      }
      if ((StartAddress.iHigh > EndAddress.iHigh) ||
          ((StartAddress.iHigh == EndAddress.iHigh) &&
           (StartAddress.iLow > EndAddress.iLow)))
      {
         bRangeOK = false;
         cout << "Address range error. Start address must be ";
         cout << "less than end address." << endl;
      }
   }
   while (!bRangeOK);
   if (bOutputSet)
   {
      out_file.open(cOutFileName);
      if (out_file.fail())
      {
         bRewriteError = true;
      }
      else
      {
         bRewriteError = false;
      }
      if (bRewriteError)
      {
         cout << "Error. Unable to open output file " << cOutFileName << "." << endl;
         bOutputOpen = false;
      }
      else
      {
         bOutputOpen = true;
         Dump (out_file, StartAddress, EndAddress);
      }
   }
   else
   {
      Dump (cout, StartAddress, EndAddress);
   }
   bOutputOpen = false;
   out_file.close();
   out_file.clear();
}

void TraceCommand()
{
   char cResponse[LINE_LENGTH];
   char ch;
   do
   {
      cout << "Trace  (p)rogram  (i)nterrupt  (l)oader: ";
      cin.getline(cResponse, LINE_LENGTH);
      ch = toupper(cResponse[0]);
      if (ch != 'P' && ch != 'I' && ch != 'L' && ch != ' ')
      {
         cout << "Invalid response." << endl;
      }
   }
   while (ch != 'P' && ch != 'I' && ch != 'L' && ch != ' ');
   switch (ch)
   {
      case 'P': eTraceMode = eT_TR_PROGRAM; break;
      case 'I': eTraceMode = eT_TR_INTERRUPTS; break;
      case 'L': eTraceMode = eT_TR_LOADER; break;
      case ' ': eTraceMode = eT_TR_OFF; break;
   }
   if (eTraceMode != eT_TR_OFF)
   {
      if (eTraceMode == eT_TR_LOADER)
      {
         LoaderCommand();
      }
      else
      {
         ExecuteCommand();
      }
   }
   eTraceMode = eT_TR_OFF;
}

void InputCommand()
{
   char cResponse[LINE_LENGTH];
   char ch;
   do
   {
      cout << "Input from  (k)eyboard  (f)ile: ";
      cin.getline(cResponse, LINE_LENGTH);
      ch = toupper(cResponse[0]);
      if (ch != 'K' && ch != 'F' && ch != ' ')
      {
         cout << "Invalid response." << endl;
      }
   }
   while (ch != 'K' && ch != 'F' && ch != ' ');
   if (ch == 'K')
   {
      bInputSet = false;
   }
   else if (ch == 'F')
   {
      cout << "Input file name: ";
      cin.getline(cInFileName, FILE_NAME_LENGTH);
      cInFileName[cin.gcount() - 1] = '\0';
      bInputSet = true;
   }
   if (bInputSet)
   {
      out_file << "Input file is " << cInFileName << "." << endl;
   }
   else
   {
      cout << "Input is from keyboard." << endl;
   }
}

void OutputCommand()
{
   char cResponse[LINE_LENGTH];
   char ch;
   do
   {
      cout << "Output to  (s)creen  (f)ile:  ";
      cin.getline(cResponse, LINE_LENGTH);
      ch = toupper(cResponse[0]);
      if (ch != 'S' && ch != 'F' && ch != ' ')
      {
         cout << "Invalid response." << endl;
      }
   }
   while (ch != 'S' && ch != 'F' && ch != ' ');
   cout << endl;
   if (ch == 'S')
   {
      bOutputSet = false;
   }
   else if (ch == 'F')
   {
      cout << "Enter output data file name: ";
      cin.getline(cOutFileName, FILE_NAME_LENGTH);
      cOutFileName[cin.gcount() - 1] = '\0';
      bOutputSet = true;
   }
   if (bOutputSet)
   {
      out_file << "Output file is " << cOutFileName << "." << endl;
   }
   else
   {
      cout << "Output file is screen." << endl;
   }
}

void InteractiveInput()
{
   char ch;
   do
   {
      cout << endl;
      cout << "(l)oad  e(x)ecute  (d)ump  (t)race  (i)nput  (o)utput  (q)uit: ";
      cin.getline(cCommand, LINE_LENGTH);
      ch = toupper(cCommand[0]);
      if (ch == 'L' || ch == 'X' || ch == 'D' || ch == 'T'
         || ch == 'I' || ch == 'O' || ch == 'Q')
      {
         switch (ch)
         {
            case 'L' : LoaderCommand(); break;
            case 'X' : ExecuteCommand(); break;
            case 'D' : DumpCommand(); break;
            case 'T' : TraceCommand(); break;
            case 'I' : InputCommand(); break;
            case 'O' : OutputCommand(); break;
            case 'Q' : break;
         }
      }
      else if (ch != ' ')
      {
         cout << "Invalid command." << endl;
      }
   }
   while (ch != 'Q');
}

int main (int argc, char *argv[])
{
   if (argc == 2)
   {
      if (strcmp(argv[1], "-v") == 0)
      {
         cout << "Pep/7 Simulator, version UNIX 7.4, Pepperdine University" << endl;
      }
      else
      {
         cerr << "usage: pep7 [-v]" << endl;
         return 2;
      }
   }
   else if (argc > 2)
   {
      cerr << "usage: pep7 [-v]" << endl;
      return 2;
   }
   Initialize (bError);
   if (!bError)
   {
      InstallRom (bError);
   }
   else
   {
      return 1;
   }
   if (!bError)
   {
      InteractiveInput();
   }
   else
   {
      return 3;
   }
   return 0;
}
