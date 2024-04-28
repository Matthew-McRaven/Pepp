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
    spacing: 20

    ThemeManagement {
      id: themeId
      buttonWidth: root.buttonWidth
      model: root.model
    }

    //  Font selection
    RowLayout {
      id: layout
      GroupBox {
        id: fontGB

        //  Groupbox label
        label: Ui.GroupBoxLabel {
          textColor: Theme.primary.foreground
          text: "Font"
        }

        background: Rectangle {
          color: Theme.container.background
          border.color: Theme.container.foreground
          border.width: 1
        }

        RowLayout {
          Label { text: "Current Font Family: "; color: Theme.container.foreground}
          Text { id: family; text: root.model.font.family; color: Theme.container.foreground}
          Label { text: "Size: "; color: Theme.container.foreground}
          Text { id: fontsize; text: root.model.font.pointSize; color: Theme.container.foreground}
          Button {
            text: "Change";
            Layout.preferredWidth: buttonWidth
            palette {
              button: Theme.container.background
              buttonText: Theme.surface.foreground
            }

            onClicked: {
              //  Open dialog and set properties.
              //  Control will trigger visible in onCompleted.
              fontDialog.open()
              //console.log("Font family=" + family)//model.font.family)
              fontDialog.currentFont = root.model.font
              fontDialog.visible = true
            }
          }
        }
      }
    } //  RowLayout - Font
    RowLayout {
      //Layout.topMargin: 10
      GroupBox {
        id: propertiesGB
        Layout.fillHeight: true
        Layout.fillWidth: true
        z: -1

        background: Rectangle {
          color: Theme.container.background
          border.color: Theme.container.foreground
          border.width: 1
        }

        //  Groupbox label
        label: Ui.GroupBoxLabel {
          textColor: Theme.primary.foreground
          text: "Color Scheme for Theme: " + Theme.name
        }

        Ui.PreferenceList {
          id: listView
          model: root.model
          anchors.fill: parent
        }
      }
    }
  }

  //  Does not work on windows platform
  Platform.FontDialog {
    id: fontDialog
    //options: MonospacedFonts

    onAccepted: {
      //console.log("Font Dialog family="+fontDialog.font.family)

      //  Save new font to model. Triggers refresh
      model.font = fontDialog.font

      fontsize.text = fontDialog.font.pointSize
      family.text = fontDialog.font.family
    }
  }
}
