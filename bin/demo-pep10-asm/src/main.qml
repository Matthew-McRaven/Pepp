import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 1.15
import edu.pepperdine.cslab.p10asm
Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    AssemblyManager {
      id: asm
      Component.onCompleted: {
        asm.emitUserListing.connect(userListing.onAssemble)
        asm.emitOSListing.connect(osListing.onAssemble)
      }
    }

    FigureManager {
      id: figs
      function onSelectionChanged(index) {
        const fig = figureAt(index);
        if(!fig) return
        selected(fig);
      }
      Component.onCompleted: {
        selected(0)
        selected.connect(userSource.onSelectionChanged)
        selected.connect(osSource.onSelectionChanged)
        selected.connect(asm.onSelectionChanged)
      }

      signal selected(figure: Figure)
    }

    Rectangle {
        anchors.fill: parent
        GridLayout{
        columns: 2
          ComboBox {
            id: choices
            model: figs.figures()
            Component.onCompleted: {
              choices.activated.connect(figs.onSelectionChanged)
              activated.connect(asm.clearUsrTxt)
              activated.connect(asm.clearOsTxt)
            }
          }
          Button {
            id: assembleNow
            Text {
              text: "Assemble"
            }
            Component.onCompleted: {
              clicked.connect(asm.onAssemble)
            }
          }
        }
        Rectangle{
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height *.9
            anchors.bottom:  parent.bottom
            ScrollView {
                width: parent.width *.5
                height: parent.height *.5
                anchors.left: parent.left
                anchors.top: parent.top
                TextArea {
                  id: userSource
                  anchors.fill: parent
                  readOnly: true
                  placeholderText: "User Source"
                  font.family: "Courier New"
                  function onSelectionChanged(user: Figure) {
                    if(!user) return;
                    userSource.text = user.elements["pep"].content
                  }
              }
            }
            ScrollView {
              width: parent.width *.5
              height: parent.height *.5
              anchors.left: parent.left
              anchors.bottom: parent.bottom
              TextArea{
                id: userListing
                anchors.fill: parent
                readOnly: true
                placeholderText: "User Listing"
                font.family: "Courier New"
                text:asm.usrTxt
              }
            }
            ScrollView {
              width: parent.width *.5
              height: parent.height *.5
              anchors.right: parent.right
              anchors.top: parent.top
              TextArea {
                id: osSource
                anchors.fill: parent
                readOnly: true
                placeholderText: "OS Source"
                  font.family: "Courier New"
                function onSelectionChanged(user: Figure) {
                  if(!user) return;
                  const os = user.defaultOS;
                  if(!os) return;
                  osSource.text = os.elements["pep"].content
                }
              }
            }
            ScrollView {
              width: parent.width *.5
              height: parent.height *.5
              anchors.right: parent.right
              anchors.bottom: parent.bottom
              TextArea {
                id: osListing
                anchors.fill:parent
                readOnly: true
                placeholderText: "OS Listing"
                font.family: "Courier New"
                text: asm.osTxt
              }
            }
        }
    }

}
