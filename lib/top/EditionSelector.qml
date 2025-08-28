pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property list<int> filterEdition: []
    required property var font

    //  Required for accurate spacing in parent layout
    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width

    //  Edition selection in ComboBox
    RowLayout {
        id: header
        spacing: 0
        implicitHeight: childrenRect.height
        implicitWidth: childrenRect.width

        anchors {
            //  Do not set bottom equal to root, or layout will break
            left: root.left
            right: root.right
            top: root.top
        }

        Label {
            id: label
            text: `Computer Systems Edition: `
            font: root.font
        }
        ComboBox {
            id: comboBox

            editable: false
            currentIndex: 0 //  Always default to latest version
            textRole: "text"
            valueRole: "edition"
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

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }   //  Flow
}
