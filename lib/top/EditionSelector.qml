pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//  Edition selection in ComboBox
Flow {
    id: root
    property list<int> filterEdition: []
    required property var font
    property font noItalic: FontUtils.fromFont(root.font).noitalicize().font()
    Label {
        id: label
        text: `Computer Systems, `
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
        model: ListModel {
            id: model
            ListElement {
                text: "Sixth"
                edition: 6
            }
            ListElement {
                text: "Fifth"
                edition: 5
            }
            ListElement {
                text: "Fourth"
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
    }   //
    Label {
        text: " Edition"
        font: root.noItalic
    }
    Item {
        Layout.fillWidth: true
    }
}   //  Row Layout
