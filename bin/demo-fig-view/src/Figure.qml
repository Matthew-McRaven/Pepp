import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
Item {


  property alias text_area:  figContent


  //required property var model

  ColumnLayout {
    id: figCol

    Highlighter {
        id: highlighter
    }
    StyleMap {
        id: styles
    }
    property string copyTitle: "5.7"
    property string copyContent: "This is some very long text used to test wrapping inside text control."
    property var payload
    property string listing: "Pep/10 is a virtual machine for writing machine language and assemply language programs"
    property bool copyToSource: true
    signal typeChange(string type)
    //  Set page contents based on parent selected values
    Component.onCompleted: {
      DefaultStyles.pep10_asm(styles)
      highlighter.set_styles(styles)
      highlighter.set_document(figContent.textDocument)

      copyTitle = drawer.selected.display;
      let payload = drawer.selected.payload;
      let edition = drawer.selected.edition;

      copyToSource = ( payload.chapterName !== "04");

      Object.keys(payload.elements).map((lang)=>{
        languageModel.append({
          key: lang,
          value: payload.elements[lang].content
        })
      })

      //  Show first item in list box.
      let lang = Object.keys(payload.elements)[0]
      figCol.listing = payload.elements[lang].content
      figType.currentIndex = 0;
      highlighter.set_highlighter(edition, lang)
    }

    spacing: 10
    anchors {
      topMargin: 0
      leftMargin: 20
      rightMargin: 20
      bottomMargin: 20
      fill: parent
    }

    ComboBox {
      id: figType
      //  Control layout
      Layout.alignment: Qt.AlignTop
      Layout.preferredHeight: 30
      Layout.preferredWidth: parent.width
      Layout.fillWidth: true

      //  Mode setup
      textRole: "key"
      valueRole: "value"
      model: ListModel {
        id: languageModel
      }

      //  Update code listing
      onActivated: figCol.listing = currentValue

      //  Use similar size as topic title
      font.pointSize: 22

      delegate: ItemDelegate {
        width: figType.width
        contentItem: Text {
            text: figType.textRole
                  ? (Array.isArray(figType.model) ? modelData[figType.textRole] : model[figType.textRole])
                  : modelData
            //  Text color in dropdown
            color: figType.highlightedIndex === index ? "#ffffff" : "#333333"// "#ff7d33"
            font.pointSize: 12
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: figType.highlightedIndex === index
      }

      indicator: Canvas {
        id: canvas
        x: figType.width - width - figType.rightPadding
        y: figType.topPadding + (figType.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: figType
            function onPressedChanged() { canvas.requestPaint(); }
        }

        onPaint: {
          context.reset();
          context.moveTo(0, 0);
          context.lineTo(width, 0);
          context.lineTo(width / 2, height);
          context.closePath();
          context.fillStyle = figType.pressed ? Qt.black : "#ff7d33";
          context.fill();
        }
     }
      contentItem: Text {
        leftPadding: 0
        rightPadding: figType.indicator.width + figType.spacing

        text: figType.displayText
        font: figType.font
        color: "#ffffff"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
      }

      //  Background in control (not dropped down)
      background: Rectangle {
        color: "#ff7d33";
        radius: 5
      }
    }

    //  Figure contents
    ScrollView {
      Layout.alignment: Qt.AlignCenter
      Layout.fillHeight: true;
      Layout.fillWidth: true
      Component.onCompleted: {
          console.log(figContent)
          finder.set_document(figContent.textDocument)
          lineNumbers.update()
      }

      BlockFinder {
          id: finder
      }

      LineNumbers {
          id: lineNumbers
          height: parent.height // Ensure that line numbering area spans entire text area.
          width: 40
      }
      TextArea {
        id: figContent
        function update() {
            lineNumbers.lineCount = lineCount
            // font metrics lies about height, because it does not include intra-line padding.
            // instead, math out from content height the line height.
            // lineNumbers.lineHeight = metrics.height
            lineNumbers.lineHeight = contentHeight / lineCount
            // Use my C++ helper code to determine line number from cursors integer.
            lineNumbers.cursorPosition = finder.find_pos(cursorPosition)
            lineNumbers.selectionStart = finder.find_pos(selectionStart)
            lineNumbers.selectionEnd = finder.find_pos(selectionEnd)
            lineNumbers.update()  // Graphics area will never update without requesting it.
        }

        // Anchor otherwise line numbers overlap text edit.
        anchors.left: lineNumbers.right

        onLineCountChanged: update()
        onHeightChanged: update()
        onCursorPositionChanged: update()

        onSelectedTextChanged: update()

        font.family: "Courier New"

        //textFormat: TextEdit.RichText
        wrapMode: TextEdit.NoWordWrap
        readOnly: true;

        text: figCol.listing
      }
    }

    Row {
      id: copyRow

      Layout.alignment: Qt.AlignBottom
      Layout.preferredHeight: 20
      Layout.maximumHeight: 30
      Layout.fillWidth: true

      spacing: 10

      //  Copy button logic
      Button {
        id: button
        text: figCol.copyToSource ? "Copy to Source" : "Copy to Object"
        anchors {
          horizontalCenter: copyRow.center
        }
      }

      //  Figure title
      Text { // figCol.copyTitle & figCol.copyContent
        width: copyRow.width - button.width - copyRow.spacing
        textFormat: Text.RichText
        text: "<div><b>Figure " + figCol.copyTitle + ":</b> "+ figCol.copyContent +"</div>"
        wrapMode: Text.WordWrap
      }
    } //  Row
  } //  Column
} //  Item
