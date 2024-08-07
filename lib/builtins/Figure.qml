
/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0
import "qrc:/ui/text/editor" as TextEdit

Item {
    id: wrapper
    property alias text_area: figContent
    signal addProject(string feats, string text, string switchToMode, var optionalOS, var tests)

    //required property var model
    ColumnLayout {
        id: figCol
        property string copyTitle: "5.7"
        property string copyContent: "This is some very long text used to test wrapping inside text control."
        property var payload
        property string listing: "Pep/10 is a virtual machine for writing machine language and assemply language programs"
        property bool copyToSource: true
        property string edition
        property string language

        signal typeChange(string type)

        //  Set page contents based on parent selected values
        Component.onCompleted: {
            copyTitle = treeView.selectedFig.display
            payload = treeView.selectedFig.payload
            edition = treeView.selectedFig.edition

            copyToSource = Qt.binding(() => (payload.chapterName !== "04"))

            Object.keys(payload.elements).map(lang => {
                                                  languageModel.append({
                                                                           "key": lang,
                                                                           "value": payload.elements[lang].content
                                                                       })
                                              })

            //  Show first item in list box.
            let lang = Object.keys(payload.elements)[0]
            figCol.listing = payload.elements[lang].content
            figType.currentIndex = Qt.binding(() => 0)
            figCol.edition = Qt.binding(() => edition)
            figCol.language = Qt.binding(() => lang)
        }

        spacing: 10
        anchors {
            topMargin: 0
            leftMargin: 20
            rightMargin: 20
            bottomMargin: 20
            fill: parent
        }

        ComboBox {
            id: figType
            //  Control layout
            Layout.alignment: Qt.AlignTop
            Layout.preferredHeight: 30
            Layout.preferredWidth: parent.width
            Layout.fillWidth: true

            //  Mode setup
            textRole: "key"
            valueRole: "value"
            model: ListModel {
                id: languageModel
            }

            //  Update code listing
            onActivated: figCol.listing = currentValue

            //  Use similar size as topic title
            font.pointSize: 22

            delegate: ItemDelegate {
                width: figType.width
                contentItem: Text {
                    text: figType.textRole ? (Array.isArray(
                                                  figType.model) ? modelData[figType.textRole] : model[figType.textRole]) : modelData
                    //  Text color in dropdown
                    color: figType.highlightedIndex === index ? palette.window : palette.dark
                    font.pointSize: 12
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
                highlighted: figType.highlightedIndex === index
            }

            indicator: Canvas {
                id: canvas
                x: figType.width - width - figType.rightPadding
                y: figType.topPadding + (figType.availableHeight - height) / 2
                width: 12
                height: 8
                contextType: "2d"

                Connections {
                    target: figType

                    function onPressedChanged() {
                        canvas.requestPaint()
                    }
                }

                onPaint: {
                    context.reset()
                    context.moveTo(0, 0)
                    context.lineTo(width, 0)
                    context.lineTo(width / 2, height)
                    context.closePath()
                    context.fillStyle = figType.pressed ? Qt.black : "#ff7d33"
                    context.fill()
                }
            }
            contentItem: Text {
                leftPadding: 0
                rightPadding: figType.indicator.width + figType.spacing

                text: figType.displayText
                font: figType.font
                color: palette.window
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            //  Background in control (not dropped down)
            background: Rectangle {
                color: "#ff7d33"
                radius: 5
            }
        }
        TextEdit.AsmTextEdit {
            id: figContent
            Layout.alignment: Qt.AlignCenter
            Layout.fillHeight: true
            Layout.fillWidth: true
            text: figCol.listing
            edition: figCol.edition
            language: figCol.language
            isReadOnly: true
            allowsBP: false
        }
        Row {
            id: copyRow

            Layout.alignment: Qt.AlignBottom
            Layout.preferredHeight: 20
            Layout.maximumHeight: 30
            Layout.fillWidth: true

            spacing: 10

            //  Copy button logic
            Button {
                id: button
                text: "Copy to New Project"
                anchors {
                    horizontalCenter: copyRow.center
                }
                onClicked: wrapper.addProject(
                               "", figCol.listing, "Edit",
                               figCol?.payload?.defaultOS?.elements[figCol.language]?.content,
                               figCol?.payload?.tests)
            }

            //  Figure title
            Text {
                // figCol.copyTitle & figCol.copyContent
                width: copyRow.width - button.width - copyRow.spacing
                textFormat: Text.RichText
                text: "<div><b>" + figCol.copyTitle + ":</b> " + figCol.copyContent + "</div>"
                wrapMode: Text.WordWrap
            }
        } //  Row
    } //  Column
} //  Item
