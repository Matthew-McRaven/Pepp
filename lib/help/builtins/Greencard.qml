import QtQuick
import QtQuick.Controls
import "qrc:/qt/qml/edu/pepp/utils" as Utils
import edu.pepp 1.0

Item {
    id: root
    property int architecture: Architecture.PEP10
    Utils.GreencardView {
        anchors.fill: parent
        architecture: root.architecture ?? Architecture.PEP10
    }
}
