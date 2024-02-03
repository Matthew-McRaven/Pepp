import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material  //  For colors
import Qt.labs.qmlmodels          //  For DelegateChooser

import "./Components" as Ui
//import edu.pepperdine 1.0 //  Only for Qml instantiation

Rectangle {
  id: root
  anchors.margins: 5
  color: Material.background
  //  For testing only.
  //border.color: "red"
  //border.width: 1

  //  Properties set by parent
  property int controlWidth: 250
  property int colWidth: 40
  property int rowHeight: 20

  Material.theme: Material.Light
  Material.accent: "#888888" //Material.Indigo  //  Cursor
  Material.foreground: Material.theme === Material.Light ?
                         "#383838" : "#f8f8f8"//  Text color
  Material.background: Material.theme === Material.Light ?
                         "#f8f8f8" : "#383838"//  Background
  //  Status bits
  ListView {
    id: statusBitsView
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.leftMargin: (controlWidth - childrenRect.width)/2
    anchors.topMargin: 5
    anchors.bottomMargin: 5
    width: controlWidth
    height: rowHeight
    orientation: ListView.Horizontal
    //model: StatusBitModel{} //  Qml instantiation
    model: StatusBitModel     //  C++ instantiation

    delegate: statusBitDelegate

    Component {
      id: statusBitDelegate

      //  Hidden elements of delegate
      //required property int index
      //required property var modelData
      Ui.NameValueEdit {
        width: colWidth
        height: rowHeight

        //  Uses c++ roleNames_ array to map to data
        label: model.statusBitRole
        value: model.flagRole ? "1" : "0"
      }
    }
  }

  //  List of registers
  ListView {
    id: registers
    anchors.top: statusBitsView.bottom
    anchors.left: parent.left
    anchors.topMargin: 5
    width: 100 //childrenRect.width
    height: childrenRect.height * 4
    orientation: ListView.Vertical
    spacing: 5

    //model: RegisterModel{}  //  Qml instantiation
    model: RegisterModel      //  C++ instantiation

    // New Start
    delegate: registerDelegateChooser

    DelegateChooser {
      id: registerDelegateChooser
      role: "nameRole"

      //  List exceptions first
      DelegateChoice {
        roleValue: "Instruction Specifier"
        Ui.RegisterBinEdit {
          width: controlWidth

          //  Uses c++ roleNames_ array to map to data
          register: model.nameRole
          address: model.addressRole
          value: model.dataRole
          hex16: false  //  Display as quint8
          showPrefix: false //  do not show leading 0b
        }
      }

      DelegateChoice {
        roleValue: "Operand"
        Ui.RegisterHexEdit {
          width: controlWidth

          //  Uses c++ roleNames_ array to map to data
          register: model.nameRole
          address: model.addressRole
          value: model.dataRole
          hex16: false  //  Display as quint8
        }
      }

      //  Default control - always last
      DelegateChoice {
        //roleValue: model.nameRole // This is always equal
        Ui.RegisterHexEdit {
          width: controlWidth

          //  Uses c++ roleNames_ array to map to data
          register: model.nameRole
          address: model.addressRole
          value: model.dataRole
          hex16: true  //  Display as quint16
        }
      }
    }
  }
}
