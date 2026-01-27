import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import QtQuick.Effects
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
    SelfTestFilterModel {
        id: selfTestFilterModel
        sourceModel: selfTestModel
        regex: filter.text
    }
    component DisableableButton: Button {
        id: control
        enabled: !selfTestModel.running
        layer.enabled: !enabled
        layer.effect: MultiEffect {
            colorization: 0.75
            colorizationColor: palette.button      // theme-derived tint color
        }
    }

    GridLayout {
        id: buttons
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottomMargin: 10
        columns: 4
        DisableableButton {
            id: enableVisble
            text: qsTr("Enable Visible")
            enabled: !selfTestModel.running
        }
        DisableableButton {
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
        DisableableButton {
            id: disableVisble
            text: qsTr("Disable Visible")
            enabled: !selfTestModel.running
        }
        DisableableButton {
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
        Button{
            id: runVisble
            text: selfTestModel.running ? qsTr("Stop tests") : qsTr("Run enabled")
            onClicked: selfTestModel.running ? selfTestModel.stop() : selfTestModel.runSelectedTests()
        }
        Button {
            id: runAll
            text: selfTestModel.running ? qsTr("Stop tests") : qsTr("Run all")
            onClicked: selfTestModel.running ? selfTestModel.stop() : selfTestModel.runAllTests()
        }
        Label {
            Layout.columnSpan: 2
            property int testCount: selfTestModel.rowCount()
            property int visibleCount: selfTestFilterModel.rowCount()
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
    DelegateChooser {
        id: chooser
        role: "type"
        DelegateChoice {
            roleValue: "text"
            Text {
                text: model.display
                color: palette.text
                clip: true
            }
        }
        DelegateChoice {
            roleValue: "check"
            CheckBox {
                checked: model.display
                clip: true
                onToggled: {
                    tableView.model.setData(tableView.model.index(model.row, 1), checked, "display");
                }
            }
        }
    }

    TableView {
        id: tableView
        clip: true
        anchors {
            top: horizontalHeader.bottom
            bottom: progress.visible? progress.top : parent.bottom
            left: parent.left
            right: parent.right
        }
        model: selfTestFilterModel
        delegate: chooser
    }
    ProgressBar {
        id: progress
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        visible: selfTestModel.running
        value: selfTestModel.progress
        from: 0
        to: selfTestModel.rowCount()
    }
}
