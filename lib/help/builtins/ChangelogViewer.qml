import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Rectangle {
    id: wrapper
    property alias min: filterModel.min
    property alias max: filterModel.max

    property int paragraphSpace: 8
    property int tabSize: 5
    property int pointSize: 12

    color: palette.base
    border.width: 1
    border.color: palette.shadow
    ChangelogModel {
        id: baseModel
    }
    ChangelogFilterModel {
        id: filterModel
        sourceModel: baseModel
    }

    //  Used to calculate indent for bullet
    TextMetrics {
        id: tm
        text: "•  "
    }
    ColumnLayout {  //  page margin
        id: columnLayout
        anchors.fill: parent
        anchors.margins: wrapper.tabSize
        spacing: 0

        GridLayout {
            id: filterCombos
            columns: 3

            Layout.fillWidth: true

            Label {
                Layout.rowSpan: 2
                text: "Filter by version:"
                font.bold: true
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
        }   //  GridLayout

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: wrapper.tabSize

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
            contentWidth: list.width - ver.width
            bottomMargin: hor.height
            clip: true
            model: filterModel
            delegate: ColumnLayout {
                id: verDelegate
                spacing: 0
                property var version: model.display
                function dateStr() {
                    if (version.hasDate) {
                        return version.date.toISOString().substring(0, 10)
                    } else
                        return "Unreleased"
                }
                property string link: `https://github.com/Matthew-McRaven/Pepp/releases/v${version.version}`
                //  Version and date
                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: wrapper.paragraphSpace

                    text: `<a href="${verDelegate.link}">${version.version}</a> -- ${verDelegate.dateStr()}`
                    font.bold: true
                    font.pixelSize: wrapper.pointSize
                    onLinkActivated: Qt.openUrlExternally(verDelegate.link)
                    color: palette.windowText
                }
                //  Summary of changes
                Text {
                    Layout.minimumWidth: list.width - ver.width
                    Layout.maximumWidth: Layout.minimumWidth

                    text: version.blurb
                    visible: version.blurb.length > 0
                    wrapMode: Text.Wrap
                    color: palette.windowText
                }

                //  Detailed list of change
                Repeater {  //  Summary
                    model: version.sections
                    delegate: ColumnLayout {
                        id: secDelegate
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        required property var modelData

                        //  Contains update type: Fixed, Changed, Added
                        Text {
                            Layout.fillWidth: true
                            Layout.topMargin: wrapper.tabSize

                            text: modelData.title
                            font.bold: true
                            color: palette.windowText
                        }
                        //  Details of changes by update type
                        Repeater {
                            model: modelData.changes
                            delegate: Text {
                                id: changeDelegate
                                Layout.leftMargin: wrapper.tabSize * 4
                                Layout.minimumWidth: list.width - ver.width - Layout.leftMargin
                                Layout.maximumWidth: Layout.minimumWidth

                                required property var modelData

                                property string link: modelData.ghRef === 0 ? "" : `https://github.com/Matthew-McRaven/Pepp/issues/${modelData.ghRef}`
                                property string linkTail: modelData.ghRef === 0 ? "" : `(<a href="${changeDelegate.link}">#${modelData.ghRef}</a>)`
                                text: `<p style="text-indent:-${Math.floor(
                                          tm.width / 2 + wrapper.tabSize)}px;">${tm.text}${modelData.body} ${linkTail}</p>`
                                textFormat: Text.RichText
                                wrapMode: Text.Wrap
                                color: palette.windowText
                                onLinkActivated: {
                                    if (changeDelegate.link.length > 0)
                                        Qt.openUrlExternally(changeDelegate.link)
                                }
                            }   //  delegate: Text
                        }   //  Repeater - details
                    }   //  Delegate - ColumnLayout
                }   //  Repeater - Summary
            }   //  delegate: ColumnLayout
        }   //  ListView
    }   //  ColumnLayout - page margin
}
