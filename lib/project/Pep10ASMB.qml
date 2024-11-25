import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/edu/pepp/text/editor" as Text
import "qrc:/edu/pepp/memory/stack" as Stack
import "qrc:/edu/pepp/memory/hexdump" as Memory
import "qrc:/edu/pepp/memory/io" as IO
import "qrc:/edu/pepp/cpu" as Cpu
import "qrc:/edu/pepp/symtab" as SymTab
import edu.pepp 1.0

Item {
    id: wrapper
    required property var project
    required property var actions
    required property string mode
    Component.onCompleted: {
        // Must connect and disconnect manually, otherwise project may be changed underneath us, and "save" targets wrong project.
        // Do not need to update on mode change, since mode change implies loss of focus of objEdit.
        userAsmEdit.editingFinished.connect(save)
        project.errorsChanged.connect(displayErrors)
        project.listingChanged.connect(fixListings)
        onProjectChanged.connect(fixListings)
        project.charInChanged.connect(() => batchio.setInput(project.charIn))
        // Connect editor BPs to project
        userAsmEdit.editor.modifyLine.connect(project.onModifyUserSource)
        osAsmEdit.editor.modifyLine.connect(project.onModifyOSSource)
        userList.editor.modifyLine.connect(project.onModifyUserList)
        osList.editor.modifyLine.connect(project.onModifyOSList)
        // Connect project to editors
        project.modifyUserSource.connect(userAsmEdit.editor.onLineAction)
        project.modifyOSSource.connect(osAsmEdit.editor.onLineAction)
        project.modifyUserList.connect(userList.editor.onLineAction)
        project.modifyOSList.connect(osList.editor.onLineAction)
        // Update breakpoints on switch
        project.clearListingBreakpoints.connect(
                    userList.editor.onClearAllBreakpoints)
        project.clearListingBreakpoints.connect(
                    osList.editor.onClearAllBreakpoints)
        project.requestSourceBreakpoints.connect(
                    userAsmEdit.editor.onRequestAllBreakpoints)
        project.requestSourceBreakpoints.connect(
                    osAsmEdit.editor.onRequestAllBreakpoints)
        project.switchTo.connect(wrapper.onSwitchTo)
        if (project)
            fixListings()
        // Can't modify our mode directly because it would break binding with parent.
        // i.e., we can't be notified if editor is entered ever again.
        wrapper.actions.debug.start.triggered.connect(
                    wrapper.requestModeSwitchToDebugger)
        wrapper.actions.build.execute.triggered.connect(
                    wrapper.requestModeSwitchToDebugger)
        const list = [userAsmEdit, osAsmEdit, userList, osList]
        for (var i = 0; i < list.length; i++) {
            wrapper.actions.edit.copy.triggered.connect(list[i].onCopy)
            wrapper.actions.edit.cut.triggered.connect(list[i].onCut)
            wrapper.actions.edit.paste.triggered.connect(list[i].onPaste)
            wrapper.actions.edit.undo.triggered.connect(list[i].onUndo)
            wrapper.actions.edit.redo.triggered.connect(list[i].onRedo)
        }
    }
    // Will be called before project is changed on unload, so we can disconnect save-triggering signals.
    Component.onDestruction: {
        userAsmEdit.editingFinished.disconnect(save)
        if (project) {
            userAsmEdit.editor.modifyLine.disconnect(project.onModifyUserSource)
            osAsmEdit.editor.modifyLine.disconnect(project.onModifyOSSource)
            userList.editor.modifyLine.disconnect(project.onModifyUserList)
            osList.editor.modifyLine.disconnect(project.onModifyOSList)
            project.errorsChanged.disconnect(displayErrors)
            project.listingChanged.connect(fixListings)
            project.modifyUserSource.disconnect(userAsmEdit.editor.onLineAction)
            project.modifyOSSource.disconnect(osAsmEdit.editor.onLineAction)
            project.modifyUserList.disconnect(userList.editor.onLineAction)
            project.modifyOSList.disconnect(osList.editor.onLineAction)
            project.clearListingBreakpoints.disconnect(
                        userList.editor.onClearAllBreakpoints)
            project.clearListingBreakpoints.disconnect(
                        osList.editor.onClearAllBreakpoints)
            project.requestSourceBreakpoints.disconnect(
                        userAsmEdit.editor.onRequestAllBreakpoints)
            project.requestSourceBreakpoints.disconnect(
                        osAsmEdit.editor.onRequestAllBreakpoints)
            project.switchTo.disconnect(wrapper.onSwitchTo)
        }
        onProjectChanged.disconnect(fixListings)

        wrapper.actions.debug.start.triggered.disconnect(
                    wrapper.requestModeSwitchToDebugger)
        wrapper.actions.build.execute.triggered.disconnect(
                    wrapper.requestModeSwitchToDebugger)

        const list = [userAsmEdit, osAsmEdit, userList, osList]
        for (var i = 0; i < list.length; i++) {
            wrapper.actions.edit.copy.triggered.disconnect(list[i].onCopy)
            wrapper.actions.edit.cut.triggered.disconnect(list[i].onCut)
            wrapper.actions.edit.paste.triggered.disconnect(list[i].onPaste)
            wrapper.actions.edit.undo.triggered.disconnect(list[i].onUndo)
            wrapper.actions.edit.redo.triggered.disconnect(list[i].onRedo)
        }
    }
    signal requestModeSwitchTo(string mode)
    function requestModeSwitchToDebugger() {
        wrapper.requestModeSwitchTo("debugger")
    }
    function getLexerLangauge() {
        switch (project?.architecture) {
        case Architecture.PEP9:
            return "Pep/9 ASM"
        case Architecture.PEP10:
            return "Pep/10 ASM"
        default:
            return "Pep/10 ASM"
        }
    }

    function onSwitchTo(os) {
        textSelector.currentIndex = Qt.binding(() => os ? 1 : 0)
    }

    function displayErrors() {
        userAsmEdit.addEOLAnnotations(project.assemblerErrors)
    }
    function fixListings() {
        if (!project)
            return
        if (userList) {
            const curURO = userList.readOnly
            userList.readOnly = false
            userList.text = project.userList
            userList.addListingAnnotations(project.userListAnnotations)
            userList.readOnly = curURO
        }
        if (osList) {
            const curORO = osList.readOnly
            osList.readOnly = false
            osList.text = project.osList
            osList.addListingAnnotations(project.osListAnnotations)
            osList.readOnly = curORO
        }
    }
    // TODO: replace preAssemble someday...
    function syncEditors() {
        save()
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
                font.family: "Courier Prime"
            }

            SplitView {
                handle: split.handle
                orientation: Qt.Vertical
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: textSelector.bottom
                anchors.bottom: parent.bottom
                StackLayout {
                    visible: mode == "editor"
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
                        language: wrapper.getLexerLangauge()
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
                        language: wrapper.getLexerLangauge()
                    }
                }
                StackLayout {
                    visible: mode == "debugger"
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
                        language: wrapper.getLexerLangauge()
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
                        language: wrapper.getLexerLangauge()
                    }
                }
                TabBar {
                    id: debugTabBar
                    visible: mode == "debugger"
                    TabButton {
                        text: qsTr("Object Code")
                    }
                    TabButton {
                        text: qsTr(`Symbol Table: ${textSelector.currentText}`)
                    }
                }
                StackLayout {
                    currentIndex: debugTabBar.currentIndex
                    visible: mode == "debugger"
                    SplitView.minimumHeight: 100
                    clip: true
                    Text.ObjTextEditor {
                        id: objView
                        readOnly: true
                        // text is only an initial binding, the value diverges from there.
                        text: project?.objectCodeText ?? ""
                    }
                    SymTab.SymbolViewer {
                        id: symTab
                        model: textSelector.currentIndex
                               === 0 ? project?.userSymbols : project?.osSymbols
                    }
                }
            }
        }

        SplitView {
            visible: mode === "debugger"
            SplitView.minimumWidth: 280
            orientation: Qt.Vertical
            Cpu.RegisterView {
                id: registers
                SplitView.minimumHeight: 225
                registers: project?.registers ?? null
                flags: project?.flags ?? null
            }
            IO.Batch {
                SplitView.fillHeight: true
                width: parent.width
                id: batchio
                property bool ignoreInputChange: false
                function setInput(input) {
                    ignoreInputChange = true
                    batchio.input = input
                    ignoreInputChange = false
                }
                Component.onCompleted: {
                    onInputChanged.connect(() => {
                                               if (!ignoreInputChange)
                                               project.charIn = input
                                           })
                }
                output: project?.charOut ?? null
            }
        }
        Item {
            SplitView.minimumWidth: 340
            visible: mode === "debugger"
            TabBar {
                id: memoryTab
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                TabButton {
                    text: qsTr("Hex Dump")
                }
                TabButton {
                    text: qsTr("Stack Trace")
                }
            }
            StackLayout {
                anchors {
                    top: memoryTab.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                currentIndex: memoryTab.currentIndex
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
                        setSource("qrc:/edu/pepp/memory/hexdump/MemoryDump.qml",
                                  props)
                    }
                    asynchronous: true
                    onLoaded: {
                        loader.item.scrollToAddress(project.currentAddress)
                        con.enabled = true
                    }
                }
                Stack.StackTrace {}
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
