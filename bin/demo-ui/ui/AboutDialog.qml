import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window

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

  //  Figure contents
  ScrollView {
    anchors.fill: parent

    Column {
      spacing: 10

      Text {
        text: qsTr("<html><h2>Pep/10 version %1</h2> <a href=\"https://www.pepperdine.edu//\">Check for updates</a><br></html>").arg(Qt.application.version)
        onLinkActivated: Qt.openUrlExternally(link)
        MouseArea {
          anchors.fill: parent
          acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
          cursorShape: Qt.PointingHandCursor
        }
      }

      Label {
        text: qsTr("Programmed By:")
        font.bold: true
        font.pixelSize: Qt.application.font.pixelSize
      }

      Label {
        text: qsTr("Matthew McRaven (Matthew.McRaven@pepperdine.edu)\nJ. Stanley Warford (Stan.Warford@pepperdine.edu)\n")
        font.pixelSize: Qt.application.font.pixelSize
      }

      Label {
        text: qsTr("Previous Contributions By:")
        font.bold: true
      }

      Label {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("David McRaven, Emily Dimpfl, Tip Aroonvatanaporn, Deacon Bradley, Jeff Cook, Nathan Counts, Stuartt Fox, Dave Grue, Justin Haight, Paul Harvey, Hermi Heimgartner, Matt Highfield, Trent Kyono, Malcolm Lipscomb, Brady Lockhart, Adrian Lomas, Ryan Okelberry, Thomas Rampelberg, Mike Spandrio, Jack Thomason, Daniel Walton, Di Wang, Peter Warford, and Matt Wells.\n")
      }

      Label {
        text: qsTr("License:")
        font.bold: true
        wrapMode: Text.WordWrap
      }

      Label {
        text: qsTr("Copyright © 2016 - 2023, J. Stanley Warford, Pepperdine University\n")
      }

      Label {
        width: parent.width
        wrapMode: Text.WordWrap
        text: qsTr("This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.\n\n")
      }

      Text {
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
