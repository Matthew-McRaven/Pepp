import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import edu.pepp 1.0

Item {
    id: root
    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height
    NuAppSettings {
        id: settings
    }
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        syncView: tableView
        clip: true
        textRole: "display"

        // choose a fixed header height (or compute it)
        height: tm.width + 10
        delegate: Item {
            id: cell
            implicitWidth: tm_width.width
            implicitHeight: tm.width

            clip: true

            Text {
                id: label
                text: model.display
                color: palette.text
                font: tm.font
                wrapMode: Text.NoWrap   // wrap + rotation usually looks bad; use elide instead
                elide: Text.ElideRight

                // Rotate around its top-left and then position it
                transform: Rotation {
                    angle: -90
                    origin.x: 0
                    origin.y: 0
                }
                // Baseline changes w/rotation
                x: 0
                y: cell.height
            }
        }
    }
    TextMetrics {
        id: tm
        text: "MemWrite "
        font: settings.extPalette.base.font
    }
    TextMetrics {
        id: tm_width
        text: "30  "
        font: settings.extPalette.base.font
    }
    TableView {
        id: tableView
        clip: true
        alternatingRows: true
        anchors {
            top: horizontalHeader.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        model: MicroObjectModel {}
        delegate: Text {
            text: model.display
            color: palette.text
            clip: true
            Rectangle {
                anchors.fill: parent
                color: model.row % 2 !== 0 ? palette.window : palette.base
                // Will overwrite text if not below z==0
                z: -1
            }
        }
        columnWidthProvider: function (column) {
            return tm_width.width;
        }
    }
}
