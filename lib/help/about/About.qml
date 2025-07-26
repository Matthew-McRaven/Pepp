/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import edu.pepp 1.0
import "../about" as About

Item {
    id: root
    focus: true

    implicitWidth: 500
    implicitHeight: 800
    property int implicitButtonWidth: 100
    property int paragraphSpace: 8
    property int sideMargin: 5

    width: Math.min(implicitWidth * 1.1, parent.width * .5)
    height: Math.min(implicitHeight, parent.height * .75)
    anchors.centerIn: parent
    FontMetrics {
        id: fontMetrics
    }

    TabBar {
        id: helpBar
        width: parent.width
        TabButton {
            text: "Pepp"
            width: implicitButtonWidth
        }
        TabButton {
            text: "Change Log"
            width: implicitButtonWidth
        }
        TabButton {
            text: "System Info"
            width: implicitButtonWidth
        }
        TabButton {
            text: "Dependencies"
            width: implicitButtonWidth
        }
    }

    //  All Screens, selection based on current index
    StackLayout {
        currentIndex: helpBar.currentIndex
        anchors {
            top: helpBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        //  Pep About screen
        Item {
            id: pepAbout
            Layout.fillHeight: true
            Layout.fillWidth: true
            Rectangle {
                anchors.fill: parent
                color: palette.base
                border.width: 1
                border.color: palette.shadow
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    RowLayout {
                        Rectangle {
                            // Rectangle is only for testing. When image works, remove this control
                            color: "yellow"
                            width: logo.width
                            height: logo.height

                            Image {
                                id: logo
                                source: "file:///E:\Projects\QTProjects\Pepp\lib\help\about\icon.png"
                                fillMode: Image.PreserveAspectFit
                                width: 75
                                height: logo.width
                            }
                        }
                        Text {
                            id: title
                            color: palette.windowText
                            Layout.margins: root.sideMargin
                            onLinkActivated: link => {
                                Qt.openUrlExternally(link);
                            }
                            // Too much text to assign in binding, so build it inline instead.
                            Component.onCompleted: {
                                let line0 = "<h2>Pepp version %1</h2> <a href=\"https://github.com/Matthew-McRaven/Pepp/releases\">Check for updates</a>.  ".arg(Version.version_str_full);
                                let url = "https://github.com/Matthew-McRaven/Pepp/commit/" + Version.git_sha;
                                let line1 = "Based on <a href=\"" + url + "\">";
                                line1 += Version.git_tag !== "unknown" ? Version.git_tag : Version.git_sha.substring(0, 7);
                                line1 += "</a>.";
                                text = line0 + line1;
                            }
                        }
                    } //  RowLayout-logo

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: root.paragraphSpace
                        Layout.leftMargin: root.sideMargin
                        Layout.rightMargin: root.sideMargin
                        text: qsTr("Programmed By:")
                        font.bold: true
                    }
                    Column {
                        Layout.fillWidth: true
                        Layout.topMargin: 0
                        Layout.margins: root.sideMargin
                        Repeater {
                            model: MaintainerList {}

                            Label {
                                width: parent.width
                                height: fontMetrics.height
                                required property var item
                                text: item.name + "  <" + item.email + ">"
                            }
                            height: model.rowCount() * fontMetrics.height
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: root.paragraphSpace
                        Layout.leftMargin: root.sideMargin
                        Layout.rightMargin: root.sideMargin
                        text: qsTr("Previous Contributions By:")
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: 0
                        Layout.margins: root.sideMargin
                        wrapMode: Text.WordWrap
                        text: Contributors.text
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: root.paragraphSpace
                        Layout.leftMargin: root.sideMargin
                        Layout.rightMargin: root.sideMargin
                        text: qsTr("Legal:")
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: root.sideMargin
                        Layout.rightMargin: root.sideMargin
                        text: qsTr("Copyright © 2016 - 2025, J. Stanley Warford, Matthew McRaven, Pepperdine University")
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.margins: root.sideMargin
                        wrapMode: Text.WordWrap
                        text: qsTr("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.")
                    }
                    Label {
                        Layout.fillWidth: true
                        Layout.margins: root.sideMargin
                        text: qsTr("<html>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License along with this program. If not, see <a href=\"https://www.gnu.org/licenses/.\">https://www.gnu.org/licenses/</a></html>")

                        //Layout.preferredWidth: parent.width
                        wrapMode: Text.WordWrap
                        onLinkActivated: link => Qt.openUrlExternally(link)
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: root.sideMargin

                        border.width: 1
                        border.color: palette.shadow

                        ScrollView {
                            anchors.fill: parent
                            TextArea {
                                id: license
                                readOnly: true
                                text: FileReader.readFile(":/about/LICENSE_FULL")
                            }
                            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
                        } //  ScrollView
                    }
                } //ColumnLayout
            } //  Rectangle
        } //  Item - pepAbout
        //  Change Log screen
        Item {
            id: changeLog
            Layout.fillHeight: true
            Layout.fillWidth: true
            Rectangle {
                anchors.fill: parent
                color: palette.base
                border.width: 1
                border.color: palette.shadow
                Text {
                    // Replace this control with Change control screen
                    text: "Change Log"
                } //  End of replacement
            }
        } //  Item - changeLog
        //  System Info screen
        Item {
            id: systemInfo
            Layout.fillHeight: true
            Layout.fillWidth: true
            Rectangle {

                anchors.fill: parent
                color: palette.base
                border.width: 1
                border.color: palette.shadow
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    About.Diagnostics {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: root.sideMargin
                    } //  Diagnostics
                }
            }
        } //  Item - systemInfo
        //  Third party license screen
        Item {
            id: thirdParty
            Layout.fillHeight: true
            Layout.fillWidth: true

            Rectangle {
                anchors.fill: parent
                color: palette.base
                border.width: 1
                border.color: palette.shadow

                //  Dependencies
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: root.sideMargin
                        Label {
                            text: qsTr("Third Party Licenses:")
                            font.bold: true
                            wrapMode: Text.WordWrap
                        }
                        ComboBox {
                            id: projectCombo
                            Component.onCompleted: {
                                // Force the correct license to be selected on load.
                                projectCombo.onCurrentIndexChanged();
                                // onCurrentIndexChanged not called automatically, so we must connect to the appropriate signal.
                                projectCombo.currentIndexChanged.connect(projectCombo.onCurrentIndexChanged);
                            }

                            Layout.preferredWidth: 160
                            Layout.minimumWidth: 160
                            Layout.maximumWidth: 160
                            model: Dependencies
                            currentIndex: 0
                            textRole: "name"
                            function onCurrentIndexChanged() {
                                let index = model.index(currentIndex, 0);
                                projectLicense.text = model.data(index, DependencyRoles.LicenseText);
                                let url = model.data(index, DependencyRoles.URL);
                                projectUrl.text = "<a href=\"" + url + "\">" + url + "</a>";
                            }
                        }
                        Item {
                            id: spacer
                            Layout.fillWidth: true
                        }
                    } //Row
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.margins: root.sideMargin
                        Label {
                            text: qsTr("Link:")
                            font.bold: true
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            id: projectUrl
                            Layout.fillWidth: true
                            color: palette.windowText
                            onLinkActivated: link => {
                                Qt.openUrlExternally(link);
                            }
                        }
                    }

                    ScrollView {
                        id: sv
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        TextArea {
                            id: projectLicense
                            readOnly: true
                        }
                    } //  ScrollView
                } //ColumnLayout
            } //  Rectangle
        } //  Item - third party
    } //  StackLayout
}
