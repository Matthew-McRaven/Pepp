pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var currentStamp: null
    property alias filterList: filterModel

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

    //  Filter list for properties box
    SortFilterProxyModel {
        id: filterModel
        model: diagramList

        // Filter based on whether the 'shapeType' role
        filters: [
            FunctionFilter {
                function filter(data: RoleData): bool {
                    return data.shapeType === "Diagram";
                }
            }
        ]
    }
    component RoleData: QtObject {
        property string shapeType
    }

    ButtonGroup {
        id: buttonGroup
        buttons: source.children.filter(child => child !== rep)

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
                padding: 5
                checkable: true
                display: AbstractButton.TextUnderIcon

                text: btn.name
                icon.source: btn.qrcFile
                icon.color: "transparent"
                icon.width: btn.implicitWidth * .7
                icon.height: btn.implicitHeight * .5
            }
        }
    }   //  GridLayout
}
