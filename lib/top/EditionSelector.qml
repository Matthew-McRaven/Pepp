pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//  Edition selection in ComboBox
Flow {
    id: root
    property list<int> filterEdition: []
    required property var font
    required property font noItalic
    Label {
        id: label
        text: `Computer Systems   `
        font: root.font
    }
    ComboBox {
        id: comboBox
        height: label.height
        editable: false
        currentIndex: 0 //  Always default to latest version
        textRole: "text"
        valueRole: "edition"
        font: root.noItalic
        implicitContentWidthPolicy: ComboBox.WidestText
        model: ListModel {
            id: model
            ListElement {
                text: "Sixth Edition"
                edition: 6
            }
            ListElement {
                text: "Fifth Edition"
                edition: 5
            }
            ListElement {
                text: "Fourth Edition"
                edition: 4
            }
        }
        // font: root.font
        onCurrentIndexChanged: {
            //  Cannot user currentValue since this is value before change.
            //  Must lookup value based on index, which is current.
            settings.general.defaultEdition = valueAt(currentIndex);
        }
        Component.onCompleted: {
            //  Set initial edition (latest)
            settings.general.defaultEdition = valueAt(currentIndex);

            //console.log("comboBox height", comboBox.height,"label.height", label.height);
        }
    }
    Item {
        Layout.fillWidth: true
    }
}   //  Row Layout
