import QtQuick
import edu.pepp

Rectangle {
    ChangelogModel {
        id: model
    }
    ListView {
        id: list
        anchors.fill: parent
        model: model
        delegate: Column {
            id: verDelegate
            property var version: model.display
            Text {
                text: `<h1>${version.version}</h1>`
            }
            Repeater {
                model: version.sections
                delegate: Column {
                    id: secDelegate
                    required property var modelData
                    Text {
                        text: `    <b>${modelData.title}</b>`
                    }
                    Repeater {
                        model: modelData.changes
                        delegate: Text {
                            id: changeDelegate
                            required property var modelData
                            text: `        - ${modelData.body}`
                        }
                    }
                }
            }
        }
    }

    color: palette.window
}
