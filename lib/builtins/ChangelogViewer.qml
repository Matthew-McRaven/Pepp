import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp

Rectangle {
    property alias min: filterModel.min
    property alias max: filterModel.max
    Component.onCompleted: {

    }

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
            onCurrentTextChanged: {
                filterModel.min = Qt.binding(() => currentText)
            }
            model: ChangelogFilterModel {
                sourceModel: baseModel
                max: maxVer.currentText
                property int oldRowCount: baseModel.rowCount()
                onMaxChanged: function () {
                    // rowCount() is updated before we enter this handler
                    // When we leave this handler, if currentIndex>rowCount, then currentIndex will be set to end.
                    // Translate our current index into an offset from the old end of the array to maintain a stable ordering
                    const oldDistanceFromEnd = oldRowCount - minVer.currentIndex
                    const adjusted = rowCount() - oldDistanceFromEnd
                    minVer.currentIndex = Qt.binding(() => adjusted)
                    oldRowCount = rowCount()
                }
            }
            Component.onCompleted: {
                const initialMin = find(filterModel.min)
                // console.log(`Min is ${filterModel.min}, index ${initialMin}`)
                if (initialMin !== -1)
                    currentIndex = Qt.binding(() => initialMin)
                else
                    currentIndex = Qt.binding(() => baseModel.rowCount() - 1)
            }
        }
        ComboBox {
            id: maxVer
            textRole: 'version_str'
            onCurrentTextChanged: {
                filterModel.max = Qt.binding(() => currentText)
            }
            model: ChangelogFilterModel {
                id: maxVerModel
                sourceModel: baseModel
                min: minVer.currentText
            }
            Component.onCompleted: {
                const initialMax = find(filterModel.max)
                // console.log(`Max is ${filterModel.max}, index ${initialMax}`)
                if (initialMax !== -1)
                    currentIndex = Qt.binding(() => initialMax)
                else
                    currentIndex = Qt.binding(() => 0)
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
