// asem7.cpp

// Version history
// UNIX 7.3
// January 24, 2003
// Stan Warford. Fixed a bug in DECI/Unimp0 that erroneously used the unary designation
// of the Unimp2 instruction.
//
// UNIX 7.2
// December 17, 2002
// Stan Warford. Changed program usage. Increased maximum number of program
// lines from 1024 to 8192. Increased the maximum number of bytes of code to 16384.
//
// UNIX 7.1
// March 4, 2002
// Scott Mace. Original design and implementation as an undergraduate student
// project.
//
// September 14, 2026
// Matthew McRaven. Adapted to use current C++ headers.

//Conventions
//////////////////////////////////////////////////////////////////////////////
//Constants are in capital letters.
//Function references in comments do not list any possible parameters the functions may have.
//Most functions begin with the lowercase first letter as that of the type they return.
//Most variables begin with the lowercase first letter as that of their type (ex. int iDec).
//For the most part, numbers other than 0 and 1 are either constants, array indices,
//    or are given explanations.

#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
using namespace std;

//Constants
//////////////////////////////////////////////////////////////////////////////
const int IDENT_LENGTH = 8; //Maximum identifier length
const int HEX_LENGTH = 4; //Length of a hexadecimal string
const int BYTE_LENGTH = 2; //Length of byte is 2 hex digits
const int CHAR_LENGTH = 4; //Two arbitrary delimiters and up to two characters
const int DEC_LENGTH = 6; //-32768(6 characters) to 65535
const int COMMENT_LENGTH = 65; //Maximum comment length for empty lines
const int COMMENT_LENGTH_NONEMPTY = 38; //Maximum comment length for nonempty lines
const int COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS = 47; //Maximum comment length for nonempty lines
const int STRING_LENGTH = 96; //Maximum string length for .ASCII pseudo-op
const int MAX_LINES = 8192; //pACode[MAX_LINES], pAMnemon[MAX_LINES]
const int BYTE = 1; //A byte is 1 byte
const int WORD = 2; //A word is 2 bytes
const int UNARY = 1; //A unary instruction takes up 1 byte
const int NONUNARY = 3; //A nonunary instruction takes up 3 bytes
const int OBJ_FILE_LINE_LENGTH = 16; //Number of bytes per line in object(hex) file
const int OBJ_CODE_LENGTH = 6; //Number of characters per line for object code in asem listing
const int HEX = 16; //vGenerateHexCode() and vDecToHexWord()
const int HEX2 = 256; //vDecToHexWord() for value of second most significant bit in hex
const int HEX3 = 4096; //vDecToHexWord() for value of most significant bit in hex
const int OPERAND_SPACES = 11; //Number of charcter spaces in assembler listing for operands
const int ADDR_MODES = 4; //Number of addressing modes
const int ADDR_LENGTH = 4; //Length of addresses (2 bytes or 4 hex digits)
const int MAX_ADDR = 65535; //Maximum address location
const int MAX_BYTE = 255; //Maximum decimal value for a byte
const int MAX_DEC = 65535; //Maximum decimal value
const int MIN_BYTE = -256; //Minimum decimal value for a byte
const int MIN_DEC = -32768; //Minimum decimal value
const int LINE_LENGTH = 128; //Maximum length of a line of code
const int CODE_MAX_SIZE = 32768; //Maximum number of bytes of code
const int FILE_NAME_LENGTH = 64; //61 characters maximum in a file name.
const int UNIMPLEMENTED_INSTRUCTIONS = 3; //Number of Unimplemented opcodes

//Enumerated Types
//////////////////////////////////////////////////////////////////////////////
//All possible mnemonics
enum Mnemon
{
   eM_ADDA, eM_ADDSP, eM_ADDX, eM_ANDA, eM_ANDX, eM_ASLA, eM_ASLX, 
   eM_ASRA, eM_ASRX, eM_BR, eM_BRC, eM_BREQ, eM_BRGE, eM_BRGT,
   eM_BRLE, eM_BRLT, eM_BRNE, eM_BRV, eM_CHARI, eM_CHARO, eM_COMPA,
   eM_COMPX, eM_JSR, eM_LDBYTA, eM_LDBYTX, eM_LOADA, eM_LOADB, eM_LOADX,
   eM_NOTA, eM_NOTX, eM_ORA, eM_ORX, eM_RTI, eM_RTS, eM_STBYTA, eM_STBYTX,
   eM_STOP, eM_STOREA, eM_STOREX, eM_SUBA, eM_SUBX,
   eM_UNIMP0, eM_UNIMP1, eM_UNIMP2, eM_EMPTY
};

//All possible dot commands
enum DotCommand 
{ 
   eD_BLOCK, eD_ADDRSS, eD_ASCII, eD_BURN, eD_BYTE, eD_EQUATE, eD_WORD, eD_END,
   eD_EMPTY 
};

//Tokens found by vGetToken()
enum Key 
{ 
   eT_IDENTIFIER, eT_DOTCOMMAND, eT_HEXCONSTANT, eT_ADDRMODE, eT_DECCONSTANT,
   eT_CHARCONSTANT, eT_SYMBOL, eT_COMMENT, eT_STRING, eT_EMPTY, eT_INVALIDDEC,
   eT_INVALIDHEX, eT_INVALIDCHAR, eT_INVALIDSTRING, eT_INVALID 
};

//States for finite state machine of vGetToken()
enum State 
{ 
   eS_START, eS_DOT1, eS_DOT2, eS_IDENT, eS_HEX1, eS_HEX2, eS_STRING1,
   eS_STRING2, eS_HEX3, eS_ADDR1, eS_ADDR2, eS_DECIMAL1, eS_DECIMAL2,
   eS_SIGN, eS_DECIMAL3, eS_CHAR1, eS_CHAR2, eS_CHAR3, eS_COMMENT, eS_STOP
};

//States for finite state machine of vProcessSourceLine()
enum ParseState
{
   ePS_START, ePS_COMMENT, ePS_SYMBOLDEC, ePS_INSTRUCTION, ePS_OPRNDSPECDEC,
   ePS_OPRNDSPECHEX, ePS_OPRNDSPECCHAR, ePS_OPRNDSPECSYM, ePS_DOTCOMMAND, 
   ePS_STRING, ePS_EQUATE, ePS_CLOSE, ePS_FINISH
};

//Global Records
//////////////////////////////////////////////////////////////////////////////
struct sEquateNode //Contains .EQUATE symbols to be used in a linked list
{
      char cSymValue[ADDR_LENGTH + 1]; //Value of symbol
      char cSymID[IDENT_LENGTH + 1]; //Symbol identification
      sEquateNode* pNext;
};
struct sSymbolNode //Record for symbol declarations
{
      char cSymValue[ADDR_LENGTH + 1]; //Value of symbol
      int iLine;
      char cSymID[IDENT_LENGTH + 1]; //Symbol name
      sSymbolNode* pNext; //Pointer to next sSymbolNode in the linked list
};
struct sSymbolOutputNode //Record for symbol output declarations
{
      int iLine;
      char cSymID[IDENT_LENGTH + 1]; //Symbol name
      sSymbolOutputNode* pNext; //Pointer to next sSymbolOutputNode in the linked list
};
struct sUndeclaredsSymbolNode //Record for symbol declarations
{
      char cSymValue[ADDR_LENGTH + 1]; //Value of symbol
      int iLine;
      char cSymID[IDENT_LENGTH + 1]; //Symbol identification
      sUndeclaredsSymbolNode* pNext; //Pointer to next sSymbolNode in the linked list
};
struct sCommentNode //Contains information about comments to be used in a linked list
{
      int iLine;
      bool bNonemptyLine;
      char cComment[COMMENT_LENGTH + 1];
      sCommentNode* pNext;
};
struct sUnimplementedMnemonNode //Contains .EQUATE symbols to be used in a linked list
{
      bool bIsUnary; //Tells whether the instruction is unary
      char cUnimpAddrMds[ADDR_MODES + 1]; //Valid addressing modes for opcode
      char cID[IDENT_LENGTH + 1]; //Name of unimplemented opcode
};

//Global Variables (part 1)
//////////////////////////////////////////////////////////////////////////////
ifstream in_file;
ofstream out_file;
char cLine[LINE_LENGTH]; //Array of characters for a line of code
int iLineIndex; //Index of line array
int iSecPassCodeIndex = 0; //Used in second pass of assembly to account for symbols
int iCurrentAddress = 0; //Keeps track of the current address
sSymbolNode* pSymbol; //Pointer to linked list of sSymbolNodes
sSymbolOutputNode* pSymbolOutput; //Pointer to linked list of sSymbolNodes for output
sUndeclaredsSymbolNode* pUndeclaredSym; //Pointer to linked list of sUndeclaredsSymbolNodes
sCommentNode* pComment = NULL; //Pointer to first sCommentNode of the comment linked list
sEquateNode* pEquate = NULL; //Pointer to first sEquateNode of the .EQUATE linked list
char cDotTable[eD_EMPTY + 1][IDENT_LENGTH + 1]; //Used for vLookUpDot()
char cMnemonTable [eM_EMPTY + 1][IDENT_LENGTH + 1]; //Used for vLookUpMnemon()
int iHexOutputBuffer = 0; //Used for object code output for 16 bytes per line
bool bIsAscii = false; //Keeps track of whether previous token was .ASCII pseudo-op
int iBurnStart = 0; //Used first to  store the value of the operand of a .BURN
//and then to  store the value of where the first byte of code should be written.
int iBurnAddr = 0; //Used to store the address of a .BURN line
int iBurnCounter = 0; //Keeps track of number of .BURNs used in the program.
//int iCursor = 0;
sUnimplementedMnemonNode sUnimpMnemon [UNIMPLEMENTED_INSTRUCTIONS]; //Array of unimplemented mnemon nodes
// Global variables continued after class ACode declaration

//Utility functions
//////////////////////////////////////////////////////////////////////////////

//Stores the next line of assembly language code to be translated in global cLine[].
void vGetLine()
{
   in_file.getline(cLine, LINE_LENGTH);
   if ((!in_file.eof ()) && (in_file.gcount() > 0))
   {
      cLine[in_file.gcount() - 1] = '\n';
   }
   else
   {
      cLine[in_file.gcount()] = '\n';
   }
   iLineIndex = 0;
}

//Gets the next character to be processed by vGetToken().
void vAdvanceInput (char& ch)
{
   ch = cLine[iLineIndex++];
}

//Backs up the input to the current character to be processed by vGetToken().
void vBackUpInput ()
{
   iLineIndex--;
}

//Outputs the version number of the Pep/7 assembler
void vVersionNumber ()
{
   cerr << "Pep/7 Assembler, version UNIX 7.2, Pepperdine University" << endl;
}

//Converts a hexadecimal number to a decimal number and returns the decimal number.
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

//Converts a decimal number(0-15) to a hexadecimal number(0-F) and returns the hex number.
char cDecToHex (int i)
{
   switch (i)
   {
      case 0:  return '0'; break;
      case 1:  return '1'; break;
      case 2:  return '2'; break;
      case 3:  return '3'; break;
      case 4:  return '4'; break;
      case 5:  return '5'; break;
      case 6:  return '6'; break;
      case 7:  return '7'; break;
      case 8:  return '8'; break;
      case 9:  return '9'; break;
      case 10:  return 'A'; break;
      case 11:  return 'B'; break;
      case 12:  return 'C'; break;
      case 13:  return 'D'; break;
      case 14:  return 'E'; break;
      case 15:  return 'F'; break;
   }
}

//Converts a hexadecimal byte to a positive decimal integer
int iHexByteToDecInt (char cHex[HEX_LENGTH + 1])
{
   return HEX * iHexToDec(cHex[0]) + iHexToDec(cHex[1]);
}

//Converts a hexadecimal word to a positive decimal integer
int iHexWordToDecInt (char cHex[HEX_LENGTH + 1])
{
   return HEX3 * iHexToDec(cHex[0]) + HEX2 * iHexToDec(cHex[1]) + HEX * iHexToDec(cHex[2]) + iHexToDec(cHex[3]);
}

//Converts a decimal value between -256 to 255 to a hexadecimal array of characters
void vDecToHexByte (int iDec, char cHex[BYTE_LENGTH + 1]) //Used to convert opcodes to hex
{
   if (iDec < 0)
   {
      iDec = iDec + MAX_BYTE + 1;
   }
   cHex[0] = cDecToHex(iDec / HEX);
   cHex[1] = cDecToHex(iDec % HEX);
   cHex[2] = '\0';
}

//Converts a decimal value between -32768 and 65535 to a hexadecimal array of characters
void vDecToHexWord (int iDec, char cHex[ADDR_LENGTH + 1])
{
   int iFirstInt;
   int iSecondInt;
   int iThirdInt;
   int iFourthInt;
   if (iDec < 0)
   {
      iDec = iDec + MAX_DEC + 1;
   }
   iFirstInt = iDec / HEX3;
   iSecondInt = (iDec - HEX3 * iFirstInt) / HEX2;
   iThirdInt = (iDec - HEX3 * iFirstInt - iSecondInt * HEX2) / HEX;
   iFourthInt = (iDec - HEX3 * iFirstInt - iSecondInt * HEX2 - iThirdInt * HEX);
   cHex[0] = cDecToHex(iFirstInt);
   cHex[1] = cDecToHex(iSecondInt);
   cHex[2] = cDecToHex(iThirdInt);
   cHex[3] = cDecToHex(iFourthInt);
   cHex[4] = '\0';
}

//Converts an array of characters (decimal constant) to its integer decimal equivalent.
int iCharToInt (char ch [])
{
   enum CIState {eCIS_START, eCIS_iSign, eCIS_INTEGER, eS_STOP };
   CIState state = eCIS_START;
   int iSign;
   int intValue;
   int i = 0;
   while (ch[i] != '\0')
   {
      switch  (state) //Finite State Machine implementation
      {
         case eCIS_START:
            if (isdigit (ch[i]))
            {
               intValue = ch[i] - '0';
               iSign = 1;
               state= eCIS_INTEGER;
            }
            else if (ch[i] == '-')
            {
               iSign = -1;
               state = eCIS_iSign;
            }
            else if (ch[i] == '+')
            {
               iSign = 1;
               state = eCIS_iSign;
            }
            break;
         case eCIS_iSign:
            if (isdigit(ch[i]))
            {
               intValue = ch[i] - '0';
               state = eCIS_INTEGER;
            }
            break;
         case eCIS_INTEGER:
            if (isdigit (ch[i]))
            {
               intValue = 10 * intValue + ch[i] - '0';//Each place in decimal is 10 times
               //that of the previous one
            }
            break;
      };
      i++;
   }
   return intValue * iSign;
}

