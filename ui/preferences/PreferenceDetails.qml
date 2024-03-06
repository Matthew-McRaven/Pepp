import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform //  Font picker

import "." as Ui
import edu.pepperdine 1.0

Item {
  id: root
  width: 600
  property int buttonWidth: 50
  required property var model

  ColumnLayout {
    anchors.fill: parent
    Layout.margins: 10

    //  Theme selection
    RowLayout {
      Text { id: text; text: "Current Theme" }
      ComboBox {
      id: themeId
        model: ListModel {  //  Temporary-to be part of model
          ListElement { text: "Default" }
          ListElement { text: "Dark" }
        }
      }
      Button { text: "Copy";   Layout.preferredWidth: buttonWidth}
      Button { text: "Delete"; Layout.preferredWidth: buttonWidth}
      Button { text: "Import"; Layout.preferredWidth: buttonWidth}
      Button { text: "Export"; Layout.preferredWidth: buttonWidth}

    } //  RowLayout - Theme
    //  Font selection
    RowLayout {
      id: layout
      GroupBox {
        id: fontGB
        Layout.topMargin: 20
        Layout.leftMargin: 10

        //  Groupbox label
        label: Ui.GroupBoxLabel {
          textColor: model.normalText.foreground
          text: "Font"
        }

        RowLayout {
          Label { text: "Current Font Family: " }
          Text { text: model.font.family }
          Label { text: "Size: " }
          Text { text: model.font.pointSize     }
          Button {
            text: "Change";
            Layout.preferredWidth: buttonWidth

            //Component.onCompleted:
            onClicked: {
              //  Open dialog and set properties.
              //  Control will trigger visible in onCompleted.
              fontDialog.open()
              console.log("Font family=" + model.font.family)
              fontDialog.currentFont = model.font
              fontDialog.visible = true
            }
          }
        }
      }
    } //  RowLayout - Font
    RowLayout {
      Layout.topMargin: 10
      GroupBox {
        id: propertiesGB
        Layout.fillHeight: true
        Layout.fillWidth: true
        z: -1

        //  Groupbox label
        label: Ui.GroupBoxLabel {
          textColor: model.normalText.foreground //"#000000"
          text: "Color Scheme for Theme: " + themeId.currentText
        }
        ListView {
          id: propertyListView
          anchors.fill: parent
          clip: true

          model: root.model

          delegate: Label {
            text: model.currentCategory
            color: model.currentList.foreground
            font: model.currentList.font
            background: Rectangle {
              color: model.currentList.background
            }

            padding: 2
          }
        }
      }
    }
  }

  //  Does not work on windows platform
  Platform.FontDialog {
    id: fontDialog
    //options: MonospacedFonts

    onAccepted: {
      console.log("Font Dialog family="+fontDialog.font.family)
      model.font = fontDialog.font //  Works
    }
  }
}
