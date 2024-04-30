

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
import Qt.labs.qmlmodels
import "../cpu" as Ui

Rectangle {
    id: wrapper
    property alias registers: registers.model
    FontMetrics {
        id: metrics
    }

    TableView {
        id: registers
        anchors.fill: parent
        columnWidthProvider: function (column) {
            return registers.model.columnCharWidth(
                        column) * metrics.averageCharacterWidth
        }
        delegate: Component {
            Text {
                required property bool readOnly
                required property string display
                text: display
                font: metrics.font
            }
        }
    }
}
