import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

//  Font picker
import "." as Ui

Item {
    id: wrapper

    //  Used for updates
    required property var model
    property alias color: backgroundRect.color
    implicitWidth: layout.childrenRect.width
    implicitHeight: layout.childrenRect.height

    //onImplicitWidthChanged: console.log(implicitWidth, implicitHeight)
    //onImplicitHeightChanged: console.log(implicitWidth, implicitHeight)
    GroupBox {
        id: layout
        label: Ui.GroupBoxLabel {
            textColor: palette.windowText
            backgroundColor: palette.window
            text: "Update " + root.model.currentPref.name
        }
        background: Rectangle {
            id: backgroundRect
            color: palette.window
            border.color: palette.windowText
            border.width: 1
        }
        GridLayout {
            columns: 2
            anchors.fill: parent
            Label {
                text: "Foreground:"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }
            Ui.ColorButton {
                id: fgText
                color: root.model.currentPref.foreground

                onUpdatedColor: function (newColor) {
                    if (newColor !== root.model.currentPref.foreground) {

                        model.updatePreference(root.model.currentPref.id,
                                               1, newColor)
                    }
                }
            }
            Label {
                text: "Background:"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            Ui.ColorButton {
                id: bgText
                color: root.model.currentPref.background

                onUpdatedColor: function (newColor) {
                    if (newColor !== root.model.currentPref.background) {

                        model.updatePreference(root.model.currentPref.id,
                                               2, newColor)
                    }
                }
            }
            Label {
                text: "Font Properties: "
                Layout.preferredHeight: 30
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignBottom
            }

            Ui.FontProperties {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft

                bold: root.model.currentPref.bold
                italics: root.model.currentPref.italics
                underline: root.model.currentPref.underline
                strikeout: root.model.currentPref.strikeout

                onUpdatedFont: function (prop, flag) {
                    model.updatePreference(root.model.currentPref.id,
                                           prop, flag)
                }
            }
            //  Empty space at bottom of pane
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: 2
                color: "transparent"
            }
        }
    }
}
