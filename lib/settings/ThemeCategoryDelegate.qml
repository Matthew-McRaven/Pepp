import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import edu.pepp

Rectangle {
    property var category: undefined
    property int activeCategory: 0
    property int buttonWidth: 50
    id: root
    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width
    PaletteFilterModel {
        id: paletteModel
        category: root.activeCategory
        sourceModel: PaletteModel {
            palette: ExtendedPalette {}
        }
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
                model: ["Default", "Dark"]
                textRole: "name"
                valueRole: "display"
            }
            Button {
                //  System themes can never have state change
                //  If non-system theme has changes, they must be saved before a copy can be made
                text: "Save"
                Layout.preferredWidth: root.buttonWidth
            }
            Button {
                id: del
                text: "Delete"
                Layout.preferredWidth: root.buttonWidth
                enabled: false //!Theme.systemTheme
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
                model: PaletteCategoryModel {}
                TabButton {
                    required property variant model
                    text: model.display
                    onClicked: {
                        root.activeCategory = Qt.binding(() => model.value)
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
            ListView {
                id: listView
                clip: true
                Layout.fillHeight: true
                Layout.minimumWidth: Math.max(
                                         100,
                                         contentItem.childrenRect.width + 5,
                                         childrenRect.width)
                focus: true
                focusPolicy: Qt.StrongFocus
                Keys.onUpPressed: listView.currentIndex = Math.max(
                                      0, listView.currentIndex - 1)
                Keys.onDownPressed: listView.currentIndex = Math.min(
                                        listView.count - 1,
                                        listView.currentIndex + 1)
                model: paletteModel
                delegate: BoxedText {
                    required property string display
                    required property var paletteItem
                    name: display
                }
            }
            PaletteDetails {
                id: modifyArea
                paletteItem: listView.currentItem?.paletteItem
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
}
