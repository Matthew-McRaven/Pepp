import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import edu.pepp 1.0

Dialog {
    id: root

    title: qsTr("Self-Test GUI")
    modal: false
    dim: false
    focus: true

    implicitWidth: 500
    implicitHeight: 800
    width: Math.min(implicitWidth * 1.1, parent.width * .5)
    height: Math.min(implicitHeight, parent.height * .75)
    SelfTestModel {
        id: selfTestModel
    }
    GridLayout {
        id: buttons
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottomMargin: 10
        columns: 4
        Button {
            id: enableVisble
            text: qsTr("Enable Visible")
            enabled: !selfTestModel.running
        }
        Button {
            id: enableAll
            text: qsTr("Enable all")
            enabled: !selfTestModel.running
        }
        Label {
            text: "Test filter"
        }

        TextField {
            id: filter
            placeholderText: qsTr("*")
            Layout.fillWidth: true
            enabled: !selfTestModel.running
        }
        Button {
            id: disableVisble
            text: qsTr("Disable Visible")
            enabled: !selfTestModel.running
        }
        Button {
            id: disableAll
            text: qsTr("Disable all")
            enabled: !selfTestModel.running
        }
        Label {
            text: "Working Directiory"
        }
        TextField {
            id: cwd
            placeholderText: qsTr("/path/to/working/directory")
            Layout.fillWidth: true
            enabled: !selfTestModel.running
        }
        Button {
            id: runVisble
            text: qsTr("Run enabled")
            onClicked: selfTestModel.runSelectedTests()
            enabled: !selfTestModel.running
        }
        Button {
            id: runAll
            text: qsTr("Run all")
            onClicked: selfTestModel.runAllTests()
            enabled: !selfTestModel.running

        }
        Label {
            Layout.columnSpan: 2
            property int testCount: selfTestModel.rowCount()
            property int visibleCount: selfTestModel.visibleTests
            property int selectedCount: selfTestModel.selectedTests
            text: `Showing ${visibleCount} of ${testCount} tests; ${selectedCount} of ${testCount} are enabled`
        }
    }
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.top: buttons.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        syncView: tableView
        clip: true
        textRole: "display"
        delegate: Text {
            text: model.display
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.bold: true
            color: palette.text
        }
    }
    DelegateChooser{
        id: chooser
        role: "type"
        DelegateChoice {
            roleValue: "text";
            Text {
                text: model.display
                color: palette.text
                clip: true
            }
        }
        DelegateChoice {
            roleValue: "check";
            CheckBox {
                checked: model.display
                clip: true
                onToggled: {
                    selfTestModel.setData(selfTestModel.index(model.row, 1), checked, "display")
                }
            }
        }
    }

    TableView {
        id: tableView
        clip: true
        anchors {
            top: horizontalHeader.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        model: selfTestModel
        delegate: chooser

    }
}
