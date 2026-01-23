pragma ComponentBehavior: Bound

import QtQuick

Grid {
    id: root
    property int count
    property color color

    rows: root.count
    spacing: 0
    rowSpacing: 5

    Repeater {
        model: root.count
        delegate: Rectangle {
            id: endpoint
            color: root.color
            width: 5
            height: 5
        }
    }
}
