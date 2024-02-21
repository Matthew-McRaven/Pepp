import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
//import Qt.labs.qmlmodels          //  For DelegateChooser

import "." as Ui
//import edu.pepperdine 1.0

Rectangle {
  id: root

  anchors.fill: parent
  color: "white"

  RowLayout {
    spacing: 2
    anchors.fill: parent

    //  Category list
    Ui.CategoryList {
      Layout.fillHeight: true
      Layout.margins: 3
      implicitWidth: 100
      model: PreferenceModel
    }

    //  Preferences for chosen cateogory
    Ui.PreferenceDetails {
      Layout.fillHeight: true
      Layout.fillWidth: true
      Layout.margins: 3
    }

    //  Overrides
    Ui.ColorSettings {
      visible: true
      Layout.fillHeight: true
      implicitWidth: 300
      Layout.margins: 3
    }
  }
}