//Converts an addressing mode to its decimal equivalent for use in method vGenerateHexCode().
int iAddrModeValue (char addrMode)
{
   switch (addrMode)
   {
      case 'i': return 0; break;
      case 'd': return 1; break;
      case 's': return 2; break;
      case 'x': return 3; break;
   }
}

//Returns whether addrMode is a valid addressing mode for a particular instruction.
bool bSearchAddrModes (char cTemp[], char cAddrMode)
{
   int i = 0;
   do
   {
      if (cTemp[i] == cAddrMode)
      {
         return true;
      }
      else if (cTemp[i] == '\0')
      {
         return false;
      }
      i++;
   }
   while ((cTemp[i - 1] != cAddrMode) && (cTemp[i - 1] != '\0'));
}

//Gives cValue[] the cValue of the symbol named in cID[]
void vGetSymbolValue(char cID[], char cValue[]) //assertion: symbol has been defined
{
   sSymbolNode* pTemp = pSymbol;
   while (pTemp != NULL)
   {
      if (strcmp(cID, pTemp->cSymID) == 0)
      {
         strncpy(cValue, pTemp->cSymValue, HEX_LENGTH + 1);
         return;
      }
      pTemp = pTemp->pNext;
   }
}

//Continues the output following the first line of object code in the assembler
//   listing when a .BLOCK command occurs when more than 3 bytes are reserved.
void vDotBlockOutputContinued (int iDec)
{
   int iLineCounter = 0;
   int i;
   out_file << endl << "      ";
   for (i = 0; i < iDec; i++)
   {
      if (iLineCounter == OBJ_CODE_LENGTH)
      {
         out_file << " " << endl << "      ";
         iLineCounter = 0;
      }
      out_file << "00";
      iLineCounter = iLineCounter + 2; //There are two 0s output
   }
   for (i = iLineCounter; i <= OBJ_CODE_LENGTH; i++)
   {
      out_file << " ";
   }
}

//Continues the output following the first line of object code in the assembler
//   listing when a .ASCII command occurs when more than 3 bytes are reserved.
void vDotAsciiOutputContinued (char cStr[])
{
   int i = 4; //Delimiter and first 3 have already been accounted for in vGenerateHexCode()
   int iLineCounter = 0;
   int iChar[STRING_LENGTH + 1];
   char cHex[BYTE_LENGTH + 1];
        
   while (cStr[i + 1] != '\0')
   {
      iChar[i - 4] = static_cast <int> (cStr[i]); //'i' begins at 4 and arrays at 0
      i++;
   }
   i = 4; //Same reason as before previous loop
   while (cStr[i + 1] != '\0') //Add one for end arbitrary delimiter
   {
      if (iLineCounter == OBJ_CODE_LENGTH)
      {
         out_file << " " << endl << "      ";
         iLineCounter = 0;
      }
      vDecToHexByte(iChar[i - 4], cHex);
      out_file << cHex;
      i++;
      iLineCounter = iLineCounter + 2; //A character in ascii takes 2 hex digits
   }
   for (i = iLineCounter; i <= OBJ_CODE_LENGTH; i++)
   {
      out_file << " ";
   }    
}

//Gets a line from the mnemon file
void vGetMnemonLine (int iLine)
{
   char cInputChar;
   bool bUnary = false;
   int i = 0;
   int j = 0;
   vGetLine();
   vAdvanceInput(cInputChar);
   if ((cInputChar == 'u') || (cInputChar == 'U'))
   {
      bUnary = true;
   }
   vAdvanceInput(cInputChar);
   vAdvanceInput(cInputChar);
   while (cInputChar != ' ')
   {
      if (i < IDENT_LENGTH)
      {
         sUnimpMnemon[iLine].cID[i++] = toupper(cInputChar);
      }
      vAdvanceInput(cInputChar);
   }
   sUnimpMnemon[iLine].cID[i] = '\0';
   vAdvanceInput(cInputChar);
   while ((j < ADDR_MODES) && (cInputChar != '\n'))
   {
      sUnimpMnemon[iLine].cUnimpAddrMds[j++] = cInputChar;
      vAdvanceInput(cInputChar);
   }
   sUnimpMnemon[iLine].cUnimpAddrMds[j] = '\0';
   sUnimpMnemon[iLine].bIsUnary = bUnary;
}

//Buffers for assembler listing
//////////////////////////////////////////////////////////////////////////////

//Buffer for spaces in symbol column in assembler listing
void vSymbolBuffer (char cSym[])
{
   out_file << ":";
   int i = 0;
   while (cSym[i] != '\0')
   {
      i++;
   }
   for (int j = i; j < IDENT_LENGTH; j++)
   {
      out_file << " ";
   }
}

//Buffer for spaces in symbol column in symbol table
void vSymbolListingBuffer (char cSym[])
{
   int i = 0;
   while (cSym[i] != '\0')
   {
      i++;
   }
   for (int j = i; j <= IDENT_LENGTH; j++)
   {
      out_file << " ";
   }
}

//Buffer for spaces in mnemon column in assembler listing for cDot commands
void vDotCommandBuffer (char cDot[])
{
   int i = 0;
   while (cDot[i] != '\0')
   {
      i++;
   }
   for (int j = i; j < IDENT_LENGTH - 1; j++)
   {
      out_file << " ";
   }
}

//Buffer for spaces in mnemon column in assembler listing for cMnemon
void vMnemonBuffer (char cMnemon[])
{
   int i = 0;
   while (cMnemon[i] != '\0')
   {
      i++;
   }
   for (int j = i; j < IDENT_LENGTH; j++)
   {
      out_file << " ";
   }
}

//Buffer for spaces in operand column in assembler listing
void vOperandBuffer (char cOperand[], bool bConstant, bool bAddrMd)
{
   int iTemp = OPERAND_SPACES; //used to keep track of spaces required
   if (bConstant)
   {
      iTemp = iTemp - 2; //ex. "d#" is 2 spaces
   }
   if (bAddrMd)
   {
      iTemp = iTemp - 2; //ex. ",i" is 2 spaces
   }
   int i = 0;
   while (cOperand[i] != '\0')
   {
      iTemp--;
      i++;
   }
   for (int j = iTemp; j > 0; j--)
   {
      out_file << " ";
   }
}

//Outputs spaces for a blank address column in the assembler listing
void vBlankAddressColumn ()
{
   out_file << "      ";
}

//Outputs spaces for a blank object code column in the assembler listing
void vBlankObjCodeColumn ()
{
   out_file << "       ";
}

//Outputs spaces for a blank symbol column in the assembler listing
void vBlankSymbolColumn ()
{
   out_file << "         ";
}

void vOutputSymbolDecs ()
{
   if (pSymbol != NULL)
   {
      if ((pSymbolOutput != NULL) && (pSymbolOutput->iLine == iSecPassCodeIndex)) 
      {
         out_file << pSymbolOutput->cSymID;
         vSymbolBuffer (pSymbolOutput->cSymID);
         pSymbolOutput = pSymbolOutput->pNext;
      }
      else
      {
         vBlankSymbolColumn ();
      }
   }
}

//Buffer for object file for loader
void vHexOutputBufferLoader ()
{
   if (iHexOutputBuffer == OBJ_FILE_LINE_LENGTH - 1)
   {
      out_file << endl;
      iHexOutputBuffer = 0;
   }
   else
   {
      out_file << " ";
      iHexOutputBuffer++;
   }
}

//Abstract Token Class
//////////////////////////////////////////////////////////////////////////////

class AToken
{
   public:
      virtual Key kTokenType () = 0;
};

class TEmpty : public AToken
{
   public:
      Key kTokenType () { return eT_EMPTY; }
};

class TInvalid : public AToken
{
   public:
      Key kTokenType () { return eT_INVALID; }
};

class TInvalidDec : public AToken
{
   public:
      Key kTokenType () { return eT_INVALIDDEC; }
};

class TInvalidHex : public AToken
{
   public:
      Key kTokenType () { return eT_INVALIDHEX; }
};

class TInvalidChar : public AToken
{
   public:
      Key kTokenType () { return eT_INVALIDCHAR; }
};

class TInvalidString : public AToken
{
   public:
      Key kTokenType () { return eT_INVALIDSTRING; }
};

class TDotCommand : public AToken
{
   private:
      char cDotValue[IDENT_LENGTH + 1];
   public:
      TDotCommand (char str[]) { strncpy (cDotValue, str, IDENT_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cDotValue, IDENT_LENGTH + 1); }
      Key kTokenType () { return eT_DOTCOMMAND; }
};

class TIdentifier : public AToken
{
   private:
      char cIdentValue[IDENT_LENGTH + 1];
   public:
      TIdentifier (char str[]) { strncpy (cIdentValue, str, IDENT_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cIdentValue, IDENT_LENGTH + 1); }
      Key kTokenType () { return eT_IDENTIFIER; }
};

class TAddress : public AToken
{
   private:
      char cAddrValue;
   public:
      TAddress (char AddrVal) { cAddrValue = AddrVal; }
      void vGetValue (char& AddrVal) { AddrVal = cAddrValue; }
      Key kTokenType () { return eT_ADDRMODE; }
};

class TSymbol : public AToken
{
   private:
      char cSymbolValue[IDENT_LENGTH + 1];
   public:
      TSymbol (char str[]) { strncpy (cSymbolValue, str, IDENT_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cSymbolValue, IDENT_LENGTH + 1); }
      Key kTokenType () { return eT_SYMBOL; }
};

class THexConstant : public AToken
{
   private:
      char cHexValue[HEX_LENGTH + 1];
   public:
      THexConstant (char str[]) { strncpy (cHexValue, str, HEX_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cHexValue, HEX_LENGTH + 1); }
      Key kTokenType () { return eT_HEXCONSTANT; }
};

class TDecConstant : public AToken
{
   private:
      char cDecValue[DEC_LENGTH + 1];
   public:
      TDecConstant (char str[]) { strncpy (cDecValue, str, DEC_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cDecValue, DEC_LENGTH + 1); }
      Key kTokenType () { return eT_DECCONSTANT; }
};

class TCharConstant : public AToken
{
   private:
      char cCharValue[CHAR_LENGTH + 1];
   public:
      TCharConstant (char str[]) { strncpy (cCharValue, str, CHAR_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cCharValue, CHAR_LENGTH + 1); }
      Key kTokenType () { return eT_CHARCONSTANT; }
};

class TComment : public AToken
{
   private:
      char cCommentValue[COMMENT_LENGTH + 1];
   public:
      TComment (char str[]) { strncpy (cCommentValue, str, COMMENT_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cCommentValue, COMMENT_LENGTH + 1); }
      Key kTokenType () { return eT_COMMENT; }
};

class TString : public AToken
{
   private:
      char cStringValue[STRING_LENGTH + 1];
   public:
      TString (char str[]) { strncpy (cStringValue, str, STRING_LENGTH + 1); }
      void vGetValue (char str[]) { strncpy (str, cStringValue, STRING_LENGTH + 1); }
      Key kTokenType () { return eT_STRING; }
};

//Abstract Mnemonic Class
//////////////////////////////////////////////////////////////////////////////

class AMnemon
{
   public:
      virtual int iOpCode () = 0; //Returns the integer operation code of a given mnemonic
      virtual void vAddrModes (char addressingMds[]) = 0; //Gives addressingMds[] the characters
         //of possible addressing modes for a particular mnemonic
      virtual bool bIsUnary () = 0; //Returns whether a given mnemonic is unary or not
      virtual bool bNoAddrModeRequired () = 0; //Returns whether no addressing mode is required
      virtual void vMnemonOutput () = 0; //Outputs the mnemonic for the assembler listing
};

void vInitMnemonObjects  (Mnemon mnemon);

