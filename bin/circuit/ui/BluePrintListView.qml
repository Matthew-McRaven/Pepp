pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var blueprint: null
    property alias project: diagramModel.project
    //property alias diagramOnly: diagramOnlyList

    function setStamp(index) {
        root.blueprint = diagramModel.blueprint(index);
    }

    BlueprintLibraryModel {
        id: diagramModel
    }

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
            model: diagramModel
            delegate: Button {
                id: btn
                required property string name
                required property var path

                implicitWidth: 100
                implicitHeight: 60
                padding: 5
                checkable: true
                display: AbstractButton.TextUnderIcon

                text: btn.name
                icon.source: btn.path
                icon.color: "transparent"
                icon.width: btn.implicitWidth * .5
                icon.height: btn.implicitHeight * .55
            }
        }
    }   //  GridLayout
}
