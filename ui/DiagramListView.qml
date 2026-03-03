pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var currentStamp: null
    property alias filterList: filterModel
    property alias diagramOnly: diagramOnlyList

    function setStamp(index){
        root.currentStamp = diagramModel.diagramTemplate(index);
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
            canvas.curStamp = null;
        }
    }

    DiagramListModel {
        id: diagramModel
    }

    //  Filter list for properties box
    FilterDiagramListModel {
        id: filterModel
        model: diagramModel
        filter: FilterDiagramListModel.Arrow
    }

    //  Filter list for properties box
    FilterDiagramListModel {
        id: diagramOnlyList
        model: diagramModel
        filter: FilterDiagramListModel.Diagram
    }

    /*SortFilterProxyModel {
        id: diagramOnlyList
        model: diagramModel

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
    }*/

    ButtonGroup {
        id: buttonGroup
        buttons: source.children.filter(child => child !== rep)

        Component.onCompleted: {
            root.setStamp(0);
        }
        onClicked: btn => {
            //  Index is injected by Repeater delegate. Does not exist as buttonGroup property
            root.setStamp(btn.index);
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
            model: filterModel
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
                icon.width: btn.implicitWidth * .5
                icon.height: btn.implicitHeight * .55
            }
        }
    }   //  GridLayout
}
