import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
        }
        Button {
            id: enableAll
            text: qsTr("Enable all")
        }
        Label {
            text: "Test filter"
        }

        TextField {
            id: filter
            placeholderText: qsTr("*")
            Layout.fillWidth: true
        }
        Button {
            id: disableVisble
            text: qsTr("Disable Visible")
        }
        Button {
            id: disableAll
            text: qsTr("Disable all")
        }
        Label {
            text: "Working Directiory"
        }
        TextField {
            id: cwd
            placeholderText: qsTr("/path/to/working/directory")
            Layout.fillWidth: true
        }
        Button {
            id: runVisble
            text: qsTr("Run enabled")
        }
        Button {
            id: runAll
            text: qsTr("Run all")
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
        delegate: Text {
            text: model.display
            color: palette.text
        }
    }
}
