/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

//  Removed versions since HoverHandler is not in earlier controls
import QtQuick
import QtQuick.Controls
import edu.pepp
import "qrc:/kddockwidgets/qtquick/views/qml/" as KDDW

KDDW.TabBarBase {
    id: root
    NuAppSettings {
        id: settings
    }

    // Helper, only applies if you're using the TabBar from QQControls.
    // Returns the internal ListView
    function getInternalListView(): Item {
        for (var i = 0; i < tabBar.children.length; ++i) {
            if (tabBar.children[i].toString().startsWith("QQuickListView"))
                return tabBar.children[i];
        }

        console.warn("Couldn't find the internal ListView");
        return null;
    }

    function getTabAtIndex(index) {
        var listView = getInternalListView();
        var content = listView.children[0];

        var curr = 0;
        for (var i = 0; i < content.children.length; ++i) {
            var candidate = content.children[i];
            if (typeof candidate.tabIndex == "undefined") {
                // All tabs need to have "tabIndex" property.
                continue;
            }

            if (curr == index)
                return candidate;

            curr++;
        }

        if (index < listView.children.length)
            return listView.children[0].children[index];

        return null;
    }

    function getTabIndexAtPosition(globalPoint) {
        var listView = getInternalListView();
        var content = listView.children[0];

        for (var i = 0; i < content.children.length; ++i) {
            var candidate = content.children[i];
            if (typeof candidate.tabIndex == "undefined") {
                // All tabs need to have "tabIndex" property.
                continue;
            }

            var localPt = candidate.mapFromGlobal(globalPoint.x, globalPoint.y);
            if (candidate.contains(localPt)) {
                return i;
            }
        }

        return tabBar.currentIndex;
    }

    implicitHeight: tabBar.implicitHeight

    onCurrentTabIndexChanged: {
        // A change coming from C++
        tabBar.currentIndex = root.currentTabIndex;
    }

    TabBar {
        id: tabBar

        width: parent.width
        position: (root.groupCpp && root.groupCpp.tabsAtBottom) ? TabBar.Footer : TabBar.Header
        hoverEnabled: false

        onCurrentIndexChanged: {
            // Tells the C++ backend that the current dock widget has changed
            root.currentTabIndex = this.currentIndex;
        }

        // If the currentIndex changes in the C++ backend then update it here
        Connections {
            target: root.groupCpp
            function onCurrentIndexChanged() {
                root.currentTabIndex = root.groupCpp.currentIndex;
                const w = tabBarCpp.dockWidgetObject(root.groupCpp.currentIndex);
                if (w && w.needsAttention) {
                    w.needsAttention = Qt.binding(() => false);
                }
            }
        }

        Repeater {
            /// The list of tabs is stored in a C++ model. This repeater populates our TabBar.
            model: root.groupCpp ? root.groupCpp.tabBar.dockWidgetModel : 0
            TabButton {
                id: btn
                required property int index
                required property string title
                readonly property int tabIndex: index
                property color error: settings.extPalette.error.background
                property real flashFactor: warningLogic.flashFactor
                property color flashColor: Qt.rgba(error.r, error.g, error.b, flashFactor)
                property var dockObj: tabBarCpp.dockWidgetObject(index)
                text: title
                hoverEnabled: true

                background: Item {
                    id: tbBackground

                    //  Control that flashes red when there are errors
                    Rectangle {
                        id: warningLogic
                        property real flashFactor: 0.0
                        anchors.fill: tbBackground
                        implicitHeight: 21
                        topLeftRadius: 5
                        topRightRadius: topLeftRadius
                        z: 1    //  Must appear on top for effect to work
                        visible: (dockObj && dockObj.needsAttention)
                        color: (dockObj && dockObj.needsAttention) ? Qt.tint(palette.button, flashColor) : "transparent"
                        opacity: .25
                        border.width: 1
                        border.color: "transparent"

                        SequentialAnimation on flashFactor {
                            id: flashAnim
                            loops: 10

                            NumberAnimation {
                                from: 1.0
                                to: 0.0
                                duration: 500
                                easing.type: Easing.InOutQuad
                            }
                            NumberAnimation {
                                from: 0.0
                                to: 1.0
                                duration: 500
                                easing.type: Easing.InOutQuad
                            }

                            onStopped: btn.flashFactor = 1.0
                        }
                    }
                    //  Standard background copied from Fusion control
                    Rectangle {
                        id: originalFusion
                        anchors.fill: tbBackground
                        implicitHeight: 21
                        topLeftRadius: 5
                        topRightRadius: topLeftRadius

                        //  Event is getting eaten by parent somewhere. Hover never triggers in TabButton or HoverHandler
                        HoverHandler {
                            id: mouse
                            blocking: true
                        }

                        border.width: 1
                        border.color: btn.hovered ? palette.highlight : Qt.darker(palette.window, 1.3)
                        gradient: Gradient {
                            GradientStop {
                                position: 0
                                color: btn.checked ? Qt.lighter(btn.palette.button, 1.04) : Qt.darker(btn.palette.button, 1.08)
                            }
                            GradientStop {
                                position: btn.checked ? 0 : 0.85
                                color: btn.checked ? Qt.lighter(btn.palette.button, 1.04) : Qt.darker(btn.palette.button, 1.08)
                            }
                            GradientStop {
                                position: 1
                                color: btn.checked ? btn.palette.button : Qt.darker(btn.palette.button, 1.16)
                            }
                        }
                    }
                }   //  background: Item
                function onNeedsAttentionChanged() {
                    if (dockObj?.needsAttention ?? false) {
                        if (dockObj.isCurrentTab())
                            dockObj.needsAttention = Qt.binding(() => false);
                        else if (dockObj.isOpen)
                            flashAnim.start();
                    } else if (btn && flashAnim)
                        flashAnim.stop();
                }
                Component.onCompleted: {
                    dockObj.needsAttentionChanged.connect(onNeedsAttentionChanged);
                }
                Component.onDestruction: {
                    if (dockObj)
                        dockObj.needsAttentionChanged.disconnect(onNeedsAttentionChanged);
                }
            }   //TabButton
        }   //  Repeater
    }   //  TabBar
}
