import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp

Item {
    id: root
    required property int architecture
    property bool hideStatus: false
    property bool hideMnemonic: false
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
            sourceModel: GreencardModel {
                id: innerModel
            }
        }
        columnSpacing: 5
        delegate: Text {
            // Prevent 0-width columns, which causes many errors to be emitted to the console.
            text: model.display ? model.display : " "
            font: settings.extPalette.baseMono.font
            Layout.fillWidth: column == 0
        }
    }
    onArchitectureChanged: {
        if (root.architecture === Architecture.PEP10)
            innerModel.make_pep10();
        else if (root.architecture === Architecture.PEP9)
            innerModel.make_pep9();
    }
}
