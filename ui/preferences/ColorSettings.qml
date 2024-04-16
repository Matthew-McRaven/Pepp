import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform //  Font picker

import "." as Ui

Item {
  id: root
  width: 350
  height: 500

  //  Used to paint screen
  required property var preference

  //  Used for updates
  required property var model

  ColumnLayout  {
    id: wrapper
    Layout.fillHeight: true
    Layout.fillWidth: true
    Layout.margins: 5
    property int colWidth: 70

    //  Group box for parent properties
    GroupBox  {
      id: inheritGB
      Layout.topMargin: 20
      Layout.leftMargin: 10
      background: Rectangle {
        color: Theme.container.background
        border.color: Theme.container.foreground
        border.width: 1
      }

      //  Groupbox label
      label: Ui.GroupBoxLabel {
        textColor: Theme.primary.foreground
        text: "Parent Data"
      }
      ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true

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

      background: Rectangle {
        color: Theme.container.background
        border.color: Theme.container.foreground
        border.width: 1
      }

      //  Groupbox label
      label: Ui.GroupBoxLabel {
        textColor: Theme.primary.foreground
        text: parentId.currentIndex === 0 ? ("Set Data for " + preference.name) : "Override Parent"
      }

      ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true

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

            onUpdatedColor: (newColor) => {
              if(newColor !== preference.foreground) {

                //  Update model. Model will trigger screen repaint
                model.updatePreference(preference.id,1,newColor)
                //console.log("After pref.foreground: " + model.currentPref.foreground)
              }
            }
          }
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

            onUpdatedColor: (newColor) => {
              if(newColor !== preference.background) {

                //  Update model. Model will trigger screen repaint
                model.updatePreference(preference.id,2,newColor)
                //console.log("After pref.foreground: " + model.currentPref.background)
              }
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

            bold:      preference.bold
            italics:   preference.italics
            underline: preference.underline
            strikeout: preference.strikeout

            onUpdatedFont: (prop,flag) => {

              //  Update model. Model will trigger screen repaint
              model.updatePreference(preference.id,prop,flag)
              //console.log("After pref.foreground: " + prop +"="+flag)
            }
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
}
