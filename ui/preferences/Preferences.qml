import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "." as Ui
import edu.pepperdine 1.0

Rectangle {
  id: root

  color: model.surface.background

  property variant model: PreferenceModel

  RowLayout {
    spacing: 2
    anchors.fill: parent

    //  Category list
    Ui.CategoryList {
      Layout.fillHeight: true
      Layout.margins: 3
      implicitWidth: 100
      model: root.model
    }

    //  Preferences for chosen cateogory
    Ui.PreferenceDetails {
      id: details
      Layout.fillHeight: true
      Layout.fillWidth: true
      Layout.margins: 3
      model: root.model
    }

    //  Overrides
    Ui.ColorSettings {
      visible: true
      Layout.fillHeight: true
      implicitWidth: 300
      Layout.margins: 3
      Layout.topMargin: 100

      //  Currently selected preference
      preference: root.model.currentPref
      model: root.model

    }
  }
}

