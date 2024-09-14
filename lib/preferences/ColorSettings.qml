import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform

//  Font picker
import "." as Ui

Rectangle {
    id: root

    //  Used for updates
    required property var model

    border.color: palette.windowText
    border.width: 1

    ColumnLayout {
        id: wrapper
        Layout.margins: 5
        anchors.fill: parent
        property int colWidth: 70
        GroupBox {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 200

            //  Groupbox label
            label: Ui.GroupBoxLabel {
                textColor: palette.windowText
                backgroundColor: palette.window
                text: "Update " + root.model.currentPref.name
            }

            background: Rectangle {
                color: palette.window
                border.color: palette.windowText
                border.width: 1
            }

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true

                RowLayout {
                    id: foreground
                    spacing: 5
                    height: 20
                    Layout.alignment: Qt.AlignLeft
                    Layout.fillWidth: true
                    Label {
                        text: "Foreground:"
                        Layout.preferredWidth: wrapper.colWidth * 1.25
                        Layout.preferredHeight: 20
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Ui.ColorButton {
                        id: fgText
                        Layout.preferredWidth: wrapper.colWidth
                        Layout.preferredHeight: 20
                        color: root.model.currentPref.foreground

                        onUpdatedColor: newColor => {
                                            if (newColor !== root.model.currentPref.foreground) {

                                                //  Update model. Model will trigger screen repaint
                                                model.updatePreference(
                                                    root.model.currentPref.id,
                                                    1, newColor)
                                                //console.log("After pref.foreground: " + model.currentPref.foreground)
                                            }
                                        }
                    }

                    /*Button {
            text: "Clear"
            enabled: parentId.currentIndex === 0 || fgColor.color !== pfgColor.color
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            onClicked: {
              if(parentId.currentIndex === 0) {
                fgColor.color = preference.foreground
                fgText.text = preference.foreground
              }
              else {
                fgColor.color = pfgColor.color
                fgText.text = pfgText.text
              }
            }
          }*/
                }
                RowLayout {
                    id: background
                    spacing: 5
                    height: 20
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft
                    Label {
                        text: "Background:"
                        Layout.preferredWidth: wrapper.colWidth * 1.25
                        Layout.preferredHeight: 20
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                    }

                    Ui.ColorButton {
                        id: bgText
                        Layout.preferredWidth: wrapper.colWidth
                        Layout.preferredHeight: 20
                        color: root.model.currentPref.background

                        onUpdatedColor: newColor => {
                                            if (newColor !== root.model.currentPref.background) {

                                                //  Update model. Model will trigger screen repaint
                                                model.updatePreference(
                                                    root.model.currentPref.id,
                                                    2, newColor)
                                                //console.log("After pref.foreground: " + model.currentPref.background)
                                            }
                                        }
                    }


                    /*Button {
            text: "Clear"
            enabled: parentId.currentIndex === 0 || bgColor.color !== pbgColor.color
            Layout.preferredWidth: wrapper.colWidth
            Layout.preferredHeight: 20
            onClicked: {
              if(parentId.currentIndex === 0) {
                bgColor.color = "transparent"
                bgText.text = "transparent"
              }
              else {
                bgColor.color = pbgColor.color
                bgText.text = pbgText.text
              }
            }
          } */
                }

                RowLayout {
                    spacing: 5
                    height: 30
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft

                    Label {
                        text: "Font Properties"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignBottom
                    }
                }

                RowLayout {
                    Ui.FontProperties {
                        Layout.fillWidth: true
                        height: 40
                        Layout.alignment: Qt.AlignLeft

                        bold: root.model.currentPref.bold
                        italics: root.model.currentPref.italics
                        underline: root.model.currentPref.underline
                        strikeout: root.model.currentPref.strikeout

                        onUpdatedFont: (prop, flag) => {

                                           //  Update model. Model will trigger screen repaint
                                           model.updatePreference(
                                               root.model.currentPref.id,
                                               prop, flag)
                                           //console.log("After pref.foreground: " + prop +"="+flag)
                                       }
                    }
                }

                RowLayout {
                    //  Empty space at bottom of pane
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "transparent"
                    }
                }
            }
        }
    }
}
