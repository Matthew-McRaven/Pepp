pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var blueprint: null
    //  List of available blueprints for current project
    required property BlueprintLibraryModel blueprintModel

    ButtonGroup {
        id: buttonGroup
        buttons: source.children.filter(child => child !== rep)

        Component.onCompleted: {
            const btn = buttons[0];
            root.blueprint = btn.id;
            btn.checked = true;
        }
        onClicked: btn => {
            //  Id is injected by Repeater delegate. Does not exist as buttonGroup property
            root.blueprint = btn.id;
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
            model: blueprintModel
            delegate: Button {
                id: btn
                required property string name
                required property var path
                required property int id

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
