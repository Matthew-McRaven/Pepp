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

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Pep/10 Help")

    //! [orientation]
    //  Used to make figure treeview appear and disappear based on orientation.
    //  Primarily used in devices like an iPad which can be flipped.
    readonly property bool inPortrait: window.width < window.height
    //! [orientation]

    Component.onCompleted: {
        //drawer.onSelectedChanged.connect(
        //      (arg) => {console.log (drawer.selected.display)}
        //      )
        drawer.onSelectedChanged.connect(
                (arg) => {
                if (drawer.selected === undefined) {
                    mainWindow.source = "Topic.qml"
                } else {
                    mainWindow.source = ""
                    mainWindow.source = "Figure.qml"

                    //figCol.help( drawer.selected.display,drawer.selected.payload );
                }
            }
        )
    }

    signal switchedToFigure(TextEdit area)

    Rectangle {
        id: mainLoader
        anchors.fill: parent
        //function hi(){console.log("Hi")}
        //  Left pane with selections
        Drawer {
            id: drawer

            y: 0
            width: textMetrics.width
            // Make sure the drawer is always at least as wide as the text
            // There was an issue in WASM where the titles clipper the center area
            TextMetrics {
                id: textMetrics
                text: "Computer Systems, 200th edition"
            }
            height: window.height
            property var selected: undefined

            modal: inPortrait
            interactive: inPortrait
            position: inPortrait ? 0 : 1
            visible: !inPortrait
            background: Rectangle {
                color: "#ffffff"
            }

            // Derived from: https://doc.qt.io/qt-6/qml-qtquick-treeview.html
            TreeView {
                id: treeView
                anchors.fill: parent
                model: global_model

                delegate: Item {
                    id: treeDelegate
                    implicitWidth: padding + label.x + label.implicitWidth + padding
                    implicitHeight: label.implicitHeight * 1.5
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
                                drawer.selected = undefined
                                treeView.toggleExpanded(row)
                            } else {
                                drawer.selected = {
                                    //kind,payload
                                    display, payload, edition
                                }
                                //let d = drawer.selected
                                //console.log(Object.keys(drawer.selected))
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
                        text: {
                            if (model.kind === "figure") {
                                let x = model.payload.elements;
                                return "Figure " + model.display
                                //console.log(Object.keys(x));
                            } else if (model.kind === "book") {
                                let text = ""
                                for (let fig of model.payload) {
                                    //console.log(fig.isOS)
                                }
                            }
                            return model.display// + " ---- " + model.kind;
                        }
                    }
                }

                /*background: Rectangle {
                  anchors.fill: parent;
                  color: "blue"
                }*/


                ScrollIndicator.vertical: ScrollIndicator {
                }
            } //  TreeView
        } //  Drawer

        Flickable {
            id: flickable
            anchors.fill: parent
            anchors.topMargin: 0
            anchors.leftMargin: !inPortrait ? drawer.width : undefined

            topMargin: 10
            bottomMargin: 10

            //  Moved logic to separate qml control files
            Loader {
                id: mainWindow
                anchors.fill: parent
                source: "Topic.qml"
            }

            ScrollIndicator.vertical: ScrollIndicator {
            }
        } //  Flickable
    } //Loader
}
