import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
  id: wrapper

  property bool isEnabled: true
  property int buttonWidth: 100
  property int buttonHeight: 20
  property color textColor: "#ffffff"

  property alias bold: boldCB.checked
  property alias italics: italicsCB.checked
  property alias underline: underlineCB.checked
  property alias strikeout: strikeoutCB.checked

  implicitWidth: 300

  //  Indicates user changed font properties
  signal updatedFont(fontProperty: int, value: bool)

  ColumnLayout {

    RowLayout {
      Layout.fillWidth: true
      Layout.alignment: Qt.AlignTop

      CheckBox {
        id: boldCB
        text: "Bold"
        enabled: isEnabled
        Layout.preferredWidth: buttonWidth
        Layout.preferredHeight: buttonHeight

        onClicked: {
          wrapper.updatedFont(3 /*PreferenceModel.PrefProperty.Bold*/, boldCB.checked)
        }
      }
      CheckBox {
        id: italicsCB
        Layout.preferredWidth: buttonWidth
        Layout.preferredHeight: buttonHeight
        enabled: isEnabled
        text: "Italics"

        onClicked: {
          wrapper.updatedFont(4/*PreferenceModel.PrefProperty.Italic*/, italicsCB.checked)
        }
      }
    }
    RowLayout {
      Layout.fillWidth: true
      Layout.alignment: Qt.AlignTop

      CheckBox {
        id: underlineCB
        text: "Underline"
        enabled: isEnabled
        Layout.preferredWidth: buttonWidth
        Layout.preferredHeight: buttonHeight

        onClicked: {
          wrapper.updatedFont(5 /*PreferenceModel.PrefProperty.Underline*/, underlineCB.checked)
        }
      }
      CheckBox {
        id: strikeoutCB
        text: "Strikeout"
        enabled: isEnabled
        Layout.preferredWidth: buttonWidth
        Layout.preferredHeight: buttonHeight

        onClicked: {
          wrapper.updatedFont(6 /*PreferenceModel.PrefProperty.Strikeout*/, strikeoutCB.checked)
        }      }
    } //  RowLayout
    RowLayout {
      Layout.fillHeight: true
      Layout.fillWidth: true
    }
  } //  ColumnLayout
}