class Adda : public AMnemon
{
   public:
      int iOpCode () { return 24; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ADDA    "; }
};

class Addsp : public AMnemon
{
   public:
      int iOpCode () { return 104; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ADDSP   "; }
};

class Addx : public AMnemon
{
   public:
      int iOpCode () { return 28; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ADDX    "; }
};

class Anda : public AMnemon
{
   public:
      int iOpCode () { return 40; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ANDA    "; }
};

class Andx : public AMnemon
{
   public:
      int iOpCode () { return 44; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ANDX    "; }
};

class Asla : public AMnemon
{
   public:
      int iOpCode () { return 64; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ASLA    "; }
};

class Aslx : public AMnemon
{
   public:
      int iOpCode () { return 68; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ASLX    "; }
};

class Asra : public AMnemon
{
   public:
      int iOpCode () { return 72; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ASRA    "; }
};

class Asrx : public AMnemon
{
   public:
      int iOpCode () { return 76; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ASRX    "; }
};

class Br : public AMnemon
{
   public:
      int iOpCode () { return 112; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';       
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BR      "; }
};

class Brc : public AMnemon
{
   public:
      int iOpCode () { return 176; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BRC     "; }
};

class Breq : public AMnemon
{
   public:
      int iOpCode () { return 136; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BREQ    "; }
};

class Brge : public AMnemon
{
   public:
      int iOpCode () { return 152; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BRGE    "; }
};

class Brgt : public AMnemon
{
   public:
      int iOpCode () { return 160; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BRGT    "; }
};

class Brle : public AMnemon
{
   public:
      int iOpCode () { return 120; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BRLE    "; }
};

class Brlt : public AMnemon
{
   public:
      int iOpCode () { return 128; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BRLT    "; }
};

class Brne : public AMnemon
{
   public:
      int iOpCode () { return 144; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';       
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BRNE    "; }
};

class Brv : public AMnemon
{
   public:
      int iOpCode () { return 168; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';       
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "BRV     "; }
};

class Chari : public AMnemon
{
   public:
      int iOpCode () { return 216; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'd';
         addressingMds[1] = 's';
         addressingMds[2] = 'x';
         addressingMds[3] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "CHARI   "; }
};

class Charo : public AMnemon
{
   public:
      int iOpCode () { return 224; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "CHARO   "; }
};

class Compa : public AMnemon
{
   public:
      int iOpCode () { return 184; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "COMPA   "; }
};

class Compx : public AMnemon
{
   public:
      int iOpCode () { return 188; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "COMPX   "; }
};

class Jsr : public AMnemon
{
   public:
      int iOpCode () { return 192; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'x';
         addressingMds[2] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return true; }
      void vMnemonOutput () { out_file << "JSR     "; }
};

class Ldbyta : public AMnemon
{
   public:
      int iOpCode () { return 80; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "LDBYTA  "; }
};

class Ldbytx : public AMnemon
{
   public:
      int iOpCode () { return 84; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "LDBYTX  "; }
};

class Loada : public AMnemon
{
   public:
      int iOpCode () { return 8; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "LOADA   "; }
};

class Loadb : public AMnemon
{
   public:
      int iOpCode () { return 96; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "LOADB   "; }
};

class Loadx : public AMnemon
{
   public:
      int iOpCode () { return 12; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "LOADX   "; }
};

class Nota : public AMnemon
{
   public:
      int iOpCode () { return 56; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "NOTA    "; }
};

class Notx : public AMnemon
{
   public:
      int iOpCode () { return 60; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "NOTX    "; }
};

class Ora : public AMnemon
{
   public:
      int iOpCode () { return 48; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ORA     "; }
};

class Orx : public AMnemon
{
   public:
      int iOpCode () { return 52; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "ORX     "; }
};

class Rti : public AMnemon
{
   public:
      int iOpCode () { return 208; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "RTI     "; }
};

class Rts : public AMnemon
{
   public:
      int iOpCode () { return 200; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "RTS     "; }
};

class Stbyta : public AMnemon
{
   public:
      int iOpCode () { return 88; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'd';
         addressingMds[1] = 's';
         addressingMds[2] = 'x';
         addressingMds[3] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "STBYTA  "; }
};

class Stbytx : public AMnemon
{
   public:
      int iOpCode () { return 92; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'd';
         addressingMds[1] = 's';
         addressingMds[2] = 'x';
         addressingMds[3] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "STBYTX  "; }
};

class Stop : public AMnemon
{
   public:
      int iOpCode () { return 0; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = '\0';               
      }
      bool bIsUnary () { return true; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "STOP    "; }
};

class Storea : public AMnemon
{
   public:
      int iOpCode () { return 16; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'd';
         addressingMds[1] = 's';
         addressingMds[2] = 'x';
         addressingMds[3] = '\0';               
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "STOREA  "; }
};

class Storex : public AMnemon
{
   public:
      int iOpCode () { return 20; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'd';
         addressingMds[1] = 's';
         addressingMds[2] = 'x';
         addressingMds[3] = '\0';
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "STOREX  "; }
};

class Suba : public AMnemon
{
   public:
      int iOpCode () { return 32; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "SUBA    "; }
};

class Subx : public AMnemon
{       
   public:
      int iOpCode () { return 36; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = 'i';
         addressingMds[1] = 'd';
         addressingMds[2] = 's';
         addressingMds[3] = 'x';
         addressingMds[4] = '\0';
      }
      bool bIsUnary () { return false; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () { out_file << "SUBX    "; }
};

class Unimp0 : public AMnemon
{       
   public:
      int iOpCode () { return 232; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = sUnimpMnemon[0].cUnimpAddrMds[0];
         addressingMds[1] = sUnimpMnemon[0].cUnimpAddrMds[1];
         addressingMds[2] = sUnimpMnemon[0].cUnimpAddrMds[2];
         addressingMds[3] = sUnimpMnemon[0].cUnimpAddrMds[3];
         addressingMds[4] = sUnimpMnemon[0].cUnimpAddrMds[4];
      }
      bool bIsUnary () { return sUnimpMnemon[0].bIsUnary; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () 
      { 
         out_file << sUnimpMnemon[0].cID; 
         vMnemonBuffer(sUnimpMnemon[0].cID);
      }
};

class Unimp1 : public AMnemon
{       
   public:
      int iOpCode () { return 240; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = sUnimpMnemon[1].cUnimpAddrMds[0];
         addressingMds[1] = sUnimpMnemon[1].cUnimpAddrMds[1];
         addressingMds[2] = sUnimpMnemon[1].cUnimpAddrMds[2];
         addressingMds[3] = sUnimpMnemon[1].cUnimpAddrMds[3];
         addressingMds[4] = sUnimpMnemon[1].cUnimpAddrMds[4];
      }
      bool bIsUnary () { return sUnimpMnemon[1].bIsUnary; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () 
      { 
         out_file << sUnimpMnemon[1].cID; 
         vMnemonBuffer(sUnimpMnemon[1].cID);
      }
};

class Unimp2 : public AMnemon
{       
   public:
      int iOpCode () { return 248; }
      void vAddrModes (char addressingMds[])
      {
         addressingMds[0] = sUnimpMnemon[2].cUnimpAddrMds[0];
         addressingMds[1] = sUnimpMnemon[2].cUnimpAddrMds[1];
         addressingMds[2] = sUnimpMnemon[2].cUnimpAddrMds[2];
         addressingMds[3] = sUnimpMnemon[2].cUnimpAddrMds[3];
         addressingMds[4] = sUnimpMnemon[2].cUnimpAddrMds[4];
      }
      bool bIsUnary () { return sUnimpMnemon[2].bIsUnary; }
      bool bNoAddrModeRequired () { return false; }
      void vMnemonOutput () 
      { 
         out_file << sUnimpMnemon[2].cID; 
         vMnemonBuffer(sUnimpMnemon[2].cID);
      }
};

//Abstract Code Class
//////////////////////////////////////////////////////////////////////////////

class ACode
{
   public:
      virtual ~ACode () {};
      virtual bool bIsError () = 0;
      virtual void vGenerateCode () = 0;
};

//Global variables, part 2
AToken* pPrevAT = new TEmpty; //Used to detect strings in vGetToken() if .ASCII was previous token
ACode* pACode[MAX_LINES + 1];//Array of pointers to abstract code
int iCodeIndex; //Used as index of pACode and pAMnemon arrays


//Error class
//////////////////////////////////////////////////////////////////////////////

class Error : public ACode
{
   public:
      bool bIsError () { return true; }
};

class eNoEnd : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Missing .END sentinal" << endl;
      }
};

class eTooLong : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Program too long. Listing table overflow." << endl;
      }
};

class eSymPrevDef : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Symbol previously defined." << endl;
      }
};

class eProgTooLong : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Program too long. Code table overflow." << endl;
      }
};

class eInstrDotExp : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Instruction or dot command expected." << endl;
      }
};

class eInvSyntax : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid syntax." << endl;
      }
};

class eSymInstrDotExp : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Symbol, instruction, or dot command expected." << endl;
      }
};

class eInvMnemon : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid Mnemonic." << endl;
      }
};

class eCommExp : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Comment expected." << endl;
      }
};

class eOprndSpecExp : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Operand specifier expected." << endl;
      }
};

class eNoDecConst : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid decimal constant." << endl;
      }
};

class eNoHexConst : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid hexadecimal constant." << endl;
      }
};

class eNoCharConst : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid character constant." << endl;
      }
};

class eAddrExp : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Addressing mode expected." << endl;
      }
};

class eAddrCommExp : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Addressing mode or comment expected." << endl;
      }
};

class eNoAddr : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid addressing mode." << endl;
      }
};

class eNoAddrmode : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "This instruction cannot have this addressing mode." << endl;
      }
};

class eDecOverflow : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Decimal overflow. Range is -32768 to 65535." << endl;
      }
};

class eNoDotCom : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid dot command." << endl;
      }
};

class eNoString : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Invalid string expression." << endl;
      }
};

class eDecHexExp : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Decimal or hex constant expected." << endl;
      }
};

class eNoCharWithInstruct : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Character constant not allowed with this instruction." << endl;
      }
};

class eSymExpWithAddrss : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Symbol required after .ADDRSS." << endl;
      }
};

class eSymBeforeEquate : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Symbol required before .EQUATE." << endl;
      }
};

class eConstOverflow : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Constant overflow. Range is 0 to 255 (dec)." << endl;
      }
};

class eByteOutOfRange : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Byte value out of range." << endl;
      }
};

class eSymNotDefined : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Symbol referenced but not defined." << endl;
      }
};

class eAddrOverflow : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Address overflow. Range is 0 to 65535 (dec)." << endl;
      }
};

class eTooMuchCodeForBurnAddr : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Too much code for given .BURN address." << endl;
      }
};

class eOneBurn : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "More than one .BURN not allowed in program." << endl;
      }
};

class eXAddrUnary : public Error
{
   public:
      void vGenerateCode ()
      {
         cerr << "Instructions with indexed addressing are unary." << endl;
      }
};

//Valid Class
//////////////////////////////////////////////////////////////////////////////

class Valid : public ACode
{
   protected:
      Mnemon mnemonic;
      DotCommand dotcom;
        
   public:
      virtual ~Valid () {};
      bool bIsError () { return false; }  //Since the valid class would not contain errors
      virtual int iAddressCounter () = 0; //Returns how many bytes each class takes up
      virtual void vGenerateHexCode (bool asemList) = 0; //Generates the object code
      virtual void vBurnAddressChange () = 0; //Changes iAddress to account for a .BURN
};

class ZeroArg : public Valid
{
   public:
      ZeroArg (DotCommand dot) { dotcom = dot; }
      int iAddressCounter () { return 0; }
      void vBurnAddressChange () {}
      void vGenerateCode () { out_file << "             "; }
      void vGenerateHexCode (bool asemList) { }
};

class DotEnd : public Valid
{
   private:
      int iAddress;
      char cFirstArg[IDENT_LENGTH + 1];
   public:
      DotEnd (int iAddr, DotCommand dot, char fArg[]) 
      {
         iAddress = iAddr;
         dotcom = dot; 
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
      }
      int iAddressCounter () { return 0; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode () 
      {
         char cAddr[ADDR_LENGTH + 1];
                
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         vBlankObjCodeColumn();
         vOutputSymbolDecs();
         out_file << "." << cFirstArg;
         vDotCommandBuffer(cFirstArg);
         out_file << "           ";
      }
      void vGenerateHexCode (bool asemList) { }
};

class UnaryInstruction : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
   public:
      UnaryInstruction (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[])
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
      }
      ~UnaryInstruction () { delete pAMnemonic; }
      int iAddressCounter () { return UNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << "           ";
      }
      void vGenerateHexCode (bool asemList)
      {
         char cTemp[BYTE_LENGTH + 1];
         vDecToHexByte(pAMnemonic->iOpCode(), cTemp);
         if (asemList)
         {
            out_file << cTemp << "     ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cTemp;
               vHexOutputBufferLoader();
            }
         }
      }
};

class IndexedAddrInstruction : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cThirdArg;
   public:
      IndexedAddrInstruction (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char tArg)
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         cThirdArg = tArg;
      }
      ~IndexedAddrInstruction () { delete pAMnemonic; }
      int iAddressCounter () { return UNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << "," << cThirdArg;
         char temp[1];
         temp[0] = '\0';
         vOperandBuffer (temp, false, true);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cTemp[BYTE_LENGTH + 1];
         vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue('x'), cTemp);
         if (asemList)
         {
            out_file << cTemp << "     ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cTemp;
               vHexOutputBufferLoader();
            }
         }
      }
};

class DotComDec : public Valid
{
   private:
      int iAddress;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[DEC_LENGTH + 1];
   public:
      DotComDec (int iAddr, DotCommand dot, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         dotcom = dot;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, DEC_LENGTH + 1);
      }
      int iAddressCounter () 
      { 
         switch (dotcom)
         {
            case eD_BLOCK:
               return iCharToInt(cSecondArg);
               break;
            case eD_BURN:
               return 0;
               break;
            case eD_BYTE:
               return BYTE;
               break;
            case eD_EQUATE:
               return 0;
               break;
            case eD_WORD:
               return WORD;
               break;
         } 
      }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         int iDec = iCharToInt(cSecondArg);
         char cAddr[ADDR_LENGTH + 1];
                
         if (dotcom != eD_EQUATE)
         {
            vDecToHexWord (iAddress, cAddr);
            out_file << cAddr << "  ";
         }
         else
         {
            out_file << "      ";
         }
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         out_file << "." << cFirstArg;
         vDotCommandBuffer(cFirstArg);
         out_file << "d#" << cSecondArg;
         vOperandBuffer (cSecondArg, true, false);
         if ((pComment != NULL) && (pComment->iLine == iSecPassCodeIndex))
         {
            if (pComment->bNonemptyLine)
            {
               if (pSymbol == NULL)
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1] = '\0';
               }
               else
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1] = '\0';
               }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p = pComment;
            pComment = pComment->pNext;
            delete p;
         }
         if ((dotcom == eD_BLOCK) && (iDec > OBJ_CODE_LENGTH / BYTE_LENGTH) &&
             ((iBurnCounter == 0) || (iAddress >= iBurnAddr)))
         {
            vDotBlockOutputContinued(iDec - OBJ_CODE_LENGTH / BYTE_LENGTH);
         }
      }
      void vGenerateHexCode (bool asemList)
      {
         int i;
         int iDec = iCharToInt(cSecondArg);
         char cVal[ADDR_LENGTH + 1];
         int lineCounter = 0;
         if (asemList)
         {
            switch (dotcom)
            {
               case eD_BLOCK:
                  if (iDec <= OBJ_CODE_LENGTH / BYTE_LENGTH)
                  {
                     for (i = 0; i < iDec; i++)
                     {
                        out_file << "00";
                        lineCounter = lineCounter + 2;
                     }
                     for (i = lineCounter; i <= OBJ_CODE_LENGTH; i++)
                     {
                        out_file << " ";
                     }
                  }
                  else
                  {
                     for (i = 0; i < OBJ_CODE_LENGTH / BYTE_LENGTH; i++)
                     {
                        out_file << "00";
                     }
                     out_file << " ";
                  }
                  break;
               case eD_BURN:
                  vBlankObjCodeColumn();
                  break;
               case eD_BYTE:
                  vDecToHexByte(iDec, cVal);
                  out_file << cVal << "     ";
                  break;
               case eD_EQUATE:
                  vBlankObjCodeColumn();
                  break;
               case eD_WORD:
                  vDecToHexWord(iDec, cVal);
                  out_file << cVal << "   ";
                  break;
            }
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               switch (dotcom)
               {
                  case eD_BLOCK:
                     for (i = 0; i < iDec; i++)
                     {
                        out_file << "00";
                        vHexOutputBufferLoader();
                     }
                     break;
                  case eD_BURN:
                     break;
                  case eD_BYTE:
                     vDecToHexByte(iDec, cVal);
                     out_file << cVal;
                     vHexOutputBufferLoader();
                     break;
                  case eD_EQUATE:
                     break;
                  case eD_WORD:
                     vDecToHexWord(iDec, cVal);
                     out_file << cVal[0] << cVal[1];
                     vHexOutputBufferLoader();
                     out_file << cVal[2] << cVal[3];
                     vHexOutputBufferLoader();
                     break;
               }
            }
         }
      }
};

