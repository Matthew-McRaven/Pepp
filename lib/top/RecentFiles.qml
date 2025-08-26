pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material    //  Used by TabBar
import QtQuick.Layouts
import edu.pepp 1.0

Item {
    id: root
    property int spacing: 10
    property int buttonWidth: 75
    property int cellHeight: 80
    property int cellWidth: 300
    required property var font

    property color borderColor: settings.extPalette.brightText.background
    property var recentFiles: settings.general.recentFiles

    signal openFile(string path, int arch, int abstraction)

    //  Layout does not work without implicit height and width
    implicitHeight: childrenRect.height
    implicitWidth: childrenRect.width

    /*Component.onCompleted: {
        //  GridLayout does not automatically calculate number of rows
        //  based on total cells / columns.
        console.log("onCompleted");
        sizeChanged();
    }

    onWidthChanged: { console.log("onWidth");sizeChanged();}
    onHeightChanged: { console.log("onHeight");sizeChanged();}

    function sizeChanged()
    {
        //if(bar.currentIndex === 0)
        //  There is always a single row at minimum
        console.log("layout.cells",layout.cells,"layout.rowCnt",layout.rowCnt,"layout.colCnt",layout.colCnt);
        console.log("layout.height",layout.height,"layout.width",layout.width);
        console.log("ep.height",ep.height,"ep.width",ep.width);
        //console.log("sl.height",sl.height,"sl.width",sl.width);
        console.log("root.height",root.height,"root.width",root.width);
    }*/

    ColumnLayout {
        anchors.fill: root
        TabBar {
            id: bar
            implicitWidth: Math.max(root.width, root.buttonWidth * 2 + root.spacing)
            currentIndex: root.recentFiles.length > 0 ? 0 : 1

            //  This erases underline that spans page under tabs
            Material.accent: "transparent"
            background: Rectangle {
                //  Default background is a very light gray. Eliminate
                color: "transparent"
            }

            TabButton {
                id: tab1
                visible: root.recentFiles.length > 0
                width: root.recentFiles.length > 0 ? root.buttonWidth : 0

                contentItem: Text {
                    id: txt1
                    text: "Recent"
                    horizontalAlignment: Text.AlignHCenter
                    color: tab1.checked ? palette.text : settings.extPalette.brightText.background
                }

                //  Replace existing tab background
                background: Rectangle {
                    id: bg1
                    anchors.fill: tab1

                    //  Show underline when active
                    Rectangle {
                        visible: tab1.checked
                        anchors.bottom: bg1.bottom
                        anchors.bottomMargin: 4
                        anchors.horizontalCenter: bg1.horizontalCenter

                        height: 4
                        width: txt1.contentWidth
                        color: palette.highlight
                        radius: 2
                    }
                }   //  background: Rectangle
            }   //  TabButton - tab1
            TabButton {
                id: tab2
                width: root.buttonWidth

                contentItem: Text {
                    id: txt2
                    text: "Favorites"
                    horizontalAlignment: Text.AlignHCenter
                    color: tab2.checked ? palette.text : settings.extPalette.brightText.background
                }

                //  Replace existing tab background
                background: Rectangle {
                    id: bg2
                    anchors.fill: tab2

                    //  Show underline when active
                    Rectangle {
                        visible: tab2.checked
                        anchors.bottom: bg2.bottom
                        anchors.bottomMargin: 4
                        anchors.horizontalCenter: bg2.horizontalCenter

                        height: 4
                        width: txt2.contentWidth
                        color: palette.highlight
                        radius: 2
                    }
                }   //  background: Rectangle
            }   //  TabButton - tab2
        }   //  TabBar

        StackLayout {
            id: sl
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: bar.currentIndex

            //  Existing Project
            ScrollView {
                id: ep
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.bottomMargin: ep.effectiveScrollBarHeight
                Layout.minimumHeight: root.cellHeight

                ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                ScrollBar.horizontal.policy: ep.width < layout.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

                function abstractText(value) {
                    switch (value) {
                    case Abstraction.MC2:
                        return "MC2";
                    case Abstraction.ISA3:
                        return "ISA3";
                    case Abstraction.ASMB3:
                        return "ASMB3";
                    case Abstraction.OS4:
                        return "OS4";
                    case Abstraction.ASMB5:
                        return "ASMB5";
                    }
                }

                function archText(value) {
                    switch (value) {
                    case Architecture.PEP10:
                        return "Pep/10";
                    case Architecture.RISCV:
                        return "RISC-V";
                    case Architecture.PEP9:
                        return "Pep/9";
                    case Architecture.PEP8:
                        return "Pep/8";
                    }
                }

                //  Grid sizing is based on number of cells allocated over rows and columns
                //  Scroll view will show scroll bars if GridLayout does not fit
                //  NOT NOT SIZE GridLayout BASED ON PARENT WIDTH OR HEIGHT
                GridLayout {
                    id: layout

                    //  Grid control does not return number of rows. Rows prorperty only sets maximum rows.
                    //  Must recalculate rows manually with rowChange() below.
                    property int cells: root.recentFiles.length

                    //  Spacing between cells
                    rowSpacing: root.spacing
                    columnSpacing: root.spacing
                    columns: Math.ceil(cells / rows)
                    rows: Math.min(cells, Math.max(1, Math.floor((ep.height + root.spacing) / (root.cellHeight + root.spacing))))

                    implicitHeight: rows * root.cellHeight + (rows-1) * root.spacing
                    implicitWidth: columns * root.cellWidth + (columns-1) * root.spacing

                    //  For file listing, show from top to bottom.
                    flow: GridLayout.TopToBottom

                    Repeater {
                        id: repeaterExisting

                        model: root.recentFiles

                        //  Delegate for file listing
                        delegate: Rectangle {
                            id: btn
                            implicitHeight: root.cellHeight
                            implicitWidth: root.cellWidth
                            Layout.topMargin: 0

                            required property var model

                            radius: 5
                            color: palette.window

                            //  Change outline when hovered
                            border.width: mouse.containsMouse ? 2 : 1
                            border.color: mouse.containsMouse ? palette.accent : root.borderColor

                            MouseArea {
                                id: mouse
                                anchors.fill: btn
                                hoverEnabled: true

                                onClicked: {
                                    //  Signal that file is ready to be opened
                                    root.openFile(btn.model.path, btn.model.arch, btn.model.abstraction);
                                }
                            }

                            ColumnLayout {

                                anchors.left: btn.left
                                anchors.top: btn.top
                                anchors.right: btn.right
                                anchors.bottom: btn.bottom
                                anchors.leftMargin: 10

                                implicitHeight: childrenRect.height
                                implicitWidth: childrenRect.width
                                spacing: 0

                                Item {
                                    //  Spacer
                                    Layout.fillHeight: true
                                }
                                Label {
                                    //  File name
                                    id: name
                                    Layout.fillWidth: true
                                    text: settings.general.fileNameFor(btn.model.path)
                                    font.bold: true
                                    font.pointSize: root.font.pointSize * 1.2
                                    color: palette.accent
                                }
                                Label {
                                    //  Architecture details
                                    id: arch
                                    Layout.fillWidth: true
                                    text: "<b>" + ep.archText(btn.model.arch) + "</b>: " + ep.abstractText(btn.model.abstraction)
                                }
                                Text {
                                    //  Full file path
                                    id: path
                                    text: btn.model.path
                                    Layout.fillWidth: true
                                    textFormat: Text.PlainText
                                    maximumLineCount: 2
                                    lineHeight: 0.85
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                }
                                Item {
                                    //  Spacer
                                    Layout.fillHeight: true
                                }
                            }   //  ColumnLayout
                        }   //  Rectangle - delegate
                    }   //  Repeater
                }   //  GridLayout
            }   //  ScrollView
            //  Favorites
            Rectangle {
                id: favoriteProject
                Layout.fillWidth: true

                ColumnLayout {
                    Layout.fillWidth: true
                    Repeater {
                        id: repeaterFavorite
                        Layout.fillWidth: true
                        model: FavoriteFigureModel {
                            id: favModel
                        }

                        Rectangle {
                            id: btn2
                            implicitHeight: root.cellHeight
                            implicitWidth: root.cellWidth

                            required property int index
                            required property var figure
                            required property string name
                            required property string type
                            required property string description

                            radius: 5
                            color: palette.window

                            //  Change outline when hovered
                            border.width: mouse2.containsMouse ? 2 : 1
                            border.color: mouse2.containsMouse ? palette.accent : root.borderColor

                            MouseArea {
                                id: mouse2
                                anchors.fill: btn2
                                hoverEnabled: true

                                onClicked: {
                                    //  Signal that file is ready to be opened
                                    //root.openFile(btn.model.path, btn.model.arch, btn.model.abstraction);
                                }
                            }
                            ColumnLayout {

                                anchors.left: btn2.left
                                anchors.top: btn2.top
                                anchors.right: btn2.right
                                anchors.bottom: btn2.bottom
                                anchors.margins: 10

                                implicitHeight: childrenRect.height
                                implicitWidth: childrenRect.width
                                spacing: 0

                                Item {
                                    //  Spacer
                                    Layout.fillHeight: true
                                }
                                Label {
                                    Layout.fillWidth: true

                                    text: "<b>Figure:<b> " + btn2.name
                                    font.pointSize: root.font.pointSize * 1.2
                                    color: palette.accent
                                }
                                Label {
                                    Layout.fillWidth: true

                                    text: btn2.description
                                    maximumLineCount: 2
                                    lineHeight: 0.85
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideLeft
                                }
                                Item {
                                    //  Spacer
                                    Layout.fillHeight: true
                                }
                            }
                        }
                    }   //  Repeater
                }   //  ColumnLayout
            }
        }
    }   //  ColumnLayout

}
