import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    ListView {
        id: listView
        anchors.fill: parent
        model: SelfTestModel {}
        delegate: Text {
            text: model.display
            color: palette.text
        }
    }
}
