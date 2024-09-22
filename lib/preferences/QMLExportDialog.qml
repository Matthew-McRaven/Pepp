import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    WASMIO {
        id: io
    }
    function open() {
        io.save(Theme.themePath(), Theme.jsonString())
    }
}
