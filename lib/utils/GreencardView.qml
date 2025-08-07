import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp

Item {
    id: root
    required property int architecture
    property bool hideStatus: false
    property bool hideMnemonic: false
    property bool dyadicAddressing: false
    NuAppSettings {
        id: settings
    }
    HorizontalHeaderView {
        id: horizontalHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        syncView: tableView
        clip: true
        textRole: "display"
        delegate: Label {
            text: model.display
            wrapMode: Text.WordWrap
            font.bold: true
            topPadding: 2
            bottomPadding: 2
            leftPadding: 7.5
            rightPadding: 7.5
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            background: Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: palette.mid
                border.width: 1
                // Must set all 4 anchors manually. As soon as you override one of left/right or top/bottom margin, the other becomes unset.
                // Since we need special handling for first row and column, we must override all margins.
                anchors.leftMargin: model.column === 0 ? 0 : -border.width / 2
                anchors.rightMargin: -border.width / 2
                anchors.topMargin: model.row === 0 ? 0 : -border.width / 2
                anchors.bottomMargin: -border.width / 2
                z: 1
            }
        }
    }
    TextMetrics {
        id: tm
        text: "Instruction"
        font: settings.extPalette.base.font
    }
    FontMetrics {
        id: fm
        font: settings.extPalette.baseMono.font
    }

    TableView {
        id: tableView
        clip: true
        anchors.top: horizontalHeader.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        boundsBehavior: Flickable.StopAtBounds
        model: GreencardFilterModel {
            hideStatus: root.hideStatus
            hideMnemonic: root.hideMnemonic
            dyadicAddressing: root.dyadicAddressing
            sourceModel: GreencardModel {
                id: innerModel
            }
        }
        columnWidthProvider: function (column) {
            if (column === 0) {
                // Pick the largest of the size of "Instruction" OR 10 monospaced characters
                return Math.max(tm.width, 10 * fm.averageCharacterWidth) + 10;
            }
            // Auto-compute other columns
            return -1;
        }

        delegate: Label {
            // Prevent 0-width columns, which causes many errors to be emitted to the console.
            text: model.display ? model.display : " "
            font: model.useMonoRole ? settings.extPalette.baseMono.font : settings.extPalette.base.font
            textFormat: model.useMarkdown ? Text.MarkdownText : Text.PlainText
            topPadding: 2
            bottomPadding: 2
            leftPadding: 7.5
            rightPadding: 7.5
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            background: Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: palette.mid
                border.width: 1
                // Must set all 4 anchors manually. As soon as you override one of left/right or top/bottom margin, the other becomes unset.
                // Since we need special handling for first row and column, we must override all margins.
                anchors.leftMargin: model.column === 0 ? 0 : -border.width / 2
                anchors.rightMargin: -border.width / 2
                anchors.topMargin: model.row === 0 ? 0 : -border.width / 2
                anchors.bottomMargin: -border.width / 2
                z: 1
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: tableView.contentHeight + horizontalHeader.height > root.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        }
        ScrollBar.horizontal: ScrollBar {
            policy: tableView.contentWidth > root.width ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        }
    }
    onArchitectureChanged: {
        if (root.architecture === Architecture.PEP10)
            innerModel.make_pep10();
        else if (root.architecture === Architecture.PEP9)
            innerModel.make_pep9();
    }
}