class DotComHex : public Valid
{
   private:
      int iAddress;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[HEX_LENGTH + 1];
   public:
      DotComHex (int iAddr, DotCommand dot, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         dotcom = dot;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, HEX_LENGTH + 1);
      }
      int iAddressCounter ()
      { 
         switch (dotcom)
         {
            case eD_BLOCK:
               cSecondArg[0] = '0';
               cSecondArg[1] = '0';
               return iHexWordToDecInt(cSecondArg);
               break;
            case eD_BURN:
               return 0;
               break;
            case eD_EQUATE:
               return 0;
               break;
            case eD_WORD:
               return WORD;
               break;
         }
                 
      }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         int iDec = iHexWordToDecInt(cSecondArg);
         char cAddr[ADDR_LENGTH + 1];
                
         if (dotcom != eD_EQUATE)
         {
            vDecToHexWord (iAddress, cAddr);
            out_file << cAddr << "  ";
         }
         else
         {
            out_file << "      ";
         }
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         out_file << "." << cFirstArg;
         vDotCommandBuffer(cFirstArg);
         out_file << "h#" << cSecondArg;
         vOperandBuffer (cSecondArg, true, false);
         if ((pComment != NULL) && (pComment->iLine == iSecPassCodeIndex))
         {
            if (pComment->bNonemptyLine)
            {
               if (pSymbol == NULL)
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1] = '\0';
               }
               else
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1] = '\0';
               }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p = pComment;
            pComment = pComment->pNext;
            delete p;
         }
         if ((dotcom == eD_BLOCK) && (iDec > OBJ_CODE_LENGTH / BYTE_LENGTH) &&
             ((iBurnCounter == 0) || (iAddress >= iBurnAddr)))
            //If .BLOCK is followed by a constant greater than 3.
         {
            vDotBlockOutputContinued(iDec - OBJ_CODE_LENGTH / BYTE_LENGTH);
         }
      }
      void vGenerateHexCode (bool asemList)
      {
         int i;
         int iDec = iHexWordToDecInt(cSecondArg);
         int lineCounter = 0;
         if (asemList)
         {
            switch (dotcom)
            {
               case eD_BLOCK:
                  if (iDec <= OBJ_CODE_LENGTH / BYTE_LENGTH)
                  {
                     for (i = 0; i < iDec; i++)
                     {
                        out_file << "00";
                        lineCounter = lineCounter + 2;
                     }
                     for (i = lineCounter; i <= OBJ_CODE_LENGTH; i++)
                     {
                        out_file << " ";
                     }
                  }
                  else
                  {
                     for (i = 0; i < OBJ_CODE_LENGTH / BYTE_LENGTH; i++)
                     {
                        out_file << "00";
                     }
                     out_file << " ";
                  }
                  break;
               case eD_BURN:
                  vBlankObjCodeColumn();
                  break;
               case eD_EQUATE:
                  vBlankObjCodeColumn();
                  break;
               case eD_WORD:
                  out_file << cSecondArg << "   ";
                  break;
            }
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               switch (dotcom)
               {
                  case eD_BLOCK:
                     for (i = 0; i < iDec; i++)
                     {
                        out_file << "00";
                        vHexOutputBufferLoader();
                     }
                     break;
                  case eD_BURN:
                     break;
                  case eD_EQUATE:
                     break;
                  case eD_WORD:
                     out_file << cSecondArg[0] << cSecondArg[1];
                     vHexOutputBufferLoader();
                     out_file << cSecondArg[2] << cSecondArg[3];
                     vHexOutputBufferLoader();
                     break;
               }
            }
         }
      }
};

class DotComByteHex : public Valid
{
   private:
      int iAddress;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[HEX_LENGTH + 1];
   public:
      DotComByteHex (int iAddr, DotCommand dot, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         dotcom = dot;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, HEX_LENGTH + 1);
      }
      int iAddressCounter () { return BYTE; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         out_file << "." << cFirstArg;
         vDotCommandBuffer(cFirstArg);
         out_file << "h#" << cSecondArg[2] << cSecondArg[3];
         char temp[3];
         temp[0] = '0';
         temp[1] = '0';
         temp[2] = '\0';
         vOperandBuffer (temp, true, false);
      }
      void vGenerateHexCode (bool asemList)
      {
         if (asemList)
         {
            out_file << cSecondArg[2] << cSecondArg[3] << "     ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cSecondArg[2] << cSecondArg[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

class DotComSym : public Valid
{
   private:
      int iAddress;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[IDENT_LENGTH + 1];
   public:
      DotComSym (int iAddr, DotCommand dot, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         dotcom = dot;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
      }
      int iAddressCounter () { return 2; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         out_file << "." << cFirstArg;
         vDotCommandBuffer(cFirstArg);
         out_file << cSecondArg;
         vOperandBuffer (cSecondArg, false, false);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cVal[ADDR_LENGTH + 1];
         vGetSymbolValue(cSecondArg, cVal);
         if (asemList)
         {
            out_file << cVal << "   ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cVal[0] << cVal[1];
               vHexOutputBufferLoader();
               out_file << cVal[2] << cVal[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

class DotComAscii : public Valid
{
   private:
      int iAddress;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[STRING_LENGTH + 1];
   public:
      DotComAscii (int iAddr, DotCommand dot, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         dotcom = dot;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, STRING_LENGTH + 1);
      }
      int iAddressCounter () 
      { 
         int i = 1;
         while (cSecondArg[i + 1] != '\0')
         {
            i++;
         }
         return i - 1; 
      }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         int i = 0;
         int j;
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         out_file << "." << cFirstArg;
         vDotCommandBuffer(cFirstArg);
         out_file << cSecondArg;
         while (cSecondArg[i] != '\0')
         {
            i++;
         }
         j = i;
         if (i - 1 <= IDENT_LENGTH + 1)
         {
            for (i = i - 1; i <= IDENT_LENGTH + 1; i++)
            {
               out_file << " ";
            }
         }
         else
         {
            out_file << " ";
         }
         if ((pComment != NULL) && (pComment->iLine == iSecPassCodeIndex))
         {
            if (pComment->bNonemptyLine)
            {
               if (pSymbol == NULL)
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1] = '\0';
               }
               else
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1] = '\0';
               }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p = pComment;
            pComment = pComment->pNext;
            delete p;
         }
         if ((j - 1 > OBJ_CODE_LENGTH / BYTE_LENGTH) && //If there are more than 3 characters
             ((iBurnCounter == 0) || (iAddress >= iBurnAddr)))
         {
            out_file << endl << "      ";
            vDotAsciiOutputContinued(cSecondArg);
         }
      }
      void vGenerateHexCode (bool asemList)
      {
         int i = 1;
         int iChar[STRING_LENGTH + 1];
         char cHex[BYTE_LENGTH + 1];
         int lineCounter = 0;
         while (cSecondArg[i + 1] != '\0')
         {
            iChar[i - 1] = static_cast <int> (cSecondArg[i]);
            i++;
         }
         if (asemList)
         {
            if (i - 1 <= OBJ_CODE_LENGTH / BYTE_LENGTH)
            {
               i = 0;
               while (cSecondArg[i + 2] != '\0') //Add two for arbitrary delimiters
               {
                  vDecToHexByte(iChar[i], cHex);
                  out_file << cHex;
                  i++;
                  lineCounter = lineCounter + 2; //A character in ascii takes 2 hex digits
               }
               for (i = lineCounter; i <= OBJ_CODE_LENGTH; i++)
               {
                  out_file << " ";
               }
            }
            else
            {
               for (i = 0; i < OBJ_CODE_LENGTH / BYTE_LENGTH; i++)
               {
                  vDecToHexByte(iChar[i], cHex);
                  out_file << cHex;
               }
               out_file << " ";
            }
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               i = 0;
               while (cSecondArg[i + 2] != '\0')
               {
                  vDecToHexByte(iChar[i], cHex);
                  out_file << cHex;
                  vHexOutputBufferLoader();
                  i++;
               }
            }
         }
      }
};

class InstructionDec : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[DEC_LENGTH + 1];
      char cThirdArg;
   public:
      InstructionDec (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg)
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, DEC_LENGTH + 1);
         cThirdArg = tArg;
      }
      ~InstructionDec () { delete pAMnemonic; }
      int iAddressCounter () { return NONUNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << "d#" << cSecondArg << "," << cThirdArg;
         vOperandBuffer (cSecondArg, true, true);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cByte[BYTE_LENGTH + 1];
         char cWord[HEX_LENGTH + 1];
         int iDec = iCharToInt(cSecondArg);
         vDecToHexWord(iDec, cWord);
         vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg), cByte);
         if (asemList)
         {
            out_file << cByte << cWord << " ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cByte;
               vHexOutputBufferLoader();
               out_file << cWord[0] << cWord[1];
               vHexOutputBufferLoader();
               out_file << cWord[2] << cWord[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

class InstructionDecNoAddr : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[DEC_LENGTH + 1];
   public:
      InstructionDecNoAddr (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, DEC_LENGTH + 1);
      }
      ~InstructionDecNoAddr () { delete pAMnemonic; }
      int iAddressCounter () { return NONUNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << "d#" << cSecondArg;
         vOperandBuffer (cSecondArg, true, false);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cByte[BYTE_LENGTH + 1];
         char cWord[HEX_LENGTH + 1];
         int iDec = iCharToInt(cSecondArg);
         vDecToHexWord(iDec, cWord);
         vDecToHexByte(pAMnemonic->iOpCode(), cByte);
         if (asemList)
         {
            out_file << cByte << cWord << " ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cByte;
               vHexOutputBufferLoader();
               out_file << cWord[0] << cWord[1];
               vHexOutputBufferLoader();
               out_file << cWord[2] << cWord[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

class InstructionChar : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[CHAR_LENGTH + 1];
      char cThirdArg;
   public:
      InstructionChar (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg)
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, CHAR_LENGTH + 1);
         cThirdArg = tArg;
      }
      ~InstructionChar () { delete pAMnemonic; }
      int iAddressCounter () { return NONUNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << "c#" << cSecondArg << "," << cThirdArg;
         vOperandBuffer (cSecondArg, true, true);
      }
      void vGenerateHexCode (bool asemList)
      {
         int iChar1 = static_cast <int> (cSecondArg[1]);
         int iChar2 = static_cast <int> (cSecondArg[2]);
         char cHexChar1[BYTE_LENGTH + 1];
         char cHexChar2[BYTE_LENGTH + 1];
         char cByte[BYTE_LENGTH + 1];
         vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg), cByte);
         vDecToHexByte(iChar1, cHexChar1);
         if (asemList)
         {
            if (cSecondArg[0] == cSecondArg[2])
            {
               out_file << cByte << "00" << cHexChar1 << " ";
            }
            else
            {
               vDecToHexByte(iChar2, cHexChar2);
               out_file << cByte << cHexChar1 << cHexChar2 << " ";
            }
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               if (cSecondArg[0] == cSecondArg[2])
               {
                  out_file << cByte;
                  vHexOutputBufferLoader();
                  out_file << "00";
                  vHexOutputBufferLoader();
                  out_file << cHexChar1;
               }
               else
               {
                  vDecToHexByte(iChar2, cHexChar2);
                  out_file << cByte;
                  vHexOutputBufferLoader();
                  out_file << cHexChar1;
                  vHexOutputBufferLoader();
                  out_file <<cHexChar2;
               }
               vHexOutputBufferLoader();
            }
         }
      }
};

