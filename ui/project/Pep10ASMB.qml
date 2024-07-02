import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/ui/text/editor" as Text
import "qrc:/ui/memory/hexdump" as Memory
import "qrc:/ui/memory/io" as IO
import "qrc:/ui/cpu" as Cpu
import edu.pepp

Item {
    id: wrapper
    required property var project
    required property string mode
    Component.onCompleted: {

        // Must connect and disconnect manually, otherwise project may be changed underneath us, and "save" targets wrong project.
        // Do not need to update on mode change, since mode change implies loss of focus of objEdit.
        userAsmEdit.editingFinished.connect(save)
        project.errorsChanged.connect(displayErrors)
        project.listingChanged.connect(function () {
            const curURO = userList.readOnly
            userList.readOnly = false
            userList.text = project.userList
            userList.addListingAnnotations(project.userListAnnotations)
            userList.readOnly = curURO
            const curORO = osList.readOnly
            osList.readOnly = false
            osList.text = project.osList
            osList.addListingAnnotations(project.osListAnnotations)
            osList.readOnly = curORO
        })
    }
    // Will be called before project is changed on unload, so we can disconnect save-triggering signals.
    Component.onDestruction: {
        userAsmEdit.editingFinished.disconnect(save)
        project.errorsChanged.disconnect(displayErrors)
    }
    function displayErrors() {
        userAsmEdit.addEOLAnnotations(project.assemblerErrors)
    }

    function save() {
        // Supress saving messages when there is no project.
        if (project === null)
            return
        else if (!userAsmEdit.readOnly) {
            project.userAsmText = userAsmEdit.text
        }
    }

    function preAssemble() {
        if (project === null)
            return
        project.userAsmText = userAsmEdit.text
        project.osAsmText = osAsmEdit.text
    }

    SplitView {
        id: split
        anchors.fill: parent
        orientation: Qt.Horizontal
        handle: Item {
            implicitWidth: 4
            Rectangle {
                implicitWidth: 4
                anchors.horizontalCenter: parent.horizontalCenter
                height: parent.height
                // TODO: add color for handle
                color: palette.base
            }
        }

        Item {
            SplitView.minimumWidth: 100
            SplitView.fillWidth: true
            ComboBox {
                id: textSelector
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                model: ["User", "OS"]
            }
            FontMetrics {
                id: editorFM
                font.family: "Courier New"
            }

            SplitView {
                handle: split.handle
                orientation: Qt.Vertical
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: textSelector.bottom
                anchors.bottom: parent.bottom
                StackLayout {
                    visible: mode == "edit"
                    currentIndex: textSelector.currentIndex
                    SplitView.fillHeight: true
                    Text.ScintillaAsmEdit {
                        id: userAsmEdit
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        // text is only an initial binding, the value diverges from there.
                        text: project?.userAsmText ?? ""
                        editorFont: editorFM.font
                        language: "Pep/10 ASM"
                    }
                    Text.ScintillaAsmEdit {
                        id: osAsmEdit
                        Component.onCompleted: {
                            // Don't set declaratively, otherwise text will not be repainted.
                            osAsmEdit.readOnly = Qt.binding(() => true)
                        }
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        // text is only an initial binding, the value diverges from there.
                        text: project?.osAsmText ?? ""
                        editorFont: editorFM.font
                        language: "Pep/10 ASM"
                    }
                }
                StackLayout {
                    visible: mode == "debug"
                    currentIndex: textSelector.currentIndex
                    SplitView.fillHeight: true
                    Text.ScintillaAsmEdit {
                        id: userList
                        readOnly: true
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        // text is only an initial binding, the value diverges from there.
                        text: project?.userList ?? ""
                        editorFont: editorFM.font
                        language: "Pep/10 ASM"
                    }
                    Text.ScintillaAsmEdit {
                        id: osList
                        readOnly: true
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        // text is only an initial binding, the value diverges from there.
                        text: project?.osList ?? ""
                        editorFont: editorFM.font
                        language: "Pep/10 ASM"
                    }
                }
                Text.ObjTextEditor {
                    id: objView
                    readOnly: true
                    // text is only an initial binding, the value diverges from there.
                    text: project?.objectCodeText ?? ""
                    SplitView.minimumHeight: 100
                }
            }
        }

        SplitView {
            visible: mode === "debug"
            SplitView.minimumWidth: 200
            orientation: Qt.Vertical
            Cpu.RegisterView {
                id: registers
                SplitView.minimumHeight: 200
                registers: project?.registers ?? null
                flags: project?.flags ?? null
            }
            IO.Batch {
                SplitView.fillHeight: true
                width: parent.width
                id: batchio
                Component.onCompleted: {
                    onInputChanged.connect(() => project.charIn = input)
                }
                output: project?.charOut ?? null
            }
        }
        Loader {
            id: loader
            Component.onCompleted: {
                const props = {
                    "memory": project.memory,
                    "mnemonics": project.mnemonics
                }
                // Construction sets current address to 0, which propogates back to project.
                // Must reject changes in current address until component is fully rendered.
                con.enabled = false
                setSource("qrc:/ui/memory/hexdump/MemoryDump.qml", props)
            }
            visible: mode === "debug"
            asynchronous: true
            SplitView.minimumWidth: 340
            onLoaded: {
                loader.item.scrollToAddress(project.currentAddress)
                con.enabled = true
            }
        }
    }
    Connections {
        id: con
        enabled: false
        target: loader.item
        function onCurrentAddressChanged() {
            project.currentAddress = loader.item.currentAddress
        }
    }
}
