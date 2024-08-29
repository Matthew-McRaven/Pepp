import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: wrapper
    required property int architecture
    required property int abstraction
    property bool enabled: true
    property alias text: title.text
    property alias description: description.text
    property real textPadding: 5

    width: 150
    height: 150
    // We cannot use button, since we want to control the foreground image, which interferes with `hover`.
    // Since `hover` is RO on a button, we can't set it from the MouseArea.
    // Therefore, we need to imitate the behaviors/styling of Button in place.
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onEntered: infoOverlay.state = "hovered"
        onExited: infoOverlay.state = "unhovered"
        onClicked: {
            console.log("Card clicked")
            // Magic function that exists in Welcome.qml and is exposed to us.
            if (wrapper.enabled)
                addProject(architecture, abstraction, "", false)
        }
    }
    color: wrapper.enabled ? palette.button : palette.button.darker(1.4)

    // Placeholder for image showing current project type.
    Rectangle {
        id: image
        anchors {
            right: parent.right
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            // Ensure that the image does not clip the title text
            bottomMargin: title.height + title.topPadding + title.bottomPadding
            margins: wrapper.textPadding
        }
        color: 'red'
    }
    // "Popup" with persistent tile and description shown on hover,
    Item {
        id: infoOverlay

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        // Must nest background to prevent making text more opaque
        Rectangle {
            anchors.fill: parent
            opacity: 0.6
            color: wrapper.color
        }

        // Must clip or description text will overflow into buttons below.
        clip: true
        // Animate height to slide in and out.
        height: title.height
        state: "unhovered"
        states: [
            State {
                name: "unhovered"
                PropertyChanges {
                    target: infoOverlay
                    height: title.height
                }
            },
            State {
                name: "hovered"
                PropertyChanges {
                    target: infoOverlay
                    height: parent.height
                }
            }
        ]
        transitions: Transition {
            NumberAnimation {
                properties: "height"
            }
        }
        Column {
            anchors.fill: parent
            anchors.margins: wrapper.textPadding
            WelcomeTitle {
                id: title
                width: parent.width
                bottomPadding: wrapper.textPadding
            }
            // Hidden by default when not hovered.
            Rectangle {
                border.color: palette.text
                height: 5
                border.width: height
                width: parent.width
            }
            Text {
                id: description
                text: "Describes the usecase of this project type..."
                color: palette.text
                wrapMode: Text.WordWrap
                width: parent.width
            }
        }
    }
}