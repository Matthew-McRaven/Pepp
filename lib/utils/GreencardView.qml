import QtQuick
import edu.pepp

TableView {
    id: root
    NuAppSettings {
        id: settings
    }
    required property int architecture
    model: GreencardModel {}
    columnSpacing: 5
    onArchitectureChanged: {
        if (root.architecture === Architecture.PEP10)
            model.make_pep10();
        else if (root.architecture === Architecture.PEP9)
            model.make_pep9();
    }
    delegate: Text {
        text: model.display
        font: settings.extPalette.baseMono.font
    }
}
