import QtQuick
import edu.pepp

Rectangle {
    ChangelogModel {
        id: model
    }
    TextMetrics {
        id: tm
        text: "      "
    }
    ListView {
        id: list
        anchors.fill: parent
        model: model
        delegate: Column {
            id: verDelegate
            property var version: model.display
            function dateStr() {
                if (version.hasDate) {
                    return version.date.toISOString().substring(0, 10)
                } else
                    return "Unreleased"
            }
            property string link: `https://github.com/Matthew-McRaven/Pepp/releases/v${version.version}`
            Text {
                text: `<h1><a href="${verDelegate.link}">${version.version}</a> -- ${verDelegate.dateStr(
                          )}</h1>`
                onLinkActivated: Qt.openUrlExternally(verDelegate.link)
                color: palette.windowText
            }
            Text {
                text: version.blurb.length > 0 ? version.blurb + "<br><br>" : ""
                wrapMode: Text.Wrap
                color: palette.windowText
            }

            Repeater {
                model: version.sections
                delegate: Column {
                    id: secDelegate
                    required property var modelData
                    Text {
                        text: `    <b>${modelData.title}</b>`
                        color: palette.windowText
                    }
                    Repeater {
                        model: modelData.changes
                        delegate: Text {
                            id: changeDelegate

                            required property var modelData
                            //font.weight: modelData.priority === 2 ? Font.Bold : Font.Normal
                            //font.underline: modelData.priority === 2
                            property string link: modelData.ghRef === 0 ? "" : `https://github.com/Matthew-McRaven/Pepp/issues/${modelData.ghRef}`
                            property string linkTail: modelData.ghRef === 0 ? "" : `(<a href="${changeDelegate.link}">#${modelData.ghRef}</a>)`
                            text: `<p style="text-indent:${Math.floor(
                                      tm.width)}px;"> - ${modelData.body} ${linkTail}</p>`
                            textFormat: Text.RichText
                            color: palette.windowText
                            onLinkActivated: {
                                if (changeDelegate.link.length > 0)
                                    Qt.openUrlExternally(changeDelegate.link)
                            }
                        }
                    }
                }
            }
        }
    }

    color: palette.window
}
