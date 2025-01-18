import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    id: wrapper
    property alias min: filterModel.min
    property alias max: filterModel.max
    ChangelogModel {
        id: baseModel
    }
    ChangelogFilterModel {
        id: filterModel
        sourceModel: baseModel
    }
    TextMetrics {
        id: tm
        text: "      "
    }
    GridLayout {
        id: filterCombos
        columns: 3
        anchors {
            top: parent.top
            left: parent.left
        }
        Label {
            Layout.rowSpan: 2
            text: "<b>Filter by version:<b>"
            Layout.alignment: Qt.AlignTop
            rightPadding: 8
        }

        Label {
            text: "From..."
        }
        Label {
            text: "To..."
        }

        ComboBox {
            id: minVer
            textRole: 'version_str'
            model: ChangelogFilterModel {
                sourceModel: baseModel
                max: filterModel.max
                onMaxChanged: minVer.update()
            }
            // Activated only occurs with user interaction and not programtic manipulation of current index.
            // This prevents spurious updates to the model.
            onActivated: filterModel.min = currentText
            Component.onCompleted: update()
            function update() {
                const index = find(filterModel.min)
                currentIndex = Qt.binding(() => index)
            }
        }
        ComboBox {
            id: maxVer
            textRole: 'version_str'
            model: ChangelogFilterModel {
                sourceModel: baseModel
                min: filterModel.min
                onMinChanged: maxVer.update()
            }
            // Activated only occurs with user interaction and not programtic manipulation of current index.
            // This prevents spurious updates to the model.
            onActivated: filterModel.max = currentText
            Component.onCompleted: update()
            function update() {
                const index = find(filterModel.max)
                currentIndex = Qt.binding(() => index)
            }
        }
    }

    ListView {
        id: list
        anchors {
            top: filterCombos.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        ScrollBar.vertical: ScrollBar {
            id: ver
            policy: list.contentHeight > list.height ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        }
        ScrollBar.horizontal: ScrollBar {
            id: hor
            policy: list.contentWidth > list.width ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        }
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: contentItem.childrenRect.width
        bottomMargin: hor.height
        leftMargin: ver.width
        clip: true
        model: filterModel
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
