import QtQuick
import QtQuick.Controls
import QtQml.Models
import edu.pepp

Item {
    id: root
    property alias model: reshapeModel.sourceModel
    NuAppSettings {
        id: settings
    }
    TextMetrics {
        id: tm
        font: settings.extPalette.baseMono.font
        text: "W" // Dummy value to get width of widest character
    }
    Rectangle {
        id: outline
        color: palette.base
        anchors.fill: parent
        //  Give object code viewer a background box
        border.width: 1
        border.color: palette.mid
    }
    HorizontalHeaderView {
        id: horizontalHeader
        // Silence warning about non-existent role.
        textRole: ""
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: outline.border.width
        }
        syncView: wrapper
        clip: true
        delegate: Item {
            id: headerDelegate
            implicitHeight: symbolHead.contentHeight
            Label {
                id: symbolHead
                anchors.left: parent.left
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                leftPadding: tm.width * 2

                color: palette.text
                text: "Test"
                font: tm.font
            }
            Label {
                id: valueHead
                focus: false
                anchors.left: symbolHead.right
                anchors.right: parent.right
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                rightPadding: tm.width * 2

                color: palette.text
                text: "Value"
                font: tm.font
            }
        }
        Rectangle {
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                rightMargin: vsc.width
            }
            color: palette.button
            z: -1
        }
    }
    TableView {
        id: wrapper
        anchors {
            top: horizontalHeader.bottom
            topMargin: rowHeightProvider(0) * 0.15
            bottom: parent.bottom
            right: parent.right
            rightMargin: vsc.width
            left: parent.left
        }
        model: PostReshapeModel {
            id: reshapeModel
        }
        // Increase inter-column padding for legibility
        columnSpacing: tm.width * 2
        contentWidth: width
        clip: true
        focus: true
        columnWidthProvider: function (index) {
            const header = "Symbol  Value".length + 4; // Need 2 padding  on each side
            const row = model.longest + 4 + 2; // Symbol + space + hex value
            return tm.width * Math.max(header, row) + 10;
        }
        rowHeightProvider: function (index) {
            return tm.font.pixelSize + 4;
        }
        onWidthChanged: {
            const actualSize = columnWidthProvider(0) + columnSpacing;
            reshapeModel.setColumnCount(width / actualSize);
        }
        onModelChanged: {
            const actualSize = columnWidthProvider(0) + columnSpacing;
            reshapeModel.setColumnCount(width / actualSize);
        }

        boundsBehavior: Flickable.StopAtBounds

        //  Disable horizontal scroll bar
        ScrollBar.horizontal: ScrollBar {
            policy: ScrollBar.AlwaysOff
        }

        //  Enable vertical scroll bar-always on
        ScrollBar.vertical: ScrollBar {
            id: vsc
            policy: ScrollBar.AlwaysOn
        }

        delegate: Item {
            id: delegate
            required property string test
            required property bool value
            // There exist extra cells in the 2D grid which do not map to indices in the underlying model.
            // The delegate does not get assigned new values, and then uses the cached text values, which are wrong.
            // These past-the-end items should not be selectable either.
            required property bool valid
            implicitHeight: post.contentHeight
            width: parent.width


            focus: false
            Label {
                id: postLabel
                focus: false
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                leftPadding: tm.width * 2

                color: palette.text
                text: valid ? delegate.test : ""
                font: tm.font
            }
            Label {
                focus: false
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: postLabel.right
                anchors.right: parent.right
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                rightPadding: tm.width * 2

                color: palette.text
                text: valid ? delegate.value : ""
                font: tm.font
            }
        }
    }
}
