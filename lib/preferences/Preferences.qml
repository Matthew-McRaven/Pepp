import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "." as Ui
import edu.peppx 1.0

Flickable {
    id: root

    implicitHeight: layout.childrenRect.height
    implicitWidth: layout.childrenRect.width
    contentWidth: layout.childrenRect.width
    contentHeight: layout.childrenRect.height

    clip: true
    required property variant model
    RowLayout {
        id: layout
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
