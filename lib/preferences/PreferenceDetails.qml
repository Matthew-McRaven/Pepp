import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

//  Font picker
import "." as Ui
import edu.peppx 1.0

Rectangle {
    id: root
    //width: 600
    property int buttonWidth: 50
    implicitWidth: columnLayout.childrenRect.width
    implicitHeight: columnLayout.childrenRect.height
    required property var model

    ColumnLayout {
        id: columnLayout
        anchors.fill: parent
        Layout.margins: 10
        spacing: 20
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height

        ThemeManagement {
            id: themeId
            buttonWidth: root.buttonWidth
            model: root.model
        }

        GroupBox {
            id: fontGB

            //  Groupbox label
            label: Ui.GroupBoxLabel {
                textColor: palette.windowText
                backgroundColor: palette.window
                text: "Font"
            }

            background: Rectangle {
                color: palette.window
                border.color: palette.windowText
                border.width: 1
            }

            RowLayout {
                Label {
                    text: "Current Font Family: "
                }
                Text {
                    id: family
                    text: root.model.font.family
                    color: palette.windowText
                }
                Label {
                    text: "Size: "
                }
                Text {
                    id: fontsize
                    text: root.model.font.pointSize
                    color: palette.windowText
                }
                Button {
                    text: "Change"
                    Layout.preferredWidth: buttonWidth
                    palette {}

                    onClicked: {
                        //  Open dialog and set properties.
                        //  Control will trigger visible in onCompleted.
                        fontDialog.open()
                        fontDialog.currentFont = root.model.font
                        fontDialog.visible = true
                    }
                }
            }
        }

        RowLayout {
            //  Listbox with current color scheme
            GroupBox {
                id: propertiesGB
                Layout.fillWidth: true
                label: Ui.GroupBoxLabel {
                    textColor: palette.windowText
                    backgroundColor: palette.window
                    text: "Color Scheme for Theme: " + Theme.name
                }

                background: Rectangle {
                    color: palette.window
                    border.color: palette.windowText
                    border.width: 1
                }
                Ui.PreferenceList {
                    id: listView
                    model: root.model
                    anchors.fill: parent
                }
            }

            ColumnLayout {
                Ui.ColorSettings {
                    id: settings
                    Layout.minimumWidth: 225
                    enabled: !Theme.systemTheme
                    color: palette.window
                    model: root.model
                }

                // Must wrap text in item to overwrite implicitWidth
                Item {
                    width: settings.implicitWidth
                    implicitWidth: settings.implicitWidth
                    implicitHeight: childrenRect.height
                    Layout.minimumHeight: implicitHeight

                    Text {
                        anchors.fill: parent
                        wrapMode: Text.WordWrap
                        padding: 15
                        color: Theme.systemTheme ? palette.windowText : "transparent"

                        text: "<b>Builtin color schemes must be copied before they can be changed.<b>"
                    }
                }
            }
        }
    }

    //  Does not work on windows platform
    Platform.FontDialog {
        id: fontDialog

        //options: MonospacedFonts
        onAccepted: {
            //console.log("Font Dialog family="+fontDialog.font.family)

            //  Save new font to model. Triggers refresh
            model.font = fontDialog.font

            fontsize.text = fontDialog.font.pointSize
            family.text = fontDialog.font.family
        }
    }
}
