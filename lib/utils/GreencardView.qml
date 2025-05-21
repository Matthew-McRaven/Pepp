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
        delegate: Text {
            text: model.display
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.bold: true
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
        columnSpacing: 5
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
                // Pick the largest of 9 monospaced characters OR the size of "Instruction"
                return Math.max(tm.width, 9 * fm.averageCharacterWidth) + 10;
            }
            // Auto-compute other columns
            return -1;
        }

        delegate: Text {
            // Prevent 0-width columns, which causes many errors to be emitted to the console.
            text: model.display ? model.display : " "
            font: model.useMonoRole ? settings.extPalette.baseMono.font : settings.extPalette.base.font
            Layout.fillWidth: column == 0
            textFormat: model.useMarkdown ? Text.MarkdownText : Text.PlainText
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
