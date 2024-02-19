import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: project
    required property string mode
    property color color: "red"
    Rectangle {
      anchors.top: parent.top; anchors.bottom: parent.bottom
      anchors.left: parent.left; anchors.right: debugArea.visible ? debugArea.left : parent.right
      ComboBox {
          id: activeFile
          anchors.right: parent.right; anchors.left: parent.left
          anchors.top: parent.top; height: 40
          model: ["User", "OS"]
      }
      StackLayout {
          currentIndex: activeFile.currentIndex
          anchors.right: parent.right; anchors.left: parent.left
          anchors.top: activeFile.bottom; anchors.bottom: parent.bottom
          TextArea {
              text: "This is an user program"
              color: project.color
          }
          TextArea {
              text: "This is an operating system"
              color: project.color
          }
      }
    }
    Rectangle {
        id: debugArea
        visible: project.mode != "EDIT"
        anchors.right: parent.right; width: 300
        anchors.top: parent.top; anchors.bottom: parent.bottom
        color: "lightgray"
    }

}
