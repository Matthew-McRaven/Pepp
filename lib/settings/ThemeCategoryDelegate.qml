import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import edu.pepp

Rectangle {
    property var category: undefined
    property int activeCategory: 0
    property int buttonWidth: 50
    id: root
    color: "red"
    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width
    ThemeModel {
        id: themeModel
    }
    ColumnLayout {
        id: layout
        anchors.fill: parent
        RowLayout {
            Label {
                text: "Current Theme: "
                Layout.alignment: Qt.AlignHCenter
            }
            ComboBox {
                id: comboBox
                model: themeModel
                textRole: "name"
                valueRole: "display"
            }
            Button {
                //  System themes can never have state change
                //  If non-system theme has changes, they must be saved before a copy can be made
                text: Theme.isDirty ? "Save" : "Copy"
                Layout.preferredWidth: root.buttonWidth
            }
            Button {
                id: del
                text: "Delete"
                Layout.preferredWidth: root.buttonWidth
                enabled: !Theme.systemTheme
            }
            Button {
                text: "Import"
                Layout.preferredWidth: root.buttonWidth
            }
            Button {
                text: "Export"
                Layout.preferredWidth: root.buttonWidth
            }
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Repeater {
                model: CategoryModel {}
                TabButton {
                    required property variant model
                    text: model.display
                    onClicked: {
                        root.activeCategory = Qt.binding(() => model.value)
                    }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "purple"
            Layout.margins: 10
        }
    }
}
