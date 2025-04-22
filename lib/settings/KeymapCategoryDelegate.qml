import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import edu.pepp 1.0

Item {
    id: root
    property int activeKeyMapping: 0
    property int buttonWidth: 50

    ListModel {
        id: mappingScheme
        ListElement {
            name: "Visual Studio"
            active: true
        }
        ListElement {
            name: "Qt"
            active: false
        }
        ListElement {
            name: "VIM"
            active: false
        }
    }
    ListModel {
        id: keyMappingModel
        ListElement {
            area: "Editor"
            name: "File Save"
            description: "Hot key for saving file"
            shortcut: "Ctrl + S"
        }
        ListElement {
            area: "Editor"
            name: "File Open"
            description: "Hot key for opening file"
            shortcut: "Ctrl + O"
        }
        ListElement {
            area: "Debugging"
            name: "Start Debugging"
            description: "Hot key starting debugging"
            shortcut: "F5"
        }
        ListElement {
            area: "Debugging"
            name: "Step Out"
            description: "Hot key stepping out of current function"
            shortcut: "F11"
        }
    }

    //  Section heading component
    Component {
        id: sectionDelegate
        Rectangle {
            width: ListView.view.width
            height: childrenRect.height
            color: palette.base

            required property string section

            Text {
                //  Short term hack. Will be replacing Listview with TreeView
                //  Use logic for screen presentation
                topPadding: parent.section != "Editor" ? 10 : 0
                text: parent.section
                font.bold: true
                color: palette.text
            }
        }
    }

    //  Screen background and border
    Rectangle {
        id: bg
        color: palette.window
        anchors {
            fill: parent
            margins: border.width
        }
        border {
            color: palette.text
            width: 1
        }
    }

    ColumnLayout {
        spacing: 6
        anchors {
            fill: parent
            margins: 8 * bg.border.width
        }

        //  Screen elements are laid out vertically
        RowLayout {
            Layout.fillWidth: true
            Layout.minimumHeight: childrenRect.height
            Layout.maximumHeight: childrenRect.height

            Label {
                id: textfont
                text: qsTr(" Current Mapping Scheme: ")
            }

            ComboBox {
                id: keyMappingsCombo
                textRole: "name"
                model: mappingScheme

                property bool isSystemMapping: true
            }
            Button {
                id: renameBtn
                text: "Rename"
                Layout.minimumWidth: root.buttonWidth
                enabled: !keyMappingsCombo.isSystemMapping
                palette {
                    buttonText: keyMappingsCombo.isSystemTheme ? root.palette.placeholderText : root.palette.buttonText
                }
                onPressed: {

                    //renameDialog.open()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumHeight: childrenRect.height
            Layout.maximumHeight: childrenRect.height

            //  Spacer to align buttons
            Label {
                width: 13
            }

            Button {
                id: copyButton
                //  System themes can never have state change
                //  If non-system theme has changes, they must be saved before a copy can be made
                text: keyMappingsCombo.isSystemMapping ? "Copy" : "Save"
                Layout.minimumWidth: root.buttonWidth
                onPressed: {
                    if (keyMappingsCombo.isSystemMapping) {
                        const curIdx = keyMappingsCombo.currentIndex
                        // Copy also creates the duplicate item for us!
                        const index = -1 // onDisk.copy(keyMappingsCombo.currentIndex)
                        if (index !== -1) {
                            keyMappingsCombo.currentIndex = index
                            requestRename()
                        }
                    } else {

                        //FileIO.save(keyMappingsCombo.currentValue,
                        //            settings.extPalette.jsonString())
                    }
                }
                signal requestRename
            }
            Button {
                id: del
                text: "Delete"
                Layout.minimumWidth: root.buttonWidth
                enabled: !keyMappingsCombo.isSystemMapping
                onClicked: {
                    const index = keyMappingsCombo.currentIndex
                    if (index !== -1) {
                        //onDisk.deleteTheme(index)
                        keyMappingsCombo.currentIndex = Math.min(
                                    index, keyMappingsCombo.count - 1)
                    }
                }
                palette {
                    buttonText: keyMappingsCombo.isSystemMapping ? root.palette.placeholderText : root.palette.buttonText
                }
            }
            Button {
                text: "Import"
                Layout.minimumWidth: root.buttonWidth
                onClicked: importLoader.item.open()
            }
            Button {
                id: exportBtn
                text: "Export"
                Layout.minimumWidth: root.buttonWidth
                // Now allows export of system key mappings. This makes it easier to keep theme files up-to-date.
                onClicked: exportLoader.item.open()
                palette {
                    buttonText: root.palette.buttonText
                }
            }
        }

        //  Spacer to separate sections
        Rectangle {
            Layout.fillWidth: true
            Layout.minimumHeight: 4
            Layout.maximumHeight: 4

            color: palette.window
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumHeight: childrenRect.height
            Layout.maximumHeight: childrenRect.height
            Label {
                text: qsTr("Search Shortcuts:")
            }

            TextField {
                Layout.fillWidth: true
            }
        }

        //  Wrap listview with rectangle to get white background. If listview
        //  does not fill view, we get a gray backround.
        Rectangle {

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 1
            color: palette.base

            border {
                width: 1
                color: palette.text
            }
            ListView {
                id: keyMappingList

                anchors.fill: parent
                anchors.margins: 5

                model: keyMappingModel

                focus: true
                focusPolicy: Qt.StrongFocus

                //  Do not select any item
                Component.onCompleted: {
                    currentIndex = -1
                }

                Keys.onUpPressed: keyMappingList.currentIndex = Math.max(
                                      0, keyMappingList.currentIndex - 1)
                Keys.onDownPressed: keyMappingList.currentIndex = Math.min(
                                        keyMappingList.count - 1,
                                        keyMappingList.currentIndex + 1)

                header: RowLayout {
                    spacing: 0
                    height: command.height

                    Rectangle {
                        implicitHeight: childrenRect.height
                        implicitWidth: childrenRect.width
                        Layout.minimumWidth: childrenRect.width
                        Layout.maximumWidth: childrenRect.width
                        Text {
                            id: command
                            text: "Command"
                            width: 110
                            color: palette.text
                            //font.bold: true
                            font.pointSize: textfont.font.pointSize * 1.3
                        }
                    }
                    Rectangle {
                        implicitHeight: childrenRect.height
                        implicitWidth: childrenRect.width
                        Layout.minimumWidth: childrenRect.width
                        Layout.maximumWidth: childrenRect.width
                        Text {
                            id: shortcut
                            width: 75
                            text: "Shortcut"
                            color: palette.text
                            //font.bold: true
                            font.pointSize: command.font.pointSize
                        }
                    }
                    Rectangle {
                        implicitWidth: childrenRect.width
                        implicitHeight: childrenRect.height
                        Layout.minimumWidth: childrenRect.width
                        Layout.maximumWidth: childrenRect.width

                        Text {
                            id: description
                            text: "Description"
                            color: palette.text
                            //font.bold: true
                            font.pointSize: command.font.pointSize
                        }
                    }
                    //  Fill in remaining listview
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        color: ListView.isCurrentItem ? palette.highlight : palette.base
                    }
                }

                delegate: RowLayout {
                    id: wrapper
                    required property string name
                    required property string description
                    required property string shortcut

                    spacing: 0
                    height: commandName.height
                    width: wrapper.ListView.width

                    //  Spacer for left indent
                    Rectangle {
                        implicitWidth: 10
                        Layout.minimumWidth: 10
                        Layout.maximumWidth: 10
                        Layout.fillHeight: true

                        color: ListView.isCurrentItem ? palette.highlight : palette.base
                    }
                    Rectangle {
                        id: commandName
                        implicitHeight: childrenRect.height
                        implicitWidth: childrenRect.width
                        Layout.minimumWidth: childrenRect.width
                        Layout.maximumWidth: childrenRect.width

                        color: ListView.isCurrentItem ? palette.highlight : palette.base

                        Text {
                            width: 100
                            text: wrapper.name
                            color: wrapper.ListView.isCurrentItem ? palette.highlightedText : palette.text
                        }
                    }

                    Rectangle {
                        implicitHeight: childrenRect.height
                        implicitWidth: childrenRect.width
                        Layout.minimumWidth: childrenRect.width
                        Layout.maximumWidth: childrenRect.width
                        Layout.fillHeight: true

                        color: ListView.isCurrentItem ? palette.highlight : palette.base

                        Text {
                            width: 75
                            text: wrapper.shortcut
                            color: wrapper.ListView.isCurrentItem ? palette.highlightedText : palette.text
                        }
                    }
                    //  Fill in remaining listview
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        color: ListView.isCurrentItem ? palette.highlight : palette.base

                        Text {
                            width: 100
                            text: wrapper.description
                            color: wrapper.ListView.isCurrentItem ? palette.highlightedText : palette.text
                        }
                    }
                }

                highlight: Rectangle {
                    color: palette.highlight
                }
                section.property: "area"
                section.criteria: ViewSection.FullString
                section.delegate: sectionDelegate
            }
        }
        //  Spacer to separate sections
        Rectangle {
            Layout.fillWidth: true
            Layout.minimumHeight: 4
            Layout.maximumHeight: 4
            color: palette.window
        }

        Label {
            text: qsTr("Add or remove shortcuts for selected command:")
        }

        RowLayout {
            Layout.minimumHeight: childrenRect.height
            Layout.maximumHeight: childrenRect.height
            TextField {
                Layout.fillWidth: true
            }
            Button {
                text: "Set"
                Layout.minimumWidth: root.buttonWidth
            }
            Button {
                text: "Reset"
                Layout.minimumWidth: root.buttonWidth
            }
        }
    }

    Loader {
        id: exportLoader
        Component.onCompleted: {
            const props = {
                "mode": "SaveFile",
                "title": "Export Key Mapping",
                "nameFilters": ["Pep Key Mapping files (*.keyMapping)"],
                "selectedNameFilter_index": 0,
                "defaultSuffix": "keyMapping",
                "selectedFile": "default.keyMapping"
            }

            if (PlatformDetector.isWASM) {
                setSource("qrc:/edu/pepp/settings/QMLFileDialog.qml", props)
            } else {
                setSource("qrc:/edu/pepp/settings/NativeFileDialog.qml", props)
            }
        }
        asynchronous: false
        Connections {
            target: exportLoader.item
            function onAccepted() {
                const path = decodeURIComponent(exportLoader.item.selectedFile)
                FileIO.save(path, settings.extPalette.jsonString())
            }
        }
    }
    Loader {
        id: importLoader
        Component.onCompleted: {
            const props = {
                "mode": "OpenFile",
                "title": "Import Key Mapping",
                "nameFilters": ["Pep Key Mapping files (*.keyMapping)"],
                "selectedNameFilter_index": 0,
                "defaultSuffix": "keyMapping"
            }

            if (PlatformDetector.isWASM) {
                setSource("qrc:/edu/pepp/settings/QMLFileDialog.qml", props)
            } else {
                setSource("qrc:/edu/pepp/settings/NativeFileDialog.qml", props)
            }
        }
        asynchronous: false
        Connections {
            target: importLoader.item
            function onAccepted() {
                const model = keyMappingsCombo.model
                const uri = decodeURIComponent(importLoader.item.selectedFile)
                const file = uri.replace("file:///", "")
                const index = model.importTheme(file)
                if (index !== -1)
                    keyMappingsCombo.currentIndex = index
            }
        }
    }
    Dialog {
        id: renameDialog
        title: "Rename Key Mapping"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        spacing: 5
        ColumnLayout {
            anchors.fill: parent
            Label {
                id: label
                text: "Key Mapping name:"
            }
            TextInput {
                id: name
                width: 100
                text: keyMappingsCombo.displayText
                focus: true
                color: palette.text
                validator: RegularExpressionValidator {
                    regularExpression: /^[^<>:;,?"*|\\/]+$/
                }
            }
        }

        onAccepted: {
            const model = keyMappingsCombo.model
            const index = model.index(keyMappingsCombo.currentIndex, 0)
            model.setData(index, name.text, model.display)
        }
    }
}
