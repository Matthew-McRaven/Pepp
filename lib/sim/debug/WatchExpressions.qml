import QtQuick
import QtQuick.Controls
import edu.pepp

// Must be focus scope or Keys.onPressed will not work
FocusScope {
    id: root
    NuAppSettings {
        id: settings
    }
    FontMetrics {
        id: fm
        font: settings.extPalette.baseMono.font
    }
    signal updateGUI
    Component.onCompleted: {
        updateGUI.connect(tableModel.onUpdateModel)
    }
    Rectangle {
        id: outline
        color: palette.base
        anchors.fill: parent
        //  Give object code viewer a background box
        border.width: 1
        border.color: palette.mid
    }

    property alias watchExpressions: tableModel.watchExpressions
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.top: parent.top
        anchors.left: tableView.left
        anchors.right: parent.right
        syncView: tableView
        clip: true
        textRole: "display"
        delegate: Text {
            text: model.display
            horizontalAlignment: Text.AlignLeft
        }
    }
    focus: true
    Keys.onPressed: function (event) {
        // If there is not a child key handler (text editor), intercept delete+backspace
        // keystrokes and remove the row which currently has focus.
        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
            const selectedRows = tableView.selectionModel.selectedRows(0)
            for (const idx in selectedRows) {
                tableView.model.removeRows(selectedRows[idx].row, 1)
            }
            event.accepted = true
        }
        event.accepted = false
    }
    Menu {
        id: contextMenu
        property int row: -1
        popupType: Qt.platform.os !== "wasm" ? Popup.Native : Popup.Item
        MenuItem {
            text: "Delete Row"
            onTriggered: {
                tableModel.removeRows(contextMenu.row, 1)
                contextMenu.row = -1
            }
        }
    }

    TableView {
        id: tableView
        anchors.left: parent.left
        anchors.top: horizontalHeader.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 2
        boundsBehavior: Flickable.StopAtBounds
        resizableColumns: true
        model: WatchExpressionTableModel {
            id: tableModel
        }
        clip: true
        // do not focus:true, else key pressed trick will not work!
        selectionModel: ItemSelectionModel {}
        selectionBehavior: TableView.SelectCells
        selectionMode: TableView.ContiguousSelection
        delegate: Item {
            id: delegate
            implicitWidth: Math.max(8 * fm.averageCharacterWidth,
                                    textView.implicitWidth + 12)
            implicitHeight: Math.max(textView.implicitHeight * 1.3,
                                     fm.height * 1.7)
            required property bool editing
            required property bool selected
            required property bool current
            clip: true
            Rectangle {
                anchors.fill: parent
                color: selected ? palette.highlight : "transparent"
            }
            Text {
                id: textView
                anchors.fill: parent
                text: model.display ?? ""
                rightPadding: 10
                leftPadding: 2
                color: model.changed ? settings.extPalette.error.background : (selected ? palette.highlightedText : palette.windowText)
                visible: !editing
                font.family: settings.extPalette.baseMono.font.family
                font.pointSize: settings.extPalette.baseMono.font.pointSize
                font.italic: model.italicize
                verticalAlignment: Text.AlignVCenter
            }

            TableView.editDelegate: TextField {
                id: textEdit
                x: textView.x
                y: textView.y
                width: textView.width
                height: textView.height
                text: model.italicize ? "" : display
                TableView.onCommit: display = text
                font: settings.extPalette.baseMono.font
                verticalAlignment: Text.AlignVCenter
            }

            // Select current row on mouse press, and open editor on double click.
            // Focus + selection + editing + keyboard navigation is a pain to configure table view for.
            // So roll our own selection mangement.
            // TODO: figure out how to make TableView handle this crap itself.
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function (mouse) {
                    const index = tableView.model.index(row, column)
                    if (mouse.button === Qt.RightButton) {
                        contextMenu.row = row
                        contextMenu.popup(root, mouse.scenePosition)
                    } else {
                        const flags = ItemSelectionModel.ClearAndSelect
                                    | ItemSelectionModel.Rows | ItemSelectionModel.Current
                        // Close existing editors or we may higlight a row without editing it.
                        tableView.closeEditor()
                        tableView.selectionModel.select(index, flags)
                        tableView.selectionModel.setCurrentIndex(
                                    index, ItemSelectionModel.Current)
                        // Key events will not be processed if we don't move focus.
                        delegate.forceActiveFocus()
                    }
                }
                // Now that we've hijacked clicks, we need to open the editor manually.
                onDoubleClicked: {
                    const index = tableView.model.index(row, column)
                    if (tableView.model.flags(index) & Qt.ItemIsEditable) {
                        tableView.edit(index)
                    }
                }
            }
        }
    }
}
