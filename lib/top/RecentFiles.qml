pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material    //  Used by TabBar
import QtQuick.Layouts

Item {
    id: root
    property int spacing: 10
    property int buttonWidth: 75
    required property var font

    property color borderColor: settings.extPalette.brightText.background
    property var recentFiles: settings.general.recentFiles

    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width

    ColumnLayout {
        TabBar {
            id: bar
            implicitWidth: root.width
            currentIndex: root.recentFiles.length > 0 ? 0 : 1

            //  This erases underline that spans page under tabs
            Material.accent: "transparent"
            TabButton {
                id: tab1
                visible: root.recentFiles.length > 0
                width: root.recentFiles.length > 0 ? root.buttonWidth : 0

                contentItem: Text {
                    id: txt1
                    text: "Recent"
                    horizontalAlignment: Text.AlignHCenter
                    color: tab1.checked ? palette.text : settings.extPalette.brightText.background
                }

                //  Replace existing tab background
                background: Rectangle {
                    id: bg1
                    anchors.fill: tab1

                    //  Show underline when active
                    Rectangle {
                        visible: tab1.checked
                        anchors.bottom: bg1.bottom
                        anchors.bottomMargin:  4
                        anchors.horizontalCenter: bg1.horizontalCenter

                        height: 4
                        width: txt1.contentWidth
                        color: palette.highlight
                        radius: 2
                    }
                }   //  background: Rectangle
            }   //  TabButton - tab1
            TabButton {
                id: tab2
                width: root.buttonWidth

                contentItem: Text {
                    id: txt2
                    text: "Favorites"
                    horizontalAlignment: Text.AlignHCenter
                    color: tab2.checked ? palette.text : settings.extPalette.brightText.background
                }

                //  Replace existing tab background
                background: Rectangle {
                    id: bg2
                    anchors.fill: tab2

                    //  Show underline when active
                    Rectangle {
                        visible: tab2.checked
                        anchors.bottom: bg2.bottom
                        anchors.bottomMargin:  4
                        anchors.horizontalCenter: bg2.horizontalCenter

                        height: 4
                        width: txt2.contentWidth
                        color: palette.highlight
                        radius: 2
                    }
                }   //  background: Rectangle
            }   //  TabButton - tab2
        }   //  TabBar

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: bar.currentIndex
            //  Existing Project
            Rectangle {
                id: existingProject
                Layout.fillWidth: true

                function abstractText(value) {
                    switch(value) {
                    case 20: return "MC2";
                    case 30: return "ISA3";
                    case 31: return "ASMB3";
                    case 40: return "OS4";
                    case 50: return "ASMB5";
                    }
                }

                function archText(value) {
                    switch(value) {
                    case 100: return "Pep/10"
                    case 1000: return "RISC-V"
                    case 90: return "Pep/9"
                    case 80: return "Pep/8"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Repeater {
                        id: repeaterExisting
                        Layout.fillWidth: true
                        model: settings.general.recentFiles

                        //  Delegate for file listing
                        Rectangle {
                            id: btn
                            implicitHeight: Math.max(60,childrenRect.height)
                            implicitWidth: 300

                            Layout.topMargin: root.spacing

                            required property int index
                            required property var model

                            radius: 5
                            border.width: 1
                            border.color: root.borderColor
                            color: palette.window
                            ColumnLayout {

                                anchors.left: btn.left
                                anchors.top: btn.top
                                anchors.right: btn.right
                                anchors.bottom: btn.bottom
                                anchors.leftMargin: 10

                                implicitHeight: childrenRect.height
                                implicitWidth: childrenRect.width
                                spacing: 0

                                Item{
                                    //  Spacer
                                    Layout.fillHeight: true
                                }
                                Label {
                                    //  File name
                                    Layout.fillWidth: true
                                    text: settings.general.fileNameFor(btn.model.path)
                                    font.bold: true
                                    font.pointSize: root.font.pointSize * 1.2
                                    color: palette.accent
                                }
                                Label {
                                    //  Architecture details
                                    Layout.fillWidth: true
                                    text: "<b>" + existingProject.archText(btn.model.arch) + "</b>: " + existingProject.abstractText(btn.model.abstraction)
                                }
                                Label {
                                    //  Full file path
                                    text: btn.model.path
                                    Layout.fillWidth: true
                                }
                                Item{
                                    //  Spacer
                                    Layout.fillHeight: true
                                }
                            }   //  ColumnLayout
                        }   //  Rectangle - delegate
                    }   //  Repeater
                }   //  ColumnLayout
            }
            //  Favorites
            Rectangle {
                id: favoriteProject
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    Repeater {
                        id: repeaterFavorite
                        Layout.fillWidth: true
                        model: ListModel {
                            id: modelFavorite
                            ListElement {
                                project: "4.20"
                                path: "/pep/directory/fig420.pep"
                            }
                            ListElement {
                                project: "4.26"
                                path: "/pep/directory/fig426.pep"
                            }
                            ListElement {
                                project: "4.27"
                                path: "/pep/directory/fig427.pep"
                            }
                        }
                        Rectangle {
                            id: btn2
                            implicitHeight: 50
                            implicitWidth: 300
                            required property int index
                            required property string project
                            required property string path

                            radius: 5
                            border.width: 1
                            border.color: root.borderColor
                            color: palette.window
                            RowLayout {

                                anchors.left: btn2.left
                                anchors.top: btn2.top
                                anchors.right: btn2.right
                                anchors.bottom: btn2.bottom
                                anchors.margins: 10

                                spacing: 0

                                Column {
                                    Layout.fillHeight: true
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 10
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                                    Label {
                                        //id: proj
                                        text: "<b>Figure:<b> " + btn2.project
                                        color: root.borderColor
                                    }
                                    Label {
                                        text: btn2.path
                                    }
                                }
                            }
                        }
                    }   //  Repeater
                }   //  ColumnLayout
            }
        }
    }   //  ColumnLayout

}
