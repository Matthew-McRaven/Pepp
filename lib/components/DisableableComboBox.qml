// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
pragma ComponentBehavior

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.Material

ComboBox {
    id: control

    //model: ["First", "Second", "Third"]
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding,
                            90)
    implicitHeight: Math.max(
                        implicitBackgroundHeight + topInset + bottomInset,
                        implicitContentHeight + topPadding + bottomPadding,
                        implicitIndicatorHeight + topPadding + bottomPadding)
    padding: 2

    property color textColor: palette.text

    //  Force text color on start.
    Component.onCompleted: {
        setTextColor()
    }

    onEnabledChanged: {
        setTextColor()
    }

    function setTextColor() {
        textColor = Qt.binding(
                    () => control.enabled ? palette.text : palette.placeholderText)
        canvas.requestPaint()
    }

    //  Items in listbox
    delegate: ItemDelegate {
        id: delegate

        required property var model
        required property int index

        width: control.width

        //  Height controls space between items in drop down
        height: 20

        //  Rectangle is a hack to set the background color
        contentItem: Rectangle {
            anchors.fill: parent
            Text {
                anchors.fill: parent
                padding: 0
                text: delegate.model[control.textRole]

                font.family: control.font.family
                font.pointSize: control.font.pointSize
                font.bold: index === control.currentIndex
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter

                //  Change text color of item currently under the mouse cursor
                color: index === control.highlightedIndex ? palette.highlightedText : palette.text
            }
            //  Set color of item currently under the mouse cursor
            color: index === control.highlightedIndex ? palette.highlight : "transparent"
        }
        highlighted: control.highlightedIndex === index
    }

    //  Drop down button
    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 10
        height: 6
        contextType: "2d"

        Connections {
            target: control
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
            context.fillStyle = control.textColor
            context.fill()
        }
    }

    //  Display text in control (not drop list)
    contentItem: Text {
        leftPadding: 0
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: control.textColor
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    //  Combobox background (not drop list)
    background: Rectangle {
        implicitWidth: control.implicitBackgroundWidth
        implicitHeight: control.implicitBackgroundHeight
        color: control.enabled ? palette.base : palette.button
        border.color: control.enabled ? palette.mid : palette.placeholderText
        border.width: control.visualFocus ? 2 : 1
        radius: 2
    }

    //  This is the dropdown part of the combo box. This area formats the background.
    //  The delegate above populates this listview with the items in the selection delegateModel.
    //  Inactive when country is not enabled.
    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentHeight + 5
        padding: 2

        contentItem: ListView {
            clip: true
            spacing: 0
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        //  Background of drop down list
        background: Rectangle {
            color: palette.button
            border.color: palette.mid
            radius: 2
        }
    }
}
