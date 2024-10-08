import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: wrapper

    property bool isEnabled: true
    property int buttonWidth: 100
    property int buttonHeight: 20

    property alias bold: boldCB.checked
    property alias italics: italicsCB.checked
    property alias underline: underlineCB.checked
    property alias strikeout: strikeoutCB.checked

    implicitWidth: layout.childrenRect.width
    implicitHeight: layout.childrenRect.height

    //  Indicates user changed font properties
    signal updatedFont(int fontProperty, bool value)

    GridLayout {
        id: layout
        columns: 2
        columnSpacing: 2
        rowSpacing: 2

        CheckBox {
            id: boldCB
            text: "Bold"
            enabled: isEnabled

            onClicked: {
                wrapper.updatedFont(3 /*PreferenceModel.PrefProperty.Bold*/
                                    , boldCB.checked)
            }
        }
        CheckBox {
            id: italicsCB
            enabled: isEnabled
            text: "Italics"
            onClicked: {
                wrapper.updatedFont(4 /*PreferenceModel.PrefProperty.Italic*/
                                    , italicsCB.checked)
            }
        }
        CheckBox {
            id: underlineCB
            text: "Underline"
            enabled: isEnabled

            onClicked: {
                wrapper.updatedFont(5 /*PreferenceModel.PrefProperty.Underline*/
                                    , underlineCB.checked)
            }
        }
        CheckBox {
            id: strikeoutCB
            text: "Strikeout"
            enabled: isEnabled

            onClicked: {
                wrapper.updatedFont(6 /*PreferenceModel.PrefProperty.Strikeout*/
                                    , strikeoutCB.checked)
            }
        } //  RowLayout
        RowLayout {
            Layout.columnSpan: 2
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    } //  ColumnLayout
}
