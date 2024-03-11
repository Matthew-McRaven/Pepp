import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform //  Font picker

import "." as Ui

Item {
  id: root
  width: 350
  height: 500

  required property var preference

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
        textColor: "#000000"
        text: parentId.currentIndex === 0 ? ("Set Data for " + preference.name) : "Override Parent"
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

          Ui.ColorButton {
            id: fgText
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            color: preference.foreground
          }

          /*Button {
            id: fgText
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            text: preference.foreground //"#ff8c00"
            background: Rectangle {
              id: fgColor
              color: preference.foreground //"#ff8c00"
            }

            onClicked: {
              //  Set current control for callback on accepted()
              colorDialog.newColor = fgColor
              //colorDialog.newColor = preference.foreground
              colorDialog.newText = fgText
              colorDialog.type = 1
              colorDialog.open()
            }
          }*/
          Button {
            text: "Clear"
            enabled: parentId.currentIndex === 0 || fgColor.color !== pfgColor.color
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            onClicked: {
              if(parentId.currentIndex === 0) {
                fgColor.color = preference.foreground
                fgText.text = preference.foreground
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

          Ui.ColorButton {
            id: bgText
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            color: preference.background
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

            bold:      preference.bold
            italics:   preference.italics
            underline: preference.underline
            strikeout: preference.strikeout
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

  /*Platform.ColorDialog {
    id: colorDialog

    //  Save callback to controls that will be updated onAccepted
    property var newColor: undefined
    property var newText: undefined
    property int type: 0

    onAccepted: {
      if(newColor === undefined || type === 0) return
      //console.log("Color: "+colorDialog.color)

      //  Sets background color
      newColor.color = color

      //  Casts to hex representation
      newText.text = color

      //  Clear state
      newColor = undefined
      newText = undefined

      if( type === 1) {
        //  Foreground font
        preference.foreground = color
      }
      else if( type === 2) {
        //  Foreground font
        preference.background = color
      }
    }
  }*/
}
