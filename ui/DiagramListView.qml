pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var currentStamp: null

    function setStamp(index){
        root.currentStamp = diagramList.diagramTemplate(index);
        console.log("Current:", root.currentStamp.name)
        if(root.currentStamp.name === "Move")
        {
            root.currentStamp = null;
        }
        else if(root.currentStamp.shapeType === "Diagram")
        {
            //canvas.curIndex = index
        }
        else
        {
            //  Disable stamp for lines
            //canvas.curStamp = null;
        }
    }

    DiagramListModel {
        id: diagramList
    }

    ButtonGroup {
        id: buttonGroup
        buttons: source.children.filter(child => child !== rep)

        //  Make sure first button is selected on startup
        Component.onCompleted: {
            root.setStamp(0);
            buttonGroup.buttons[0].checked = true;
        }
        onClicked: btn => {
            root.setStamp(btn.index);
            //console.log(btn.index);
        }
    }

    GridLayout {
        id: source
        columns: 2
        columnSpacing: 0
        rowSpacing: 0

        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true

        Repeater {
            id: rep
            model: diagramList
            delegate: Button {
                id: btn
                required property string name
                required property string file
                required property string qrcFile
                required property int index

                implicitWidth: 100
                implicitHeight: 60
                checkable: true
                display: AbstractButton.TextUnderIcon

                text: btn.name
                icon.source: btn.qrcFile//btn.file
                icon.color: "transparent"
                icon.width: btn.implicitWidth * .8
                icon.height: btn.implicitHeight * .7
            }
        }
    }   //  GridLayout
}
