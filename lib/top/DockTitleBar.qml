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
import "qrc:/qt/qml/com/kdab/dockwidgets" as KDDW


/**
  * Copied from TitleBar.qml, with buttons removed
 */
KDDW.TitleBar {
    id: root
    hideButtons: true
    color: palette.window
}
