import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform //  Font picker

import "." as Ui

Item {
  width: 350
  height: 500

  ColumnLayout  {
    id: wrapper
    anchors.fill: parent
    anchors.leftMargin: 10
    anchors.topMargin: 10
    property int colWidth: 70

    //  Group box for parent properties
    GroupBox  {
      id: inheritGB
      Layout.topMargin: 20
      Layout.leftMargin: 10

      //  Groupbox label
      label: Ui.GroupBoxLabel {
        backgroundColor: "#ffffff"
        textColor: "#000000"
        text: "Parent Data"
      }
      ColumnLayout {
        anchors.fill: parent

        RowLayout {
          id: parent
          spacing: 5
          height: 20
          Layout.alignment: Qt.AlignLeft
          Layout.fillWidth: true
          Label {
            text: "Inherit From:"
            Layout.preferredWidth: wrapper.colWidth * 1.25
            Layout.preferredHeight: 20
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
          }
          ComboBox {
              id: parentId
              model: ListModel {
                ListElement { text: "None" }
                ListElement { text: "Text" }
                ListElement { text: "Background" }
                ListElement { text: "Comment" }
              }
            }
          }
          RowLayout {
            id: parentfg
            spacing: 5
            height: 20
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            visible: parentId.currentIndex !== 0
            Label {
              text: "Foreground:"
              Layout.preferredWidth: wrapper.colWidth * 1.25
              Layout.preferredHeight: 20
              horizontalAlignment: Text.AlignLeft
              verticalAlignment: Text.AlignVCenter
            }
            Button {
              id: pfgText
              Layout.preferredWidth: wrapper.colWidth
              Layout.preferredHeight: 20
              text: "#ff8c00"
              background: Rectangle {
                id: pfgColor
                color: "#ff8c00"
              }
            }
          }
        RowLayout {
          id: parentbg
          spacing: 5
          height: 20
          Layout.fillWidth: true
          Layout.alignment: Qt.AlignLeft
          visible: parentId.currentIndex !== 0
          Label {
            text: "Background:"
            Layout.preferredWidth: wrapper.colWidth * 1.25
            Layout.preferredHeight: 20
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
          }
          Button {
            id: pbgText
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            text: "#483d8b"
            background: Rectangle {
              id: pbgColor
              color: "#483d8b"
            }
          }
        }
        //  Font properties
        RowLayout {
          id: fontProperties
          visible: parentId.currentIndex !== 0
          Ui.FontProperties {
            id: ovdFont
            isEnabled: false
            Layout.fillWidth: true
            height: 40

            bold: true
            underline: true
          }
        } //  RowLayout
      } //  ColumnLayout
    } //  GroupBox
    GroupBox  {
      Layout.topMargin: 20
      Layout.leftMargin: 10
      Layout.rightMargin: 10

      //  Groupbox label
      label: Ui.GroupBoxLabel {
        backgroundColor: "#ffffff"
        textColor: "#000000"
        text: parentId.currentIndex === 0 ? "Set Data" : "Override Parent"
      }

      ColumnLayout {
        anchors.fill: parent

        RowLayout {
          id: foreground
          spacing: 5
          height: 20
          Layout.alignment: Qt.AlignLeft
          Layout.fillWidth: true
          Label {
            text: "Foreground:"
            Layout.preferredWidth: wrapper.colWidth * 1.25
            Layout.preferredHeight: 20
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
          }
          Button {
            id: fgText
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            text: "#ff8c00"
            background: Rectangle {
              id: fgColor
              color: "#ff8c00"
            }

            onClicked: {
              //  Set current control for callback on accepted()
              colorDialog.newColor = fgColor
              colorDialog.newText = fgText
              colorDialog.open()
            }
          }
          Button {
            text: "Clear"
            enabled: parentId.currentIndex === 0 || fgColor.color !== pfgColor.color
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            onClicked: {
              if(parentId.currentIndex === 0) {
                fgColor.color = "transparent"
                fgText.text = "transparent"
              }
              else {
                fgColor.color = pfgColor.color
                fgText.text = pfgText.text
              }
            }
          }
        }
        RowLayout {
          id: background
          spacing: 5
          height: 20
          Layout.fillWidth: true
          Layout.alignment: Qt.AlignLeft
          Label {
            text: "Background:"
            Layout.preferredWidth: wrapper.colWidth * 1.25
            Layout.preferredHeight: 20
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
          }
          Button {
            id: bgText
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            text: "#483d8b"
            background: Rectangle {
              id: bgColor
              color: "#483d8b"
            }

            onClicked: {
              //  Set current control for callback on accepted()
              colorDialog.newColor = bgColor
              colorDialog.newText = bgText
              colorDialog.open()
            }
          }
          Button {
            text: "Clear"
            enabled: parentId.currentIndex === 0 || bgColor.color !== pbgColor.color
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            onClicked: {
              if(parentId.currentIndex === 0) {
                bgColor.color = "transparent"
                bgText.text = "transparent"
              }
              else {
                bgColor.color = pbgColor.color
                bgText.text = pbgText.text
              }
            }
          }
        }

        RowLayout {
          spacing: 5
          height: 30
          Layout.fillWidth: true
          Layout.alignment: Qt.AlignLeft

          Label {
            text: "Font Properties"
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignBottom
          }
        }

        RowLayout {
          Ui.FontProperties {
            Layout.fillWidth: true
            height: 40
            Layout.alignment: Qt.AlignLeft

            italics: true
            strikethrough: true
          }
        }

        RowLayout { //  Empty space at bottom of pane
          Layout.fillWidth: true
          Layout.fillHeight: true
          Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
          }
        }
      }
    }
  }

  Platform.ColorDialog {
    id: colorDialog

    //  Save callback to controls that will be updated onAccepted
    property var newColor: undefined
    property var newText: undefined

    onAccepted: {
      if(newColor === undefined) return
      console.log("Color: "+colorDialog.color)

      //  Sets background color
      newColor.color = color

      //  Casts to hex representation
      newText.text = color

      newColor = undefined
      newText = undefined
    }
  }
}
