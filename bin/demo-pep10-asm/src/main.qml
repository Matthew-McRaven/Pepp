import QtQuick
import QtQuick.Controls
// import edu.pepperdine.cslab.macroparse
Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    Text {
        id: name
        text: qsTr("I compile. Huzzah!")
    }

}
