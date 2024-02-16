pragma Singleton

import QtQuick

//  Default style is light
QtObject {
  id: root
  property string styleName: "Dark"

  //  General window presentation
  property color textColor: root.arrowColorOn
  property color backgroundColor: "0x31363b"

  //  Text editor colors
  property color breakpointColor: "red" // Color for break points

  property font font
  font.bold: false
  font.underline: false
  font.pixelSize: 11
  font.family: "Courier"
/*
  property color comment: Qt.lighter("green")         //Used for normal comments in micro & asm.
  property color rightOfExpression: "red"             //Used for right hand side of expressions in micro, and mnemonics and pseudo-ops in asm
  property color leftOfExpression: "lightsteelblue";  //Used for numbers in micro, and symbols in asm.
  property color symbolHighlight: "firebrick"         //Used for symbols in micro & unused in asm.
  property color conditionalHighlight: "orange"
  property color branchFunctionHighlight: "lightblue"
  property color warningHighlight: "lightsteelblue"   //Unused in micro, and used as warnings in asm.
  property color errorHighlight: "orangered"          //Used by errors in micro & asm, and strings in asm.
  property color altTextHighlight: "black"            //Text color opposite of the default text color (e.g. white when the default is black).

  property color seqCircuitColor: Qt.lighter("0x3B3630",200)
  property color combCircuitRed: "0xDF5A49"
  property color combCircuitBlue: "0x506082"
  property color combCircuitYellow: "0xE0B010"
  property color combCircuitGreen: "0x008A1C"

  property color muxCircuitRed: Qt.lighter("0xDF5A49",110)      // A sightly lighter shade of combCircuitRed that is a better background for text
  property color muxCircuitBlue: Qt.lighter("0x506082",110)     // A sightly lighter shade of combCircuitBlue that is a better background for text
  property color muxCircuitYellow: Qt.lighter("0xE0B010",110)   // A sightly lighter shade of combCircuitYellow that is a better background for text
  property color muxCircuitGreen: Qt.lighter("0x008A1C",110)    // A sightly lighter shade of combCircuitGreen that is a better background for text

  property color aluColor: Qt.darker(root.combCircuitBlue,140)
  property color aluOutline: root.combCircuitBlue

  property color arrowColorOn: "0xeeeeee"
  property color arrowColorOff: "gray"
  property color arrowImageOn;
  property color arrowImageOff;

  property color memoryHighlightPC: "magenta"
  property color memoryHighlightSP: "skyblue"
  property color memoryHighlightChanged: "orangered"
  property color lineAreaBackground: "0x404040"
  property color lineAreaText: "0xC0C0C0"
  property color lineAreaHighlight: "DodgerBlue"
  */
}