class InstructionHex : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[HEX_LENGTH + 1];
      char cThirdArg;
   public:
      InstructionHex (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg)
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, HEX_LENGTH + 1);
         cThirdArg = tArg;
      }
      ~InstructionHex () { delete pAMnemonic; }
      int iAddressCounter () { return NONUNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << "h#" << cSecondArg << "," << cThirdArg;
         vOperandBuffer (cSecondArg, true, true);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cByte[BYTE_LENGTH + 1];
         vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg), cByte);
         if (asemList)
         {
            out_file << cByte << cSecondArg << " ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cByte;
               vHexOutputBufferLoader();
               out_file << cSecondArg[0] << cSecondArg[1];
               vHexOutputBufferLoader();
               out_file << cSecondArg[2] << cSecondArg[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

class InstructionHexNoAddr : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[HEX_LENGTH + 1];
   public:
      InstructionHexNoAddr (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, HEX_LENGTH + 1);
      }
      ~InstructionHexNoAddr () { delete pAMnemonic; }
      int iAddressCounter () { return NONUNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << "h#" << cSecondArg;
         vOperandBuffer (cSecondArg, true, false);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cByte[BYTE_LENGTH + 1];
         vDecToHexByte(pAMnemonic->iOpCode(), cByte);
         if (asemList)
         {
            out_file << cByte << cSecondArg << " ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cByte;
               vHexOutputBufferLoader();
               out_file << cSecondArg[0] << cSecondArg[1];
               vHexOutputBufferLoader();
               out_file << cSecondArg[2] << cSecondArg[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

class InstructionSym : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[IDENT_LENGTH + 1];
      char cThirdArg;
   public:
      InstructionSym (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[], char tArg)
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
         cThirdArg = tArg;
      }
      ~InstructionSym () { delete pAMnemonic; }
      int iAddressCounter () { return NONUNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << cSecondArg << "," << cThirdArg;
         vOperandBuffer (cSecondArg, false, true);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cTemp[HEX_LENGTH + 1];
         char cByte[BYTE_LENGTH + 1];
         vDecToHexByte(pAMnemonic->iOpCode() + iAddrModeValue(cThirdArg), cByte);
         vGetSymbolValue(cSecondArg, cTemp);
         if (asemList)
         {
            out_file << cByte << cTemp << " ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cByte;
               vHexOutputBufferLoader();
               out_file << cTemp[0] << cTemp[1];
               vHexOutputBufferLoader();
               out_file << cTemp[2] << cTemp[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

class InstructionSymNoAddr : public Valid
{
   private:
      int iAddress;
      AMnemon* pAMnemonic;
      char cFirstArg[IDENT_LENGTH + 1];
      char cSecondArg[IDENT_LENGTH + 1];
   public:
      InstructionSymNoAddr (int iAddr, Mnemon mn, AMnemon* pAMnemonTemp, char fArg[], char sArg[])
      {
         iAddress = iAddr;
         mnemonic = mn;
         pAMnemonic = pAMnemonTemp;
         strncpy (cFirstArg, fArg, IDENT_LENGTH + 1);
         strncpy (cSecondArg, sArg, IDENT_LENGTH + 1);
      }
      ~InstructionSymNoAddr () { delete pAMnemonic; }
      int iAddressCounter () { return NONUNARY; }
      void vBurnAddressChange () { iAddress = iAddress + iBurnStart; }
      void vGenerateCode ()
      {
         char cAddr[ADDR_LENGTH + 1];
         vDecToHexWord (iAddress, cAddr);
         out_file << cAddr << "  ";
         if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
         {
            vGenerateHexCode(true);
         }
         else
         {
            vBlankObjCodeColumn();
         }
         vOutputSymbolDecs();
         pAMnemonic->vMnemonOutput();
         out_file << cSecondArg;
         vOperandBuffer (cSecondArg, false, false);
      }
      void vGenerateHexCode (bool asemList)
      {
         char cTemp[HEX_LENGTH + 1];
         char cByte[BYTE_LENGTH + 1];
         vDecToHexByte(pAMnemonic->iOpCode(), cByte);
         vGetSymbolValue(cSecondArg, cTemp);
         if (asemList)
         {
            out_file << cByte << cTemp << " ";
         }
         else
         {
            if ((iBurnCounter == 0) || (iAddress >= iBurnAddr))
            {
               out_file << cByte;
               vHexOutputBufferLoader();
               out_file << cTemp[0] << cTemp[1];
               vHexOutputBufferLoader();
               out_file << cTemp[2] << cTemp[3];
               vHexOutputBufferLoader();
            }
         }
      }
};

//Table and object initializations
//////////////////////////////////////////////////////////////////////////////

//Initializes all global tables with their values
void vInitGlobalTables ()
{       
   strncpy (cDotTable[eD_ADDRSS], "ADDRSS", IDENT_LENGTH + 1);
   strncpy (cDotTable[eD_ASCII], "ASCII", IDENT_LENGTH + 1);
   strncpy (cDotTable[eD_BLOCK], "BLOCK", IDENT_LENGTH + 1);
   strncpy (cDotTable[eD_BURN], "BURN", IDENT_LENGTH + 1);
   strncpy (cDotTable[eD_BYTE], "BYTE", IDENT_LENGTH + 1);
   strncpy (cDotTable[eD_END], "END", IDENT_LENGTH + 1);
   strncpy (cDotTable[eD_EQUATE], "EQUATE", IDENT_LENGTH + 1);
   strncpy (cDotTable[eD_WORD], "WORD", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ADDA], "ADDA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ADDSP], "ADDSP", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ADDX], "ADDX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ANDA], "ANDA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ANDX], "ANDX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ASLA], "ASLA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ASLX], "ASLX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ASRA], "ASRA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ASRX], "ASRX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BR], "BR", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BRC], "BRC", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BREQ], "BREQ", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BRGE], "BRGE", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BRGT], "BRGT", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BRLE], "BRLE", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BRLT], "BRLT", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BRNE], "BRNE", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_BRV], "BRV", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_CHARI], "CHARI", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_CHARO], "CHARO", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_COMPA], "COMPA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_COMPX], "COMPX", IDENT_LENGTH + 1); 
   strncpy (cMnemonTable[eM_JSR], "JSR", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_LDBYTA], "LDBYTA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_LDBYTX], "LDBYTX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_LOADA], "LOADA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_LOADB], "LOADB", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_LOADX], "LOADX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_NOTA], "NOTA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_NOTX], "NOTX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ORA], "ORA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_ORX], "ORX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_RTI], "RTI", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_RTS], "RTS", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_STBYTA], "STBYTA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_STBYTX], "STBYTX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_STOP], "STOP", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_STOREA], "STOREA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_STOREX], "STOREX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_SUBA], "SUBA", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_SUBX], "SUBX", IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_UNIMP0], sUnimpMnemon[0].cID, IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_UNIMP1], sUnimpMnemon[1].cID, IDENT_LENGTH + 1);
   strncpy (cMnemonTable[eM_UNIMP2], sUnimpMnemon[2].cID, IDENT_LENGTH + 1);
}

//Initializes the object of mnemon found in vLookUpMnemon()
void vInitMnemonObjects  (Mnemon mnemon, AMnemon*& pAMnemonTemp)
{
   switch (mnemon)
   {
      case eM_ADDA:   pAMnemonTemp = new Adda (); break;
      case eM_ADDSP:  pAMnemonTemp = new Addsp (); break;
      case eM_ADDX:   pAMnemonTemp = new Addx (); break;
      case eM_ANDA:   pAMnemonTemp = new Anda (); break;
      case eM_ANDX:   pAMnemonTemp = new Andx (); break;
      case eM_ASLA:   pAMnemonTemp = new Asla (); break;
      case eM_ASLX:   pAMnemonTemp = new Aslx (); break;
      case eM_ASRA:   pAMnemonTemp = new Asra (); break;
      case eM_ASRX:   pAMnemonTemp = new Asrx (); break;
      case eM_BR:     pAMnemonTemp = new Br (); break;
      case eM_BRC:    pAMnemonTemp = new Brc (); break;
      case eM_BREQ:   pAMnemonTemp = new Breq (); break;
      case eM_BRGE:   pAMnemonTemp = new Brge (); break;
      case eM_BRGT:   pAMnemonTemp = new Brgt (); break;
      case eM_BRLE:   pAMnemonTemp = new Brle (); break;
      case eM_BRLT:   pAMnemonTemp = new Brlt (); break;
      case eM_BRNE:   pAMnemonTemp = new Brne (); break;
      case eM_BRV:    pAMnemonTemp = new Brv (); break;
      case eM_CHARI:  pAMnemonTemp = new Chari (); break;
      case eM_CHARO:  pAMnemonTemp = new Charo (); break;
      case eM_COMPA:  pAMnemonTemp = new Compa (); break;
      case eM_COMPX:  pAMnemonTemp = new Compx (); break;
      case eM_JSR:    pAMnemonTemp = new Jsr (); break;
      case eM_LDBYTA: pAMnemonTemp = new Ldbyta (); break;
      case eM_LDBYTX: pAMnemonTemp = new Ldbytx (); break;
      case eM_LOADA:  pAMnemonTemp = new Loada (); break;
      case eM_LOADB:  pAMnemonTemp = new Loadb (); break;
      case eM_LOADX:  pAMnemonTemp = new Loadx (); break;
      case eM_NOTA:   pAMnemonTemp = new Nota (); break;
      case eM_NOTX:   pAMnemonTemp = new Notx (); break;
      case eM_ORA:    pAMnemonTemp = new Ora (); break;
      case eM_ORX:    pAMnemonTemp = new Orx (); break;
      case eM_RTI:    pAMnemonTemp = new Rti (); break;
      case eM_RTS:    pAMnemonTemp = new Rts (); break;
      case eM_STBYTA: pAMnemonTemp = new Stbyta (); break;
      case eM_STBYTX: pAMnemonTemp = new Stbytx (); break;
      case eM_STOP:   pAMnemonTemp = new Stop (); break;
      case eM_STOREA: pAMnemonTemp = new Storea (); break;
      case eM_STOREX: pAMnemonTemp = new Storex (); break;
      case eM_SUBA:   pAMnemonTemp = new Suba (); break;
      case eM_SUBX:   pAMnemonTemp = new Subx (); break;
      case eM_UNIMP0: pAMnemonTemp = new Unimp0 (); break;
      case eM_UNIMP1: pAMnemonTemp = new Unimp1 (); break;
      case eM_UNIMP2: pAMnemonTemp = new Unimp2 (); break;
   }
}

//Table Search functions
//////////////////////////////////////////////////////////////////////////////

//Looks up to see if mn is a valid mnemonic
void vLookUpMnemon (char cID[], Mnemon& mn, AMnemon*& pAMnemonTemp, bool& bFnd)
{
   for (int i = 0; i <= IDENT_LENGTH; i++)
   {
      cID[i] = toupper (cID[i]);
   }
   strncpy (cMnemonTable[eM_EMPTY], cID, IDENT_LENGTH + 1);
   mn = eM_ADDA;
   while (strcmp (cMnemonTable[mn], cID) != 0)
   {
      mn = Mnemon (mn + 1);
   }
   bFnd = (mn != eM_EMPTY);
   if (bFnd)
   {
      vInitMnemonObjects(mn, pAMnemonTemp);
   }
}

//Looks up to see if dot is a valid dot command
void vLookUpDot (char cID[], DotCommand& dot, bool& bFnd)
{
   for (int i = 0; i <= IDENT_LENGTH; i++)
   {
      cID[i] = toupper (cID[i]);
   }
   strncpy (cDotTable[eD_EMPTY], cID, IDENT_LENGTH + 1);
   dot = eD_BLOCK;
   while (strcmp (cDotTable[dot], cID) != 0)
   {
      dot = DotCommand (dot + 1);
   }
   bFnd = (dot != eD_EMPTY);
   if (dot == eD_ASCII)
   {
      bIsAscii = true;
   }
}

//Symbol and Comment functions
//////////////////////////////////////////////////////////////////////////////

//Searches to see if cID[] has been declared
bool bLookUpSymbol (char cID[])
{
   sSymbolNode* p = pSymbol;
   while ((p != NULL) && (strcmp (cID, p->cSymID) > 0))
   {  
      p = p->pNext;
   }
   if (p != NULL)
   {
      return (strcmp (cID, p->cSymID) == 0);
   }
   else
   {
      return false;
   }
}

//Installs a symbol declaration in a linked list of symbols with their values
void vInstallSymbol (char cID[])
{
   sSymbolNode *p, *q;
   sSymbolNode* pTemp = new sSymbolNode;
   char addrHex[ADDR_LENGTH + 1];
   strncpy (pTemp->cSymID, cID, IDENT_LENGTH + 1);
   vDecToHexWord (iCurrentAddress, addrHex);
   strncpy (pTemp->cSymValue, addrHex, ADDR_LENGTH + 1);
   pTemp->iLine = iCodeIndex;
   q = NULL;
   p = pSymbol;
   while ((p != NULL) && (strcmp (cID, p->cSymID) > 0))
   {  
      q = p;  // q follows p.
      p = p->pNext;
   }
   if ((p != NULL) && (strcmp (cID, p->cSymID) == 0))
   {
      delete pACode[iCodeIndex];
      pACode[iCodeIndex] = new eSymPrevDef;
      delete pTemp;
      return;
   }
   pTemp->pNext = p;
   if ((q != NULL))
   {
      q->pNext = pTemp;   
   }
   else
   {
      pSymbol = pTemp;
   }
}

//Installs a symbol output declaration in a linked list of symbols with their lines
void vInstallSymbolOutput (char cID[])
{
   sSymbolOutputNode *p, *q;
   sSymbolOutputNode* pTemp = new sSymbolOutputNode;
   strncpy (pTemp->cSymID, cID, IDENT_LENGTH + 1);
   pTemp->iLine = iCodeIndex;
   q = NULL;
   p = pSymbolOutput;
   while (p != NULL)
   {  
      q = p;  // q follows p.
      p = p->pNext;
   }
   pTemp->pNext = p;
   if (q != NULL)
   {
      q->pNext = pTemp;   
   }
   else
   {
      pSymbolOutput = pTemp;
   }
}

//Changes cSymValue[] to cVal[] of the symbol named cID[] to account for .EQUATE
void vChangeSymValEquate (char cID[], char cVal[])
{
   sSymbolNode* p = pSymbol;
   while (p != NULL)
   {  
      if (strcmp (cID, p->cSymID) != 0)
      {
         p = p->pNext;
      }
      else
      {
         strncpy (p->cSymValue, cVal, ADDR_LENGTH + 1);
         return;
      }
   }
}

//Installs cVal[] and cID into the pEquate linked list
void vInstallEquateNode (char cID[], char cVal[])
{
   sEquateNode* p = new sEquateNode;
   strncpy (p->cSymValue, cVal, ADDR_LENGTH + 1);
   strncpy (p->cSymID, cID, IDENT_LENGTH + 1);
   p->pNext = pEquate;
   pEquate = p;
}

//Changes the value of every symbol to account for .BURN
void vChangeSymValBurn (int iBurnStartAddress)
{
   sSymbolNode* p = pSymbol;
   char cVal[ADDR_LENGTH + 1];
   while (p != NULL)
   {
      vDecToHexWord(iHexWordToDecInt (p->cSymValue) + iBurnStartAddress, cVal);
      strncpy (p->cSymValue, cVal, ADDR_LENGTH + 1);
      p = p->pNext;
   }
}

//Installs an undeclared symbol in a linked list of undeclared symbols with their values
void vInstallUndeclaredSymbol (char cID[])
{
   sUndeclaredsSymbolNode *p, *q;
   sUndeclaredsSymbolNode* pTemp = new sUndeclaredsSymbolNode;
   strncpy (pTemp->cSymID, cID, IDENT_LENGTH + 1);
   pTemp->iLine = iCodeIndex;
   pTemp->pNext = NULL;
   q = NULL;
   p = pUndeclaredSym;
   while (p != NULL)
   {  
      q = p;  // q follows p.
      p = p->pNext;
   }
   pTemp->pNext = p;
   if ((q != NULL))
   {
      q->pNext = pTemp;   
   }
   else
   {
      pUndeclaredSym = pTemp;
   }
}

//Installs a comment in a linked list of comments with their lines and values
void vInstallComment (char cID[], bool bNonempty)
{
   sCommentNode *p, *q;
   sCommentNode* pTemp = new sCommentNode;
   strncpy (pTemp->cComment, cID, COMMENT_LENGTH + 1);
   pTemp->bNonemptyLine = bNonempty;
   pTemp->iLine = iCodeIndex;
   pTemp->pNext = NULL;
   q = NULL;
   p = pComment;
   while (p != NULL)
   {  
      q = p;  // q follows p.
      p = p->pNext;
   }
   pTemp->pNext = p;
   if (q != NULL)
   {
      q->pNext = pTemp;   
   }
   else
   {
      pComment = pTemp;
   }
}

