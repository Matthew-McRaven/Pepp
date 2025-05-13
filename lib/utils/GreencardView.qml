import QtQuick
import edu.pepp

TableView {
    id: root
    NuAppSettings {
        id: settings
    }
    required property int architecture
    property bool hideStatus: false
    property bool hideMnemonic: false
    model: GreencardFilterModel {
        hideStatus: root.hideStatus
        hideMnemonic: root.hideMnemonic
        sourceModel: GreencardModel {
            id: innerModel
        }
    }
    columnSpacing: 5
    onArchitectureChanged: {
        if (root.architecture === Architecture.PEP10)
            innerModel.make_pep10();
        else if (root.architecture === Architecture.PEP9)
            innerModel.make_pep9();
    }
    delegate: Text {
        text: model.display
        font: settings.extPalette.baseMono.font
    }
}
