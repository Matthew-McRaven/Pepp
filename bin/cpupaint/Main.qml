import QtQuick

import CPUPaint
import QtQuick.Controls

Window {
    id: root
    width: 1024
    height: 720
    visible: true
    title: qsTr("Circuit Design")
    // A dummy left-panel
    Rectangle {
        id: lpanel
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: 150
        color: "orange"
    }
    component LabeledTriState: Item {
        id: triState
        required property var location
        required property string label
        required property int value
        required property int maxValue
        x: location.x
        y: location.y
        height: location.height
        width: childrenRect.width
        SpinBox {
            id: spin
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: 50
            height: parent.height
            wrap: true
            value: triState.value
            stepSize: 1
            from: -1
            to: triState.maxValue
            editable: true
            textFromValue: function (value, locale) {
                if (value == -1)
                    return "";
                return Number(value);
            }
        }
        Label {
            anchors {
                left: spin.right
                verticalCenter: spin.verticalCenter
                leftMargin: 5
            }
            text: parent.label
        }
    }
    component LabeledCheck: CheckBox {
        required property var location
        required property string label
        required property bool value

        // Magic constant to align right edges of of controls
        x: location.x + 35
        y: location.y
        // Do not bind width, else we may chop off the label.
        height: location.height
        text: label
        checked: value
    }
    /*
     * Both "viewport" and "flickable" must belong to the same parent, because they have complex z-ordering issues,
     * viewport should visually be behind the flickable, so that it does not clip the scrollbars.
     * viwport must logically be in front of the flickable, so nested mouse areas can receive focus.
     * So, this item exists to enforce these mutually impossible goals.
     *
     * On z==0, we manually created Scrollbars so that they stay on top of the viewport.
     * On z==-1, we have our various (interactive!) scene items.
     * On z==-2, we have the actual flickable mouse area.
     */
    Item {
        clip: true
        anchors {
            left: lpanel.right
            right: rpanel.left
            top: parent.top
            bottom: parent.bottom
        }
        ScrollBar {
            id: vbar
            hoverEnabled: true
            active: hovered || pressed
            orientation: Qt.Vertical
            position: flickable.contentY / flickable.contentHeight
            size:  flickable.height / flickable.contentHeight
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            policy: ScrollBar.AlwaysOn
        }

        ScrollBar {
            id: hbar
            hoverEnabled: true
            active: hovered || pressed
            orientation: Qt.Horizontal
            position: flickable.contentX / flickable.contentWidth
            size: flickable.width / flickable.contentWidth
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            policy: ScrollBar.AlwaysOn
        }
        Item {
            id: viewport
            // Translate moves the children to left... which would clip with other content in the scene even when clip=true
            z: -1
            property real logicalX: flickable.contentX / scene.scale
            property real logicalY: flickable.contentY / scene.scale
            anchors.fill: parent
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
            transform: [
                Scale {
                    origin.x: viewport.logicalX
                    origin.y: viewport.logicalY
                    xScale: scene.scale
                    yScale: scene.scale
                },
                Translate {
                    x: -viewport.logicalX
                    y: -viewport.logicalY
                }
            ]

            CursedCPUCanvas {
                id: canvas
                x: 0
                y: 0
                width: canvas.contentWidth
                height: canvas.contentHeight
            }
            Instantiator {
                model: canvas.overlays
                delegate: DelegateChooser {
                    id: chooser
                    role: "type"
                    DelegateChoice {
                        roleValue: 1
                        LabeledCheck {
                            parent: viewport
                        }
                    }
                    DelegateChoice {
                        roleValue: 2
                        LabeledTriState {
                            parent: viewport
                        }
                    }
                }
            }
        }
        Flickable {
            id: flickable
            z: -2
            anchors {
                left: parent.left
                right: vbar.left
                top: parent.top
                bottom: hbar.bottom
            }

            clip: true
            boundsBehavior: Flickable.StopAtBounds
            // Ensure that have non-empty content, even if the canvas is currently empty.
            contentWidth: viewport.implicitWidth * scene.scale
            contentHeight: viewport.implicitHeight * scene.scale
            // A dummy item which gives us something to scroll against
            Item {
                id: scene
                width: viewport.implicitWidth
                height: viewport.implicitHeight
            }

            function clamp(v, lo, hi) {
                return Math.max(lo, Math.min(hi, v));
            }
            // step is a multiplier on scale. oldCenterX/Y are "mouse" coordinates, and should be in [0..contentWidth/Height] not [0..flickable.widht/height]
            // This function computes 2 viewport translations: one which is centered on the exisiting viewport, and one centered on the mouse.
            // These two translations are linerally interpolated using alpha. alpha=0 means zoom to the mouse, alpha=1 means zoom to the center of the viewport.
            function zoomTo(step, oldMouseX, oldMouseY, alpha) {
                // Compute & bound old and new scale factors
                const z0 = scene.scale;
                // Our graphics items are low-res. Don't allow >3x zoom
                const z1 = flickable.clamp(z0 * step, 0.5, 3.0);
                // Skip the scaling/translation if we clamped.
                if (z1 === z0)
                    return;

                // ratio of new to old, and cache width/height
                const k = z1 / z0, W = flickable.width, H = flickable.height;
                const W2 = W / 2, H2 = H / 2;
                // contentX/Y are already scaled, and are for the topleft corner of the viewport.
                // But, we want to scale about the center of the viewport, which is at contentX/Y + W/2,H/2 in scaled coordinates.
                // Add H/W to compute center under old scale, then scale to new coordinates, then subtract H/W to get back to top-left.
                const centered_x_old = flickable.contentX + W2;
                const centered_y_old = flickable.contentY + H2;
                const centered_x_new = centered_x_old * k - W2;
                const centered_y_new = centered_y_old * k - H2;
                // Repeat the calculation from above, but treating the mouse as the center.
                // Zoom in and zoom out should be inverse operations, so we need to repulse the mouse on zoom out
                // to invert the "attraction" on zoom in. To do so, compute the distance of the mouse from the center, and flip the sign when zooming out.
                const mouse_x_old_centered = (step < 1 ? -1 : 1) * (oldMouseX - centered_x_old);
                const mouse_y_old_centered = (step < 1 ? -1 : 1) * (oldMouseY - centered_y_old);
                const mouse_x_new = (centered_x_old + mouse_x_old_centered) * k - W2;
                const mouse_y_new = (centered_y_old + mouse_y_old_centered) * k - H2;
                scene.scale = z1;
                // Blend the "centered" and "mouse" new coordinates, to get a smoother zooming experience.
                // If step<1, we are zooming out, and we want to "rep
                flickable.contentX = alpha * centered_x_new + (1 - alpha) * mouse_x_new;
                flickable.contentY = alpha * centered_y_new + (1 - alpha) * mouse_y_new;
                // If the new X/Y is out of bounds (which can happen when zooming out), rebound to valid coordinates gently.
                flickable.returnToBounds();
            }
            // Ordering before WheelHandler seems to avoid focus being stolen?
            PinchHandler {
                //Must be null, otherwise we will modify the flickable in-place.
                //If we add zoom-to-mouse in the future, coordinate will need to be translated to flickable-relative.
                target: null
                onActiveChanged: function () {
                    if (!active) {
                        flickable.returnToBounds();
                    }
                }
                onScaleChanged: function (delta) {
                    // Do not attempt to zoom to mouse, just center.
                    flickable.zoomTo(delta, -1, -1, 1);
                }
                scaleAxis.minimum: 0.5
                scaleAxis.maximum: 3.0
            }

            WheelHandler {
                target: flickable
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                acceptedModifiers: Qt.AltModifier
                // Zoom about the center of the viewport. Previously, I attempted zooming about the cursor
                // Zooming to cursor was jumpy and worse the further the cursors was from the center.
                onWheel: function (e) {
                    e.accepted = true;
                    const step = (e.angleDelta.y > 0) ? 1.1 : 1.0 / 1.1;
                    flickable.zoomTo(step, e.x, e.y, .75);
                }
            }
        }
    }
    // A dummy right-panel
    Rectangle {
        id: rpanel
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: 325
        color: "red"
    }
}
