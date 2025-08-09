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
import "qrc:/qt/qml/edu/pepp/text/editor" as Editor

Item {
    id: wrapper
    required property string title
    required property var payload
    required property string lexerLang
    property int architecture: Architecture.PEP10 // Silence QML warning about non-existent property
    property string curFragName: undefined
    property var curFragment: undefined
    NuAppSettings {
        id: settings
    }

    Component.onCompleted: {
        const frags = payload.fragments;
        const names = Object.keys(frags);
        var defaultFragmentIndex = 0;
        Object.keys(frags).map(name => {
            if (frags[name].isHidden)
                return;
            fragmentModel.append({
                "key": name,
                "value": frags[name].content
            });
            if (frags[name].isDefault)
                defaultFragmentIndex = fragmentModel.count - 1;
        });
        wrapper.curFragName = Qt.binding(() => Object.keys(frags)[defaultFragmentIndex]);
        wrapper.curFragment = Qt.binding(() => payload.fragments[wrapper.curFragName]);
        fragmentSelector.currentIndex = Qt.binding(() => defaultFragmentIndex);
        fragmentSelector.activated(defaultFragmentIndex);
    }

    ColumnLayout {
        id: figureLayout
        spacing: 0
        anchors.fill: parent
        RowLayout {
            Layout.alignment: Qt.AlignTop
            Layout.preferredHeight: Math.max(35, fragmentSelectorFM.height)
            Layout.minimumHeight: Layout.preferredHeight
            Layout.maximumHeight: Layout.preferredHeight
            Layout.preferredWidth: parent.width
            Layout.fillWidth: true
            spacing: 0
            ComboBox {
                id: fragmentSelector
                Layout.fillHeight: true
                textRole: "key"
                valueRole: "value"
                font: fragmentSelectorFM.font
                FontMetrics {
                    id: fragmentSelectorFM
                    font.pointSize: 25
                }

                model: ListModel {
                    id: fragmentModel
                }
                delegate: ItemDelegate {
                    id: delegate
                    required property var model
                    required property int index
                    width: fragmentSelector.width
                    contentItem: Text {
                        text: delegate.model[fragmentSelector.textRole]
                        //  Text color in dropdown
                        color: fragmentSelector.highlightedIndex === index ? palette.window : palette.dark
                        font.pointSize: 12
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    highlighted: fragmentSelector.highlightedIndex === index
                }
                indicator: Canvas {
                    id: canvas
                    x: fragmentSelector.width - width - fragmentSelector.rightPadding
                    y: fragmentSelector.topPadding + (fragmentSelector.availableHeight - height) / 2
                    width: 12
                    height: 8
                    contextType: "2d"

                    Connections {
                        target: fragmentSelector
                        function onPressedChanged() {
                            canvas.requestPaint();
                        }
                    }

                    onPaint: {
                        context.reset();
                        context.moveTo(0, 0);
                        context.lineTo(width, 0);
                        context.lineTo(width / 2, height);
                        context.closePath();
                        context.fillStyle = fragmentSelector.pressed ? Qt.black : "#ff7d33";
                        context.fill();
                    }
                }
                contentItem: Text {
                    leftPadding: 0
                    rightPadding: fragmentSelector.indicator.width + fragmentSelector.spacing
                    text: fragmentSelector.displayText
                    font: fragmentSelector.font
                    color: palette.text
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                onActivated: textArea.updateText(currentValue)
            }   //  ComboBox
            Rectangle {
                color: palette.window
                //  Hide all but top line
                Layout.leftMargin: -1
                Layout.rightMargin: -1
                Layout.topMargin: -1
                Layout.fillWidth: true
                Layout.fillHeight: true
                border {
                    width: 1
                    color: palette.shadow
                }
                z: -1

            }
        }   //  RowLayout
        Editor.ScintillaAsmEdit {
            id: textArea
            Layout.alignment: Qt.AlignCenter
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.rightMargin: 1
            editorFont: editorFM.font
            language: wrapper.lexerLang
            readOnly: false
            Component.onCompleted: {
                readOnly = true;
                textArea.caretBlink = 0;
            }
            function updateText(newText) {
                textArea.readOnly = false;
                textArea.text = newText;
                textArea.readOnly = true;
            }
        }
        Rectangle {
            Layout.preferredHeight: button.height
            Layout.minimumHeight: Layout.preferredHeight
            Layout.maximumHeight: Math.max(button.height, helpText.contentHeight)
            Layout.fillWidth: true
            color: palette.window
            border {
                width: 1
                color: palette.shadow
            }

             //  Copy button logic
            RowLayout {
                spacing: 0
                anchors.left: parent.left
                anchors.right: parent.right
                Button {
                    id: button
                    visible: enabled
                    enabled: wrapper.payload.defaultFragmentName.length > 0
                    text: "Copy to New Project"
                    padding: 5

                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillHeight: true
                    Layout.minimumWidth: 50//button.width

                    onClicked: {
                        const pl = wrapper.payload;
                        const name = pl.defaultFragmentName;
                        const text = pl.fragments[name].content;
                        wrapper.addProject("", text, "Editor", pl?.defaultOS?.fragments["pep"]?.content, pl?.tests);
                        wrapper.renameCurrentProject(wrapper.title);
                    }
                }   //  Button

                //  Figure title
                Text {
                    id: helpText

                    Layout.leftMargin: 10
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    textFormat: Text.RichText
                    text: "<b>" + wrapper.title + ":</b> " + wrapper.payload.description
                    color: palette.windowText
                    wrapMode: Text.WordWrap
                }   //  Text - figure title
            }   //  RowLayout
        }   //  Rectangle - figure backgound
    }
    FontMetrics {
        id: editorFM
        font: settings.extPalette.baseMono.font
    }
    signal addProject(string feats, string text, string switchToMode, var optionalOS, var tests)
    signal renameCurrentProject(string newName)
}
