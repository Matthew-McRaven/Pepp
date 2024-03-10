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
      }
      CheckBox {
        id: italicsCB
        Layout.preferredWidth: buttonWidth
        Layout.preferredHeight: buttonHeight
        enabled: isEnabled
        text: "Italics"
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
      }
      CheckBox {
        id: strikeoutCB
        text: "Strikeout"
        enabled: isEnabled
        Layout.preferredWidth: buttonWidth
        Layout.preferredHeight: buttonHeight
      }
    } //  RowLayout
    RowLayout {
      Layout.fillHeight: true
      Layout.fillWidth: true
    }
  } //  ColumnLayout
}
