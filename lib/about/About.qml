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

Item {
    id: root
    focus: true

    implicitWidth: 500
    implicitHeight: 800

    width: Math.min(implicitWidth * 1.1, parent.width * .5)
    height: Math.min(implicitHeight, parent.height * .75)
    anchors.centerIn: parent
    FontMetrics {
        id: fontMetrics
    }
    //  Figure contents
    ScrollView {
        anchors.fill: parent
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentWidth: parent.width
        ColumnLayout {
            anchors.fill: parent
            Text {
                id: title
                color: palette.windowText
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
            Label {
                Layout.fillWidth: true
                text: qsTr("Programmed By:")
                font.bold: true
            }
            Column {
                Layout.fillWidth: true
                Layout.fillHeight: true
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
                text: qsTr("Previous Contributions By:")
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: Contributors.text
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Legal:")
                font.bold: true
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Copyright © 2016 - 2025, J. Stanley Warford, Matthew McRaven, Pepperdine University\n")
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                Layout.preferredWidth: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n\n")
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("<html>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License along with this program. If not, see <a href=\"https://www.gnu.org/licenses/.\">https://www.gnu.org/licenses/</a><br/></html>")

                Layout.preferredWidth: parent.width
                wrapMode: Text.WordWrap
                onLinkActivated: link => Qt.openUrlExternally(link)
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Dependencies:")
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

                Layout.fillWidth: true
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
            Text {
                id: projectUrl
                Layout.fillWidth: true
                color: palette.windowText
                onLinkActivated: link => {
                    Qt.openUrlExternally(link);
                }
            }
            TextArea {
                id: projectLicense
                Layout.fillWidth: true
                readOnly: true
                wrapMode: Text.WordWrap
            }
        }
    } //  ScrollView
}
