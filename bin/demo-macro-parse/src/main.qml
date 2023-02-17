import QtQuick
import QtQuick.Controls
import edu.pepperdine.cslab.macroparse
Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    Item {
      id: logic
      MacroParser {
        id:parser
      }
      function onHandleText() {
        let text = macro.text;
        if(!text) emptyInput()
        const parseResult = parser.parse(text)
        if(parseResult.valid) validInput(parseResult.name, parseResult.argc)
        else invalidInput()
      }
      signal emptyInput()
      signal invalidInput()
      signal validInput(name: string, argc:int)
      Component.onCompleted: {
        parseButton.clicked.connect(logic.onHandleText)
        emptyInput.connect(parseResult.onEmptyInput)
        invalidInput.connect(parseResult.onInvalidInput)
        validInput.connect(parseResult.onValidInput)
      }
    }

    TextArea {
      id: macro
      anchors.top: parent.top
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: actionBar.top
      placeholderText: "Add macro text"
    }
    Rectangle {
      id: actionBar
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      height: 20
      Label {
        id: parseResult
        anchors.left: parent.left
        anchors.right: parseButton.left
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        text: "Please parse a macro"
        function onEmptyInput() {
          parseResult.text = "Text area is empty, please add text"
        }
        function onInvalidInput() {
          parseResult.text = "Not a valid macro definition"
        }
        function onValidInput(name, argc) {
          parseResult.text = `Macro is named "${name}" and has ${argc} arguments`
        }
      }
      Button {
        id: parseButton
        text: "parse"
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: parent.width * .1
      }
    }
}
