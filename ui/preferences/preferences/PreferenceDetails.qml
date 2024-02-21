import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform //  Font picker

import "." as Ui
import edu.pepperdine 1.0

Item {
  width: 600
  property int buttonWidth: 50

  property font asciiFont:
      Qt.font({family: 'Courier New',
                weight: Font.Normal,
                italic: false,
                bold: false,
                pointSize: 10})

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
      GroupBox {
        id: fontGB
        Layout.topMargin: 20
        Layout.leftMargin: 10

        //  Groupbox label
        label: Ui.GroupBoxLabel {
          backgroundColor: "#f0f0f0"
          textColor: "#000000"
          text: "Font"
        }

        RowLayout {
          Label { text: "Current Font Family: " }
          Text { text: asciiFont.family         }
          Button {
            text: "Change";
            Layout.preferredWidth: buttonWidth

            //Component.onCompleted:
            onClicked: {
              //  Open dialog and set properties.
              //  Control will trigger visible in onCompleted.
              fontDialog.open()
              fontDialog.currentFont = asciiFont
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
          backgroundColor: "#f0f0f0"
          textColor: "#000000"
          text: "Color Scheme for Theme: " + themeId.currentText
        }
        ListView {
          id: propertyListView
          //model: PreferenceModel
          anchors.fill: parent
          //anchors.margins: 2.5
          clip: true

          model: 10

          delegate: Text {
            text: "Line "+ index
            padding: 2
          }

          //  Trigger change in right pane
          //onCurrentItemChanged:
        }
      }
    }
  }

  //  Does not work on windows platform
  Platform.FontDialog {
    id: fontDialog

    //Component.onCompleted:  visible = true
    onAccepted: {
      asciiFont = font
    }
  }
}