//Lexical Analyzer (finds tokens in the language)
//////////////////////////////////////////////////////////////////////////////

void vGetToken (AToken*& pAT)
{
   char cNextChar;
   int i;
   char cLocalIdentValue[IDENT_LENGTH + 1];
   char cLocalCommentValue[COMMENT_LENGTH + 1];
   char cLocalHexValue[HEX_LENGTH + 1];
   char cLocalDecValue[DEC_LENGTH + 1];
   char cLocalCharValue[CHAR_LENGTH + 1];
   char cLocalStringValue[STRING_LENGTH + 1];
   char cAddr;
   char cCharDelimiter;
   TDotCommand* pTDot = NULL;//Used to detect .ASCII for strings
   State state = eS_START;
   pAT = new TEmpty;
   do
   {
      vAdvanceInput (cNextChar);
      switch (state)
      {
         case eS_START:
            if (bIsAscii)
            {
               if (cNextChar != '\n')
               {
                  i = 0;
                  state = eS_STRING1;
                  bIsAscii = false;
               }
               else
               {
                  delete pAT;
                  pAT = new TInvalidString;
               }
            }
            else if (cNextChar == '.')
            {
               state = eS_DOT1;
            }
            else if ((cNextChar == 'd') || (cNextChar == 'D'))
            {
               cLocalIdentValue[0] = cNextChar;
               i = 1;
               state = eS_DECIMAL1;
            }
            else if ((cNextChar == 'h') || (cNextChar == 'H'))
            {
               cLocalIdentValue[0] = cNextChar;
               i = 1;
               state = eS_HEX1;
            }
            else if ((cNextChar == 'c') || (cNextChar == 'C'))
            {
               cLocalIdentValue[0] = cNextChar;
               i = 1;
               state = eS_CHAR1;
            }
            else if (cNextChar == ',')
            {
               state = eS_ADDR1;
            }
            else if (isalpha (cNextChar))
            {
               cLocalIdentValue[0] = cNextChar;
               i = 1;
               state = eS_IDENT;
            }
            else if (cNextChar == ';')
            {
               i = 0;
               state = eS_COMMENT;      
            }
            else if (cNextChar == '\n')
            {
               state = eS_STOP;
            }
            else if ((cNextChar != ' ') && (cNextChar != '\t'))
            {
               delete pAT;
               pAT = new TInvalid;
            }
            break;
         case eS_IDENT:
            if (isalpha (cNextChar) || isdigit (cNextChar))
            {
               if (i < IDENT_LENGTH)
               {
                  cLocalIdentValue[i++] = cNextChar;
               }
            }
            else if (cNextChar == ':')
            {
               cLocalIdentValue[i] = '\0';
               delete pAT;
               pAT = new TSymbol (cLocalIdentValue);
               state = eS_STOP;
            }
            else
            {
               cLocalIdentValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TIdentifier (cLocalIdentValue);
               state = eS_STOP;
            }
            break;
         case eS_DOT1:
            if (isalpha (cNextChar))
            {
               cLocalIdentValue[0] = cNextChar;
               i = 1;
               state = eS_DOT2;
            }
            else
            {
               delete pAT;
               pAT = new TInvalid;
            }
            break;
         case eS_DOT2:
            if (isalpha (cNextChar) || isdigit (cNextChar))
            {
               if (i < IDENT_LENGTH)
               {
                  cLocalIdentValue[i++] = cNextChar;
               }
            }
            else
            {
               cLocalIdentValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TDotCommand (cLocalIdentValue);
               state = eS_STOP;
            }
            break;
         case eS_DECIMAL1:
            if (isalpha (cNextChar) || isdigit (cNextChar))
            {
               cLocalIdentValue[i++] = cNextChar;
               state = eS_IDENT;
            }
            else if (cNextChar == '#')
            {
               state = eS_DECIMAL2;
               i = 0;
               for (int j = 0; j <= DEC_LENGTH; j++)
               {
                  cLocalDecValue[j] = '\0';
               }
            }
            else if (cNextChar == ':')
            {
               cLocalIdentValue[i] = '\0';
               delete pAT;
               pAT = new TSymbol (cLocalIdentValue);
               state = eS_STOP;
            }
            else
            {
               cLocalIdentValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TIdentifier (cLocalIdentValue);
               state = eS_STOP;
            }
            break;
         case eS_DECIMAL2:
            if (isdigit (cNextChar))
            {
               cLocalDecValue[i++] = cNextChar;
               state = eS_DECIMAL3;
            }
            else if ((cNextChar == '+') || (cNextChar == '-'))
            {
               if (cNextChar == '-')
               {
                  cLocalDecValue[i++] = cNextChar;
               }
               state = eS_SIGN;
            }
            else if ((cNextChar != ' ') && (cNextChar != '\t'))
            {
               vBackUpInput ();
               delete pAT;
               pAT = new TInvalidDec;
               state = eS_STOP;
            }
            break;
         case eS_SIGN:
            if (isdigit(cNextChar))
            {
               cLocalDecValue[i++] = cNextChar;
               state = eS_DECIMAL3;
            }
            else
            {
               vBackUpInput ();
               delete pAT;
               pAT = new TInvalidDec;
               state = eS_STOP;
            }
            break;
         case eS_DECIMAL3:
            if (isdigit (cNextChar))
            {
               if (i < DEC_LENGTH)
               {
                  cLocalDecValue[i++] = cNextChar;
               }
               else
               {
                  vBackUpInput ();
                  delete pAT;
                  pAT = new TDecConstant (cLocalDecValue);
                  state = eS_STOP;
               }
            }
            else
            {
               vBackUpInput ();
               delete pAT;
               pAT = new TDecConstant (cLocalDecValue);
               state = eS_STOP;
            }
            break;
         case eS_HEX1:
            if ((isdigit (cNextChar)) || (isalpha (cNextChar)))
            {
               cLocalIdentValue[i++] = cNextChar;
               state = eS_IDENT;
            }
            else if (cNextChar == '#')
            {
               state = eS_HEX2;
            }
            else if (cNextChar == ':')
            {
               cLocalIdentValue[i] = '\0';
               delete pAT;
               pAT = new TSymbol (cLocalIdentValue);
               state = eS_STOP;
            }
            else
            {
               cLocalIdentValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TIdentifier (cLocalIdentValue);
               state = eS_STOP;
            }
            break;
         case eS_HEX2:
            if ((isdigit (cNextChar)) || ((cNextChar >= 'a') && (cNextChar <= 'f'))
                || ((cNextChar >= 'A') && (cNextChar <= 'F')))
            {
               cNextChar = toupper (cNextChar);
               cLocalHexValue[0] = '0';
               cLocalHexValue[1] = '0';
               cLocalHexValue[2] = '0';
               cLocalHexValue[3] = cNextChar;
               cLocalHexValue[4] = '\0';
               state = eS_HEX3;
               i = 1;
            }
            else if ((cNextChar != ' ') && (cNextChar != '\t'))
            {
               vBackUpInput ();
               delete pAT;
               pAT = new TInvalidHex;
               state = eS_STOP;
            }
            break;
         case eS_HEX3:
            cNextChar = toupper (cNextChar);
            if ((isdigit (cNextChar)) || ((cNextChar >= 'a') && (cNextChar <= 'f'))
                || ((cNextChar >= 'A') && (cNextChar <= 'F')))
            {
               if (i < HEX_LENGTH)
               {
                  cLocalHexValue[0] = cLocalHexValue[1];
                  cLocalHexValue[1] = cLocalHexValue[2];
                  cLocalHexValue[2] = cLocalHexValue[3];
                  cLocalHexValue[3] = cNextChar;
               }
               else
               {
                  vBackUpInput ();
                  delete pAT;
                  pAT = new THexConstant (cLocalHexValue);
                  state = eS_STOP;
               }
               i++;
            }
            else
            {
               vBackUpInput ();
               delete pAT;
               pAT = new THexConstant (cLocalHexValue);
               state = eS_STOP;
            }
            break;
         case eS_CHAR1:
            if ((isdigit (cNextChar)) || (isalpha (cNextChar)))
            {
               cLocalIdentValue[i++] = cNextChar;
               state = eS_IDENT;
            }
            else if (cNextChar == '#')
            {
               i = 0;
               state = eS_CHAR2;
            }
            else if (cNextChar == ':')
            {
               cLocalIdentValue[i] = '\0';
               delete pAT;
               pAT = new TSymbol (cLocalIdentValue);
               state = eS_STOP;
            }
            else
            {
               cLocalIdentValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TIdentifier (cLocalIdentValue);
               state = eS_STOP;
            }
            break;
         case eS_CHAR2:
            if ((cNextChar != ' ') && (cNextChar != '\t') && (cNextChar != '\n'))
            {
               cCharDelimiter = cNextChar;
               cLocalCharValue[i++] = cNextChar;
               state = eS_CHAR3;
            }
            else if ((cNextChar != ' ') && (cNextChar != '\t'))
            {
               vBackUpInput ();
               delete pAT;
               pAT = new TInvalidChar;
               state = eS_STOP;
            }
            break;
         case eS_CHAR3:
            if ((cNextChar != '\n') && (cNextChar != cCharDelimiter))
            {
               if (i < CHAR_LENGTH)
               {
                  cLocalCharValue[i++] = cNextChar;
               }
            }
            if (cNextChar == '\n')
            {
               cLocalCharValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TCharConstant (cLocalCharValue);
               state = eS_STOP;
            }
            if (cNextChar == cCharDelimiter)
            {
               if (i < CHAR_LENGTH)
               {
                  cLocalCharValue[i++] = cNextChar;
                  cLocalCharValue[i] = '\0';
                  delete pAT;
                  pAT = new TCharConstant (cLocalCharValue);
                  state = eS_STOP;
               }
               else
               {
                  cLocalCharValue[i] = '\0';
                  delete pAT;
                  pAT = new TCharConstant (cLocalCharValue);
                  state = eS_STOP;
               }
            }
            break;
         case eS_STRING1:
            if ((cNextChar != ' ') && (cNextChar != '\t') && (cNextChar != '\n'))
            {
               cCharDelimiter = cNextChar;
               cLocalStringValue[i++] = cNextChar;
               state = eS_STRING2;
            }
            else if ((cNextChar != ' ') && (cNextChar != '\t'))
            {
               vBackUpInput ();
               delete pAT;
               pAT = new TInvalidString;
               state = eS_STOP;
            }
            break;
         case eS_STRING2:
            if ((cNextChar != '\n') && (cNextChar != cCharDelimiter))
            {
               if (i < STRING_LENGTH)
               {
                  cLocalStringValue[i++] = cNextChar;
               }
            }
            if (cNextChar == '\n')
            {
               cLocalStringValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TString (cLocalStringValue);
               state = eS_STOP;
            }
            if (cNextChar == cCharDelimiter)
            {
               if (i < STRING_LENGTH)
               {
                  cLocalStringValue[i++] = cNextChar;
                  cLocalStringValue[i] = '\0';
                  delete pAT;
                  pAT = new TString (cLocalStringValue);
                  state = eS_STOP;
               }
               else
               {
                  cLocalStringValue[i] = '\0';
                  delete pAT;
                  pAT = new TString (cLocalStringValue);
                  state = eS_STOP;
               }
            }
            break;
         case eS_ADDR1:
            if ((cNextChar == 'i') || (cNextChar == 'I'))  
            {
               state = eS_ADDR2;
               cAddr = 'i';
            }
            else if ((cNextChar == 'd') || (cNextChar == 'D'))
            {
               state = eS_ADDR2;
               cAddr = 'd';
            }
            else if ((cNextChar == 'x') || (cNextChar == 'X'))
            {
               state = eS_ADDR2;
               cAddr = 'x';
            }
            else if ((cNextChar == 's') || (cNextChar == 'S'))
            {
               state = eS_ADDR2;
               cAddr = 's';
            }
            else if ((cNextChar != ' ') && (cNextChar != '\t'))
            {
               delete pAT;
               pAT = new TInvalid;
            }
            break;
         case eS_ADDR2:
            vBackUpInput ();
            delete pAT;
            pAT = new TAddress (cAddr);
            state = eS_STOP;
            break;
         case eS_COMMENT:
            if (cNextChar == '\n')
            {
               cLocalCommentValue[i] = '\0';
               vBackUpInput ();
               delete pAT;
               pAT = new TComment (cLocalCommentValue);
               state = eS_STOP;
            }
            else
            {
               if (i < COMMENT_LENGTH)
               {
                  cLocalCommentValue[i++] = cNextChar;
               }
            }
            break;
      }
   }
   while ((state != eS_STOP) && (pAT->kTokenType () != eT_INVALID));
   pPrevAT = pAT;
}

//Parser
//////////////////////////////////////////////////////////////////////////////
//Determines whether a string of tokens is a valid line of assembly language
//code using a FSM implementation.


