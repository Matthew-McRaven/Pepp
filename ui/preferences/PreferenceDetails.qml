import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

//  Font picker
import "." as Ui
import edu.pepperdine 1.0

Rectangle {
    id: root
    //width: 600
    property int buttonWidth: 50
    required property var model

    ColumnLayout {
        anchors.fill: parent
        Layout.margins: 10
        spacing: 20

        ThemeManagement {
            id: themeId
            buttonWidth: root.buttonWidth
            model: root.model
        }

        //  Font selection
        RowLayout {
            id: layout
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
                        palette {//button: Theme.container.background
                            //buttonText: Theme.surface.foreground
                        }

                        onClicked: {
                            //  Open dialog and set properties.
                            //  Control will trigger visible in onCompleted.
                            fontDialog.open()
                            //console.log("Font family=" + family)//model.font.family)
                            fontDialog.currentFont = root.model.font
                            fontDialog.visible = true
                        }
                    }
                }
            }
        } //  RowLayout - Font

        RowLayout {
            //  Listbox with current color scheme
            Layout.fillWidth: true
            spacing: 5
            RowLayout {
                Layout.fillWidth: true
                GroupBox {
                    id: propertiesGB
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    z: -1

                    //  Groupbox label
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
            }
            //  Overrides
            Rectangle {
                id: overrides
                Layout.fillHeight: true
                Layout.margins: 0
                implicitWidth: 225

                color: palette.window
                border.color: palette.windowText
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    Rectangle {
                        color: 'red'
                        Layout.preferredWidth: 225
                        Layout.topMargin: 15

                        Ui.ColorSettings {
                            id: settings
                            enabled: !Theme.systemTheme
                            anchors.fill: parent

                            color: palette.window

                            //  Currently selected preference
                            //preference: root.model.currentPref
                            model: root.model
                        }
                    }

                    //  Do not hide control or ColumnLayout will move ColorSettings
                    //  Hide text instead to keep consistent layout.
                    Text {
                        Layout.topMargin: 140
                        Layout.fillWidth: true
                        Layout.fillHeight: true
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
