import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    required property variant lineModel
    required property bool active

    //  Pass in dimensions & font
    required property variant font
    required property double implicitAddressWidth
    required property double implicitValueWidth
    required property double implicitLineHeight
    required property double boldBorderWidth

    // Helpers to "math" the position of the background rectangle.
    property double valueX: 0
    property double valueWidth: 0
    property double valueY: 0
    property double valueHeight: 0

    //  Bubble children size up to parent control for correct sizing
    implicitHeight: column.implicitHeight
    implicitWidth: column.implicitWidth

    NuAppSettings {
        id: settings
    }

    // Sorry all, this bit is cursed.
    // Instead of trying to do the "right thing" and use three columns+repeaters and place a rectangle
    // around the center column inside the middle repeater, I'm opting to only use a single repeater.
    // This reduces the amount of synchronization code between the repeaters/columns, but it also means
    // we no longer obviously know where the middle column is.
    // So, as we create our rows, we record the location of the middle column.
    // We'll math out the location of the background rectangle using those values.
    Rectangle {
        id: background
        x: root.valueX - border.width / 2
        y: root.valueY - border.width / 2
        color: "transparent"
        width: root.valueWidth + border.width
        height: column.height + border.width
        border.color: root.active ? palette.text : "transparent"
        border.width: root.boldBorderWidth
        z: 1
    }
    Column {
        id: column
        spacing: 0

        Repeater {
            model: root.lineModel
            RowLayout {
                spacing: 0
                Label {
                    Layout.fillWidth: false
                    Layout.preferredWidth: root.implicitAddressWidth
                    Layout.preferredHeight: root.implicitLineHeight
                    Layout.alignment: Qt.AlignLeft & Qt.AlignVCenter

                    font: root.font
                    text: `${model.address?.toString(16)?.padStart(4, '0')?.toUpperCase() ?? 0}`
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                Rectangle {
                    id: valueRect
                    Layout.fillWidth: false
                    Layout.preferredWidth: root.implicitValueWidth
                    Layout.preferredHeight: root.implicitLineHeight
                    Layout.alignment: Qt.AlignLeft & Qt.AlignVCenter

                    //  Force rectangle to overlap surrounding rectange
                    //  This forces a single line with adjacent rectanges
                    //  Without this offset, adjacent rectanges would have
                    //  double line width
                    Layout.margins: -.5

                    //  Color changes based on status
                    //  Update colors from theme when theme is complete
                    color: (model.status === ChangeType.Allocated) ? settings.extPalette.comment.foreground : (model.status === ChangeType.Modified) ? settings.extPalette.error.background : palette.base
                    border.color: palette.text
                    border.width: 1
                    Label {
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: root.font
                        text: model.value ?? ""
                    }
                }
                Label {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignLeft & Qt.AlignVCenter
                    Layout.preferredWidth: root.implicitValueWidth
                    Layout.preferredHeight: root.implicitLineHeight
                    Layout.leftMargin: 5

                    font: root.font
                    text: model.name ?? ""
                    color: palette.text
                }
                Component.onCompleted: {
                    // Record position of the value column so we can place the background rect.
                    if (model.row === 0) {
                        root.valueX = Qt.binding(function () {
                            return valueRect.x;
                        });
                        root.valueWidth = Qt.binding(function () {
                            return valueRect.width;
                        });
                        root.valueY = Qt.binding(function () {
                            return valueRect.y;
                        });
                        root.valueHeight = Qt.binding(function () {
                            return valueRect.height;
                        });
                    }
                }
            }
        }
    }
}