void vProcessSourceLine (bool& term)
{
   char cLocalCommentVal[COMMENT_LENGTH + 1];
   char cLocalStringVal[STRING_LENGTH + 1];
   char cLocalSymVal[IDENT_LENGTH + 1];
   char cLocalFirstVal[IDENT_LENGTH + 1];
   char cLocalSecondVal[IDENT_LENGTH + 1];
   char cLocalThirdVal;
   char cAddressingModes[ADDR_MODES + 1];
   bool bSymDeclared = false;
   TIdentifier* pTIdent = NULL;
   TDotCommand* pTDot = NULL;
   TAddress* pTAddress = NULL;
   THexConstant* pTHex = NULL;
   TDecConstant* pTDec = NULL;
   TCharConstant* pTChar = NULL;
   TSymbol* pTSym = NULL;
   TString* pTString = NULL;
   TComment* pTComm = NULL;
   AToken* pAToken = NULL;
   Valid* pValid = NULL;
   AMnemon* pAMnemonTemp = NULL;
   Mnemon mnemon;
   DotCommand dotcom;
   int iTemp;
   bool bFound;
   pACode[iCodeIndex] = new ZeroArg (eD_EMPTY);
   ParseState psState = ePS_START;
   do
   {
      vGetToken (pAToken);
      switch (psState)
      {
         case ePS_START:
            if (pAToken->kTokenType () == eT_IDENTIFIER)
            {
               pTIdent = static_cast <TIdentifier*> (pAToken);
               pTIdent->vGetValue (cLocalFirstVal);
               vLookUpMnemon (cLocalFirstVal, mnemon, pAMnemonTemp, bFound);
               if (bFound)
               {
                  if (pAMnemonTemp->bIsUnary())
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new UnaryInstruction (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     psState = ePS_INSTRUCTION;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eInvMnemon;
               }
            }
            else if (pAToken->kTokenType () == eT_DOTCOMMAND)
            {
               pTDot = static_cast <TDotCommand*> (pAToken);
               pTDot->vGetValue (cLocalFirstVal);
               vLookUpDot (cLocalFirstVal, dotcom, bFound);
               if (bFound)
               {
                  if (dotcom == eD_END)
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new DotEnd (iCurrentAddress, eD_END, cLocalFirstVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     term = true;
                     psState = ePS_CLOSE;
                  }
                  else if (dotcom == eD_ASCII)
                  {
                     psState = ePS_STRING;
                  }
                  else
                  {
                     psState = ePS_DOTCOMMAND;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eNoDotCom;
               }
            }
            else if (pAToken->kTokenType () == eT_SYMBOL)
            {
               pTSym = static_cast <TSymbol*> (pAToken);
               pTSym->vGetValue (cLocalSymVal);
               vInstallSymbol (cLocalSymVal);
               vInstallSymbolOutput (cLocalSymVal);
               psState = ePS_SYMBOLDEC;
            }
            else if (pAToken->kTokenType () == eT_EMPTY)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new ZeroArg (eD_EMPTY);
               psState = ePS_FINISH;
            }
            else if (pAToken->kTokenType () == eT_COMMENT)
            {
               pTComm = static_cast <TComment*> (pAToken);
               pTComm->vGetValue (cLocalCommentVal);
               vInstallComment (cLocalCommentVal, false);
               psState = ePS_COMMENT;
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eSymInstrDotExp;
            }
            break;
         case ePS_SYMBOLDEC:
            bSymDeclared = true;
            if (pAToken->kTokenType () == eT_IDENTIFIER)
            {
               pTIdent = static_cast <TIdentifier*> (pAToken);
               pTIdent->vGetValue (cLocalFirstVal);
               vLookUpMnemon (cLocalFirstVal, mnemon, pAMnemonTemp, bFound);
               if (bFound)
               {
                  if (pAMnemonTemp->bIsUnary())
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new UnaryInstruction (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     psState = ePS_INSTRUCTION;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eInvMnemon;
               }
            }
            else if (pAToken->kTokenType () == eT_DOTCOMMAND)
            {
               pTDot = static_cast <TDotCommand*> (pAToken);
               pTDot->vGetValue (cLocalFirstVal);
               vLookUpDot (cLocalFirstVal, dotcom, bFound);
               if (bFound)
               {
                  if (dotcom == eD_END)
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new DotEnd (iCurrentAddress, eD_END, cLocalFirstVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     term = true;
                     psState = ePS_CLOSE;
                  }
                  else if (dotcom == eD_EQUATE)
                  {
                     psState = ePS_EQUATE;
                  }
                  else if (dotcom == eD_ASCII)
                  {
                     psState = ePS_STRING;
                  }
                  else
                  {
                     psState = ePS_DOTCOMMAND;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eNoDotCom;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInstrDotExp;
            }
            break;
         case ePS_DOTCOMMAND:
            if (pAToken->kTokenType () == eT_IDENTIFIER)
            {
               pTIdent = static_cast <TIdentifier*> (pAToken);
               pTIdent->vGetValue (cLocalSecondVal);
               vInstallUndeclaredSymbol(cLocalSecondVal);
               if (dotcom == eD_ADDRSS)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new DotComSym (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_CLOSE;
               }
               else if (dotcom == eD_EQUATE)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eSymBeforeEquate;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eDecHexExp;
               }
            }
            else if (pAToken->kTokenType () == eT_HEXCONSTANT)
            {
               pTHex = static_cast <THexConstant*> (pAToken);
               pTHex->vGetValue (cLocalSecondVal);
               if (dotcom == eD_ADDRSS)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eSymExpWithAddrss;
               }
               else if (dotcom == eD_BLOCK)
               {
                  delete pACode[iCodeIndex];
                  if ((cLocalSecondVal[0] != '0') || (cLocalSecondVal[1] != '0'))
                  {
                     pACode[iCodeIndex] = new eConstOverflow;
                  }
                  else
                  {
                     pACode[iCodeIndex] = new DotComHex (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
               }
               else if (dotcom == eD_BURN)
               {
                  if (iBurnCounter == 0)
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new DotComHex (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iBurnAddr = iCurrentAddress;
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                     iBurnStart = iHexWordToDecInt(cLocalSecondVal);
                     iBurnCounter++;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eOneBurn;
                  }
               }
               else if (dotcom == eD_BYTE)
               {
                  if ((cLocalSecondVal[0] == '0') && (cLocalSecondVal[1] == '0'))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new DotComByteHex (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eByteOutOfRange;
                  }
               }
               else if (dotcom == eD_EQUATE)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eSymBeforeEquate;
               }
               else if (dotcom == eD_WORD)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new DotComHex (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_CLOSE;
               }
            }
            else if (pAToken->kTokenType () == eT_DECCONSTANT)
            {
               pTDec = static_cast <TDecConstant*> (pAToken);
               pTDec->vGetValue (cLocalSecondVal);
               if (dotcom == eD_ADDRSS)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eSymExpWithAddrss;
               }
               else if (dotcom == eD_BLOCK)
               {
                  iTemp = iCharToInt (cLocalSecondVal);
                  if ((iTemp >= 0) && (iTemp <= MAX_BYTE))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new DotComDec (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eConstOverflow;
                  }
               }
               else if (dotcom == eD_BURN)
               {
                  if (iBurnCounter == 0)
                  {
                     iTemp = iCharToInt (cLocalSecondVal);
                     if ((iTemp >= 0) && (iTemp <= MAX_ADDR))
                     {
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex] = new DotComDec (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                        pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                        iBurnAddr = iCurrentAddress;
                        iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                        psState = ePS_CLOSE;
                        iBurnStart = iCharToInt(cLocalSecondVal);
                        iBurnCounter++;
                     }
                     else 
                     {
                        delete pACode[iCodeIndex];
                        pACode[iCodeIndex] = new eAddrOverflow;
                     }
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eOneBurn;
                  }
               }
               else if (dotcom == eD_BYTE)
               {
                  iTemp = iCharToInt (cLocalSecondVal);
                  if ((iTemp >= MIN_BYTE) && (iTemp <= MAX_BYTE))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new DotComDec (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eByteOutOfRange;
                  }
               }
               else if (dotcom == eD_EQUATE)
               {
                  iTemp = iCharToInt (cLocalSecondVal);
                  if ((iTemp >= MIN_DEC) && (iTemp <= MAX_DEC))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eSymBeforeEquate;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eDecOverflow;
                  }
               }
               else if (dotcom == eD_WORD)
               {
                  iTemp = iCharToInt (cLocalSecondVal);
                  if ((iTemp >= MIN_DEC) && (iTemp <= MAX_DEC))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new DotComDec (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eDecOverflow;
                  }
               }
            }
            else if (pAToken->kTokenType () == eT_CHARCONSTANT)
            {
               pTChar = static_cast <TCharConstant*> (pAToken);
               pTChar->vGetValue (cLocalSecondVal);
               if (dotcom == eD_ADDRSS)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eSymExpWithAddrss;
               }
               else if (dotcom == eD_EQUATE)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eSymBeforeEquate;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eDecHexExp;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALIDDEC)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eNoDecConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDHEX)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eNoHexConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCHAR)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eNoCharConst;
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eDecHexExp;
            }
            break;
         case ePS_STRING:
            if (pAToken->kTokenType () == eT_STRING)
            {
               pTString = static_cast <TString*> (pAToken);
               pTString->vGetValue (cLocalStringVal);
               psState = ePS_CLOSE;
               int i = 0;
               while (cLocalStringVal[i] != '\0')
               {
                  i++;
               }
               if (cLocalStringVal[0] == cLocalStringVal[1])
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eNoString;
               }
               else if (cLocalStringVal[0] == cLocalStringVal[i - 1])
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new DotComAscii (iCurrentAddress, dotcom, cLocalFirstVal, cLocalStringVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_CLOSE;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eNoString;
               }
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eNoString;
            }
            break;
         case ePS_EQUATE:
            if (pAToken->kTokenType () == eT_HEXCONSTANT)
            {
               pTHex = static_cast <THexConstant*> (pAToken);
               pTHex->vGetValue (cLocalSecondVal);
               delete pACode[iCodeIndex];
               vChangeSymValEquate(cLocalSymVal, cLocalSecondVal);
               vInstallEquateNode(cLocalSymVal, cLocalSecondVal);
               pACode[iCodeIndex] = new DotComHex (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
               pValid = static_cast <Valid*> (pACode[iCodeIndex]);
               iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
               psState = ePS_CLOSE;
            }
            else if (pAToken->kTokenType () == eT_DECCONSTANT)
            {
               char cVal[ADDR_LENGTH + 1];
               pTDec = static_cast <TDecConstant*> (pAToken);
               pTDec->vGetValue (cLocalSecondVal);
               int iTemp = iCharToInt(cLocalSecondVal);
               vDecToHexWord(iTemp, cVal);
               vChangeSymValEquate(cLocalSymVal, cVal);
               vInstallEquateNode(cLocalSymVal, cVal);
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new DotComDec (iCurrentAddress, dotcom, cLocalFirstVal, cLocalSecondVal);
               pValid = static_cast <Valid*> (pACode[iCodeIndex]);
               iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
               psState = ePS_CLOSE;
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else
            { 
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eDecHexExp;
            }
            break;
         case ePS_INSTRUCTION:
            if (pAToken->kTokenType () == eT_IDENTIFIER)
            {
               pTIdent = static_cast <TIdentifier*> (pAToken);
               pTIdent->vGetValue (cLocalSecondVal);
               vInstallUndeclaredSymbol(cLocalSecondVal);
               psState = ePS_OPRNDSPECSYM;
            }
            else if (pAToken->kTokenType () == eT_HEXCONSTANT)
            {
               pTHex = static_cast <THexConstant*> (pAToken);
               pTHex->vGetValue (cLocalSecondVal);
               psState = ePS_OPRNDSPECHEX;
            }
            else if (pAToken->kTokenType () == eT_DECCONSTANT)
            {
               pTDec = static_cast <TDecConstant*> (pAToken);
               pTDec->vGetValue (cLocalSecondVal);
               iTemp = iCharToInt (cLocalSecondVal);
               if ((iTemp >= MIN_DEC) && (iTemp <= MAX_DEC))
               {
                  psState = ePS_OPRNDSPECDEC;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eDecOverflow;
               }
            }
            else if (pAToken->kTokenType () == eT_CHARCONSTANT)
            {
               pTChar = static_cast <TCharConstant*> (pAToken);
               pTChar->vGetValue (cLocalSecondVal);
               if ((cLocalSecondVal[0] == cLocalSecondVal[CHAR_LENGTH - 1]) ||
                   (cLocalSecondVal[0] == cLocalSecondVal[CHAR_LENGTH - 2]))
               {
                  psState = ePS_OPRNDSPECCHAR;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eNoCharConst;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALIDDEC)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eNoDecConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDHEX)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eNoHexConst;
            }
            else if (pAToken->kTokenType () == eT_INVALIDCHAR)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eNoCharConst;
            }
            else if (pAToken->kTokenType () == eT_ADDRMODE)
            {
               pTAddress = static_cast <TAddress*> (pAToken);
               pTAddress->vGetValue (cLocalThirdVal);
               if (cLocalThirdVal == 'x')
               {
                  pAMnemonTemp->vAddrModes(cAddressingModes);
                  if (bSearchAddrModes(cAddressingModes, cLocalThirdVal))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new IndexedAddrInstruction (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalThirdVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eNoAddrmode;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eNoAddr;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eOprndSpecExp;
            }
            break;
         case ePS_OPRNDSPECDEC:
            if (pAToken->kTokenType () == eT_ADDRMODE)
            {
               pTAddress = static_cast <TAddress*> (pAToken);
               pTAddress->vGetValue (cLocalThirdVal);
               if (cLocalThirdVal != 'x')
               {
                  pAMnemonTemp->vAddrModes(cAddressingModes);
                  if (bSearchAddrModes(cAddressingModes, cLocalThirdVal))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new InstructionDec (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal, cLocalThirdVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eNoAddrmode;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eXAddrUnary;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired())
            {
               if (pAToken->kTokenType () == eT_EMPTY)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new InstructionDecNoAddr (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_FINISH;
               }
               else if (pAToken->kTokenType () == eT_COMMENT)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new InstructionDecNoAddr (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_FINISH;
                  pTComm = static_cast <TComment*> (pAToken);
                  pTComm->vGetValue (cLocalCommentVal);
                  iCurrentAddress = iCurrentAddress - pValid->iAddressCounter();
                  vInstallComment (cLocalCommentVal,true);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_COMMENT;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eAddrCommExp;
               }
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eAddrExp;
            }
            break;
         case ePS_OPRNDSPECHEX:
            if (pAToken->kTokenType () == eT_ADDRMODE)
            {
               pTAddress = static_cast <TAddress*> (pAToken);
               pTAddress->vGetValue (cLocalThirdVal);
               if (cLocalThirdVal != 'x')
               {
                  pAMnemonTemp->vAddrModes(cAddressingModes);
                  if (bSearchAddrModes(cAddressingModes, cLocalThirdVal))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new InstructionHex (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal, cLocalThirdVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eNoAddrmode;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eXAddrUnary;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired())
            {
               if (pAToken->kTokenType () == eT_EMPTY)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new InstructionHexNoAddr (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_FINISH;
               }
               else if (pAToken->kTokenType () == eT_COMMENT)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new InstructionHexNoAddr (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_FINISH;
                  pTComm = static_cast <TComment*> (pAToken);
                  pTComm->vGetValue (cLocalCommentVal);
                  iCurrentAddress = iCurrentAddress - pValid->iAddressCounter();
                  vInstallComment (cLocalCommentVal, true);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_COMMENT;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eAddrCommExp;
               }
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eAddrExp;
            }
            break;
         case ePS_OPRNDSPECCHAR:
            if (pAToken->kTokenType () == eT_ADDRMODE)
            {
               pTAddress = static_cast <TAddress*> (pAToken);
               pTAddress->vGetValue (cLocalThirdVal);
               if (cLocalThirdVal != 'x')
               {
                  pAMnemonTemp->vAddrModes(cAddressingModes);
                  if (bSearchAddrModes(cAddressingModes, cLocalThirdVal))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new InstructionChar (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal, cLocalThirdVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eNoAddrmode;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eXAddrUnary;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired())
            {
               pACode[iCodeIndex] = new eNoCharWithInstruct;
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eAddrExp;
            }
            break;
         case ePS_OPRNDSPECSYM:
            if (pAToken->kTokenType () == eT_ADDRMODE)
            {
               pTAddress = static_cast <TAddress*> (pAToken);
               pTAddress->vGetValue (cLocalThirdVal);
               if (cLocalThirdVal != 'x')
               {
                  pAMnemonTemp->vAddrModes(cAddressingModes);
                  if (bSearchAddrModes(cAddressingModes, cLocalThirdVal))
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new InstructionSym (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal, cLocalThirdVal);
                     pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                     iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                     psState = ePS_CLOSE;
                  }
                  else
                  {
                     delete pACode[iCodeIndex];
                     pACode[iCodeIndex] = new eNoAddrmode;
                  }
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eXAddrUnary;
               }
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else if (pAMnemonTemp->bNoAddrModeRequired())
            {
               if (pAToken->kTokenType () == eT_EMPTY)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new InstructionSymNoAddr (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_FINISH;
               }
               else if (pAToken->kTokenType () == eT_COMMENT)
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new InstructionSymNoAddr (iCurrentAddress, mnemon, pAMnemonTemp, cLocalFirstVal, cLocalSecondVal);
                  pValid = static_cast <Valid*> (pACode[iCodeIndex]);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_FINISH;
                  pTComm = static_cast <TComment*> (pAToken);
                  pTComm->vGetValue (cLocalCommentVal);
                  iCurrentAddress = iCurrentAddress - pValid->iAddressCounter();
                  vInstallComment (cLocalCommentVal, true);
                  iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
                  psState = ePS_COMMENT;
               }
               else
               {
                  delete pACode[iCodeIndex];
                  pACode[iCodeIndex] = new eAddrCommExp;
               }
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eAddrExp;
            }
            break;
         case ePS_COMMENT:
            if (pAToken->kTokenType () == eT_EMPTY)
            {
               psState = ePS_FINISH;
            }
            break;
         case ePS_CLOSE:
            if (pAToken->kTokenType () == eT_EMPTY)
            {
               psState = ePS_FINISH;
            }
            else if (pAToken->kTokenType () == eT_COMMENT)
            {
               pTComm = static_cast <TComment*> (pAToken);
               pTComm->vGetValue (cLocalCommentVal);
               iCurrentAddress = iCurrentAddress - pValid->iAddressCounter();
               vInstallComment (cLocalCommentVal, true);
               iCurrentAddress = iCurrentAddress + pValid->iAddressCounter();
               psState = ePS_COMMENT;
            }
            else if (pAToken->kTokenType () == eT_INVALID)
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eInvSyntax;
            }
            else
            {
               delete pACode[iCodeIndex];
               pACode[iCodeIndex] = new eCommExp;
            }
            break;
      }
      delete pAToken;
      if (iCodeIndex >= MAX_LINES)
      {
         delete pACode[iCodeIndex];
         pACode[iCodeIndex] = new eTooLong;
         term = true;
      }
      if (iCurrentAddress >= CODE_MAX_SIZE - 2)
      {
         delete pACode[iCodeIndex];
         pACode[iCodeIndex] = new eProgTooLong;
         term = true;
      }
   }
   while ((psState != ePS_FINISH) && (!pACode[iCodeIndex]->bIsError ()));
}

int main (int argc, char *argv[])
{
   bool bTerminate = false;
   iCodeIndex = 0;
   int iLineErrors[MAX_LINES]; //Keeps track of lines containing errors
   int iErrorIndex = 0; //Index for iLineErrors[]
   Valid* pValid;
   int i;
   int j;
   char sourceFileName[FILE_NAME_LENGTH];
   char objectFileName[FILE_NAME_LENGTH];
   char listingFileName[FILE_NAME_LENGTH];

//   int iExtension;
   bool bTemp = false;
   bool bListing = false;
   bool bVersion = false;
   for (j = 0; j <= MAX_LINES; j++) //Initialize pACode array
   {
      pACode[j] = NULL;
   }
   //Input mnemon file
   in_file.open("mnemon");
   if (in_file.fail())
   {
      cerr << "Could not open mnemon file." << endl;
      return 1;
   }
   for (i = 0; i < UNIMPLEMENTED_INSTRUCTIONS; i++)
   {
      vGetMnemonLine(i);
   }
   i = 0;
   in_file.close();
   //
   //Analyze input command

   if (argc == 1)
   {
      return 0;
   }
   else if (argc == 2)
   {
      if (argv[1][0] == '-')
      {
         if (strcmp(argv[1], "-v") == 0)
         {
            vVersionNumber();
            return 0;
         }
         else
         {
            cerr << "usage: asem7 [-v] [[-l] sourceFile]" << endl;
            return 2;
         }
      }
      else
      {
         if (strlen(argv[1]) > FILE_NAME_LENGTH - 3)
         {
            cerr << "Source file name too long" << endl;
            return 2;
         }
         else
         {
            strncpy (sourceFileName, argv[1], FILE_NAME_LENGTH);
         }
      }
   }
   else if (argc == 3)
   {
      if (strcmp(argv[1], "-v") == 0)
      {
         bVersion = true;
      }
      else if (strcmp(argv[1], "-l") == 0)
      {
         bListing = true;
      }
      else
      {
         cerr << "usage: asem7 [-v] [[-l] sourceFile]" << endl;
         return 2;
      }
      if (argv[2][0] == '-')
      {
         cerr << "usage: asem7 [-v] [[-l] sourceFile]" << endl;
         return 2;
      }
      else
      {
         if (strlen(argv[1]) > FILE_NAME_LENGTH - 3)
         {
            cerr << "Source file name too long" << endl;
            return 2;
         }
         else
         {
            strncpy (sourceFileName, argv[2], FILE_NAME_LENGTH);
         }
      }
   }
   else if (argc == 4)
   {
      if (strcmp(argv[1], "-v") == 0 && strcmp(argv[2], "-l") == 0 && argv[3][0] != '-')
      {
         bVersion = true;
         bListing = true;
         if (strlen(argv[1]) > FILE_NAME_LENGTH - 3)
         {
            cerr << "Source file name too long" << endl;
            return 2;
         }
         else
         {
            strncpy (sourceFileName, argv[3], FILE_NAME_LENGTH);
         }
      }
      else
      {
         cerr << "usage: asem7 [-v] [[-l] sourceFile]" << endl;
         return 2;
      }
   }
   else
   {
      cerr << "usage: asem7 [-v] [[-l] sourceFile]" << endl;
      return 2;
   }
   in_file.open(sourceFileName);
   if (in_file.fail())
   {
      cerr << "Could not open " << sourceFileName << "." << endl;
      return 3;
   }
   if (bVersion)
   {
      vVersionNumber();
   }
   vInitGlobalTables ();
   while (!(in_file.eof() || bTerminate)) //First pass of assembler
   {
      vGetLine();
      vProcessSourceLine (bTerminate);
      if (pACode[iCodeIndex]->bIsError())
      {
         iLineErrors[iErrorIndex++] = iCodeIndex;
      }
      iCodeIndex++;
   } 
   in_file.close();
   sUndeclaredsSymbolNode* q;
   i = 0;
   while (pUndeclaredSym != NULL) //Check for undeclared symbols and resolve addresses
   {
      if (!bLookUpSymbol(pUndeclaredSym->cSymID))
      {
         delete pACode[pUndeclaredSym->iLine];
         pACode[pUndeclaredSym->iLine] = new eSymNotDefined;
         int iTemp = 0;
         while ((i < iErrorIndex) && (iLineErrors[i] < pUndeclaredSym->iLine))
         {
            i++;
         }
         if (iLineErrors[i] != pUndeclaredSym->iLine)
         {
            iTemp = iLineErrors[i];
            iLineErrors[i] = pUndeclaredSym->iLine; //Insert new error line number
            for (j = iErrorIndex + 1; j > i + 1; j--)
            {
               iLineErrors[j] = iLineErrors[j - 1]; //Shift values to the right
            }
            iLineErrors[j] = iTemp;
         }
         iErrorIndex++;
      }
      q = pUndeclaredSym; //Deallocate pUndeclaredSym linked list
      pUndeclaredSym = pUndeclaredSym->pNext;
      delete q;
   }
   if ((iBurnCounter > 0) && (iErrorIndex == 0)) //Change addresses and symbol values if a .BURN was encountered
   {
      iBurnStart = iBurnStart - iCurrentAddress + 1;
      vChangeSymValBurn(iBurnStart);
      sEquateNode* p = pEquate;
      while (p != NULL)
      {
         vChangeSymValEquate(p->cSymID, p->cSymValue);
         p = p->pNext;
      }
      iBurnAddr = iBurnAddr + iBurnStart;
      for (iSecPassCodeIndex = 0; iSecPassCodeIndex < iCodeIndex; iSecPassCodeIndex++)
      {
         pValid = static_cast <Valid*> (pACode[iSecPassCodeIndex]);
         pValid->vBurnAddressChange();
      }
   }
   if ((iErrorIndex == 0) && (bTerminate) && (bListing)) //Create assembler listing
   {
      strncpy (listingFileName, sourceFileName, FILE_NAME_LENGTH);
      strcat(listingFileName, ".l");
      out_file.open(listingFileName);
      out_file << setiosflags(ios::fixed) << setiosflags(ios::showpoint)
               << setprecision(2);
      out_file << "-------------------------------------------------------------------------------" << endl;
      out_file << "      Object" << endl;
      if (pSymbol == NULL)
      {
         out_file << "Addr  code   Mnemon  Operand    Comment" << endl;
      }
      else
      {
         out_file << "Addr  code   Symbol   Mnemon  Operand    Comment" << endl;
      }
      out_file << "-------------------------------------------------------------------------------" << endl;
      for (iSecPassCodeIndex = 0; iSecPassCodeIndex < iCodeIndex; iSecPassCodeIndex++)
      {  //iSecPassCodeIndex is a global variable
         pACode[iSecPassCodeIndex]->vGenerateCode (); 
         //Comments already resolved in ACode objects containing .BLOCK or .ASCII pseudo-ops
         if ((pComment != NULL) && (pComment->iLine == iSecPassCodeIndex)) 
         {
            if (pComment->bNonemptyLine)
            {
               if (pSymbol == NULL)
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY_NO_SYMBOLS - 1] = '\0';
               }
               else
               {
                  pComment->cComment[COMMENT_LENGTH_NONEMPTY - 1] = '\0';
               }
            }
            out_file << ";" << pComment->cComment;
            sCommentNode* p = pComment; //Deallocate pComment linked list
            pComment = pComment->pNext;
            delete p;
         }
         out_file << endl;
      }
      out_file << "-------------------------------------------------------------------------------" << endl;
      if (pSymbol != NULL) //Output symbol table for assembler listing
      {
         out_file << endl << endl;
         out_file << "Symbol table" << endl;
         out_file << "--------------------------------------" << endl;
         out_file << "Symbol    Value        Symbol    Value" << endl;
         out_file << "--------------------------------------" << endl;
         sSymbolNode* p = pSymbol;
         bTemp = false;
         while (p != NULL)
         {
            out_file << p->cSymID;
            vSymbolListingBuffer(p->cSymID);
            out_file << " " << p->cSymValue;
            p = p->pNext;
            if (bTemp)
            {
               out_file << endl;
               bTemp = false;
            }
            else
            {
               vBlankSymbolColumn();
               bTemp = true;
            }
         }
         if (bTemp)
         {
            out_file << endl;
         }
         out_file << "--------------------------------------" << endl;
         out_file << "No errors.  Successful assembly." << endl;
      }
      out_file.close();
   }
   if ((iErrorIndex == 0) && (bTerminate)) //Generate object file
   {
      strncpy (objectFileName, sourceFileName, FILE_NAME_LENGTH);
      strcat(objectFileName, ".o");
      out_file.open(objectFileName);
      out_file << setiosflags(ios::fixed) << setiosflags(ios::showpoint)
               << setprecision(2);
      for (iSecPassCodeIndex = 0; iSecPassCodeIndex < iCodeIndex; iSecPassCodeIndex++)
      {
         pValid = static_cast <Valid*> (pACode[iSecPassCodeIndex]);
         pValid->vGenerateHexCode (false);
      }
      out_file << "zz" << endl;
      out_file.close();
   }
   else //Errors were detected
   {
      if (!bTerminate) //To account for absence of .END pseudo-op
      {
         delete pACode[iCodeIndex];
         pACode[iCodeIndex] = new eNoEnd;
         iLineErrors[iErrorIndex++] = iCodeIndex;
      }
      cerr << iErrorIndex;
      if (iErrorIndex == 1)
      {
         cerr << " error was detected. No object code generated." << endl;
      }
      else
      {
         cerr << " errors were detected. No object code generated." << endl;
      }
      for (i = 0; i < iErrorIndex; i++) //Generate error messages
      {
         cerr << "Error on line "<< iLineErrors[i] + 1 << ": ";
         pACode[iLineErrors[i]]->vGenerateCode ();
      }
   }
   sSymbolNode* p;
   while (pSymbol != NULL) //Deallocate pSymbol linked list
   {
      p = pSymbol;
      pSymbol = pSymbol->pNext;
      delete p;
   }
   sSymbolOutputNode* qTemp;
   while (pSymbolOutput != NULL) //Deallocate pSymbolOutput linked list
   {
      qTemp = pSymbolOutput;
      pSymbolOutput = pSymbolOutput->pNext;
      delete qTemp;
   }
   sEquateNode* pTemp;
   while (pEquate != NULL) //Deallocate pEquate linked list
   {
      pTemp = pEquate;
      pEquate = pEquate->pNext;
      delete pTemp;
   }
   for (i = 0; i <= iCodeIndex; i++) //Deallocate pACode array
   {
      delete pACode[i];
   }
   return 0;
}

