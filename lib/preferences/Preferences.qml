import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "." as Ui
import edu.pepp 1.0

Rectangle {
    id: root

    color: palette.base

    required property variant model

    RowLayout {
        anchors.fill: parent

        //  Category list
        Ui.CategoryList {
            id: categoryList
            Layout.fillHeight: true
            Layout.margins: 3

            implicitWidth: 100
            model: root.model
        }

        //  Preferences for chosen cateogory
        Ui.PreferenceDetails {
            id: prefenceDetails
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 3

            color: palette.base
            model: root.model
        }
    }
}
