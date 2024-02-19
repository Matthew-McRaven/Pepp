/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
import QtQuick.Window
import QtQuick.Controls

import edu.pepp 1.0

Item {
    anchors.fill: parent
    Component.onCompleted: {
        treeView.onSelectedFigChanged.connect(
                (arg) => {
                if (treeView.selectedFig === undefined) {
                    topicWindow.source = "Topic.qml"
                } else {
                    topicWindow.source = ""
                    topicWindow.source = "Figure.qml"

                    //figCol.help( drawer.selected.display,drawer.selected.payload );
                }
            }
        )
    }


    Rectangle {
        id: container
        anchors.fill: parent

        TreeView {
            property var selectedFig: undefined
            model: BookModel {
            }
            id: treeView
            // Make sure the drawer is always at least as wide as the text
            // There was an issue in WASM where the titles clipper the center area
            TextMetrics {
                id: textMetrics
                text: "Computer Systems, 200th edition"
            }
            width: textMetrics.width
            anchors.left: parent.left
            anchors.top: parent.top; anchors.bottom: parent.bottom


            delegate
                :
                Item {
                    id: treeDelegate
                    implicitWidth: padding + label.x + label.implicitWidth + padding
                    implicitHeight: textMetrics.height*1.1
                    readonly property real indent: 10
                    readonly property real padding: 5
                    // Assigned to by TreeView:
                    required property TreeView treeView
                    required property bool isTreeNode
                    required property bool expanded
                    required property int hasChildren
                    required property int depth
                    required property var payload
                    required property var kind
                    required property var display
                    required property var edition

                    TapHandler {
                        onTapped: {
                            if (treeDelegate.hasChildren) {
                                treeView.selectedFig = undefined
                                treeView.toggleExpanded(row)
                            } else {
                                treeView.selectedFig = {
                                    //kind,payload
                                    display, payload, edition
                                }
                            }
                        }
                    }

                    Text {
                        id: indicator
                        visible: treeDelegate.isTreeNode && treeDelegate.hasChildren
                        x: padding + (treeDelegate.depth * treeDelegate.indent)
                        anchors.verticalCenter: label.verticalCenter
                        text: "▸"
                        rotation: treeDelegate.expanded ? 90 : 0
                    }

                    Text {
                        id: label
                        x: padding + (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)
                        width: treeDelegate.width - treeDelegate.padding - x
                        clip: true
                        text: model.display
                    }
                }

            ScrollIndicator.vertical: ScrollIndicator {
            }
        }
        Flickable {
            id: flickable
            anchors.fill: parent
            anchors.topMargin: 0
            anchors.leftMargin: treeView.width

            topMargin: 10
            bottomMargin: 10

            //  Moved logic to separate qml control files
            Loader {
                id: topicWindow
                anchors.fill: parent
                source: "Topic.qml"
            }

            ScrollIndicator.vertical: ScrollIndicator {
            }
        } //  Flickable
    } //Loader
}
