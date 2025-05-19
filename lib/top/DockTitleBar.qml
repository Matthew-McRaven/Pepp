/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

import QtQuick 2.9
import QtQuick.Window 2.15

// Will be moved to a plugin in the future, if there's enough demand
import "qrc:/kddockwidgets/qtquick/views/qml/" as KDDW

/**
  * Copied from TitleBar.qml, with buttons removed
 */
KDDW.TitleBarBase {
    id: root

    color: "#eff0f1"
    heightWhenVisible: 30

    function dpiSuffix(): string {
        // Since Qt's built-in @Nx doesn't support fractionals, we load the correct image manually
        if (Screen.devicePixelRatio === 1) {
            return "";
        } else if (Screen.devicePixelRatio === 1.5) {
            return "-1.5x";
        } else if (Screen.devicePixelRatio === 2) {
            return "-2x";
        } else {
            return "";
        }
    }

    function imagePath(id: string): string {
        return "qrc:/img/" + id + dpiSuffix() + ".png";
    }

    Text {
        id: title
        text: root.title
        anchors {
            left: parent ? parent.left : undefined
            leftMargin: 5
            verticalCenter: parent.verticalCenter
        }
    }
}
