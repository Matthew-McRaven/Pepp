import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import "./Components" as Ui

Rectangle {
  id: root
  width: 1000
  height: 800

  Material.theme: Material.Light
  Material.accent: "#888888" //Material.Indigo  //  Cursor
  /*Material.foreground: Material.theme === Material.Light ?
                         "#383838" : "#f8f8f8"//  Text color
  Material.background: Material.theme === Material.Light ?
                         "#f8f8f8" : "#383838"//  Background*/
  Material.primary: Material.Red  //  Breakpoint
  property color highlight: Material.theme === Material.Light ?
                              Material.background.darker(1.5) :
                              Material.background.lighter(1.5) //"#888888"

  property int colWidth: 30
  property int rowHeight: editor.contentHeight / editor.lineCount

  color: Material.background

  //  Link row position in editor to column controls
  function setCurrentRow() {
    //  Convert pixel location to row count
    var row = (editor.cursorRectangle.y - editor.topPadding)/ rowHeight;
    rows.currentRow = row;
    breakpoint.currentRow = row;
    //console.info(editor.cursorRectangle,rowHeight,row);
  }

  ScrollView {
    anchors.fill: parent

    RowLayout {
      spacing: 0
      anchors.fill: parent
      Layout.fillHeight: true
      Layout.fillWidth: true

      //  Line numbers
      Ui.RowNumbers {
        id: rows
        Layout.topMargin: editor.topPadding
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignLeft
        Layout.maximumWidth: rows.width
        Layout.minimumWidth: rows.width

        width: colWidth

        colWidth: root.colWidth
        rowHeight: root.rowHeight

        rows: editor.lineCount
        rowfont: editor.font
        backgroundColor: Material.background
        textColor: Material.foreground
        highlightColor: highlight
      }

      //  Break point indicator
      Ui.BreakPoint {
        id: breakpoint

        Layout.topMargin: editor.topPadding
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignLeft
        Layout.maximumWidth: breakpoint.width
        Layout.minimumWidth: breakpoint.width

        width: root.rowHeight * 1.5

        colWidth: root.rowHeight * 1.5
        rowHeight: root.rowHeight
        rows: editor.lineCount
        backgroundColor: Material.background
        breakpointColor: Material.primary
        highlightColor: root.highlight
      }

      //  Editor for coding
      Rectangle {
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignLeft
        Layout.minimumWidth: root.colWidth * 10

        TextArea {
          id: editor

          focus: true
          anchors.fill: parent
          topPadding: 5
          color: Material.foreground
          selectedTextColor: root.highlight

          text: "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20"

          background: Rectangle {
            border.color: "transparent"
            color: Material.background
          }

          onCursorPositionChanged: {
            root.setCurrentRow(TextEdit.cursorRectangle)
          }
        }
      }
    }
  }
}
