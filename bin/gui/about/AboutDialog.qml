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

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import edu.pepp 1.0

Dialog {
    id: aboutDialog
    title: qsTr("About Pep/10")
    modal: true
    dim: false
    focus: true

    implicitWidth: 500
    implicitHeight: 800

    standardButtons: Dialog.Ok
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
                Layout.fillWidth: true
                text: qsTr("<html><h2>Pepp version %1</h2> <a href=\"https://github.com/Matthew-McRaven/Pepp/releases\">Check for updates</a><br></html>").arg(Version.version_str_full)
                onLinkActivated: Qt.openUrlExternally(link)
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
                    cursorShape: Qt.PointingHandCursor
                }
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Programmed By:")
                font.bold: true
                font.pixelSize: Qt.application.font.pixelSize
            }
            Column {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Repeater {
                    model: Maintainers
                    Text {
                        width:parent.width
                        height: fontMetrics.height
                        required property string name;
                        required property string email;
                        text: name + "  <" + email + ">";
                    }
                    height: Maintainers.rowCount * fontMetrics.height
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
                text: qsTr("David McRaven, Emily Dimpfl, Tip Aroonvatanaporn, Deacon Bradley, Jeff Cook, Nathan Counts, Stuartt Fox, Dave Grue, Justin Haight, Paul Harvey, Hermi Heimgartner, Matt Highfield, Trent Kyono, Malcolm Lipscomb, Brady Lockhart, Adrian Lomas, Ryan Okelberry, Thomas Rampelberg, Mike Spandrio, Jack Thomason, Daniel Walton, Di Wang, Peter Warford, and Matt Wells.\n")
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("License:")
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Copyright © 2016 - 2024, J. Stanley Warford, Matthew McRaven, Pepperdine University\n")
            }

            Label {
                Layout.fillWidth: true
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n\n")
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("<html>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License along with this program. If not, see <a href=\"https://www.gnu.org/licenses/.\">https://www.gnu.org/licenses/</a><br/></html>")

                width: parent.width
                wrapMode: Text.WordWrap
                onLinkActivated: Qt.openUrlExternally(link)
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
                    cursorShape: Qt.PointingHandCursor
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Pep/10 is programmed using QT. Learn more at <html><a href=\"https://www.qt.io/\">Qt Group</a></html>")
                wrapMode: Text.WordWrap
                onLinkActivated: Qt.openUrlExternally(link)
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
    } //  ScrollView
}
