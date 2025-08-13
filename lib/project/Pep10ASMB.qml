import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "qrc:/qt/qml/edu/pepp/text/editor" as Text
import "qrc:/qt/qml/edu/pepp/memory/stack" as Stack
import "qrc:/qt/qml/edu/pepp/memory/hexdump" as Memory
import "qrc:/qt/qml/edu/pepp/memory/io" as IO
import "qrc:/qt/qml/edu/pepp/cpu" as Cpu
import "qrc:/qt/qml/edu/pepp/toolchain/symtab" as SymTab
import "qrc:/qt/qml/edu/pepp/sim/debug" as Debug
import edu.pepp 1.0
import com.kdab.dockwidgets 2 as KDDW

FocusScope {
    id: wrapper
    required property var project
    required property var actions
    required property string mode
    // WASM version's active focus is broken with docks.
    required property bool isActive
    property bool needsDock: true
    focus: true
    NuAppSettings {
        id: settings
    }
    onModeChanged: modeVisibilityChange()

    function modeVisibilityChange() {
        // Don't allow triggering before initial docking, otherwise the layout can be 1) slow and 2) wrong.
        if (needsDock) {
            return;
        } else if (!(mode === "editor" || mode === "debugger")) {
            return;
        }
        // visibility model preserves user changes within a mode.
        for (const x of visibilityBar.model) {
            const visible = x.visibility[mode];
            if (visible && !x.isOpen)
                x.open();
            else if (!visible && x.isOpen)
                x.close();
        }
    }

    // Call when the height, width have been finalized.
    // Otherwise, we attempt to layout when height/width == 0, and all our requests are ignored.
    function dock() {
        const StartHidden = 1;
        const PreserveCurrent = 2;
        const total_height = parent.height - visibilityBar.height;
        const reg_height = registers.childrenRect.height;
        const regmemcol_width = registers.implicitWidth;
        const memdump_height = total_height - registers.implicitHeight;

        const bottom_height = Math.max(200, total_height * .1);
        const io_width = Math.max(300, parent.width * .2);
        const editor_height = (total_height - bottom_height) / 2;
        const editor_width = parent.width - regmemcol_width - io_width;
        // Dock text
        dockWidgetArea.addDockWidget(dock_source, KDDW.KDDockWidgets.Location_OnLeft, dockWidgetArea, Qt.size(editor_width, editor_height));
        dockWidgetArea.addDockWidget(dock_listing, KDDW.KDDockWidgets.Location_OnBottom, dock_source, Qt.size(editor_width, editor_height));
        // Dock IOs to right of editors
        dockWidgetArea.addDockWidget(dock_input, KDDW.KDDockWidgets.Location_OnRight, dockWidgetArea, Qt.size(io_width, editor_height));
        dockWidgetArea.addDockWidget(dock_output, KDDW.KDDockWidgets.Location_OnBottom, dock_input, Qt.size(io_width, editor_height));
        // Dock "helpers" below everything
        dockWidgetArea.addDockWidget(dock_object, KDDW.KDDockWidgets.Location_OnBottom, null, Qt.size(editor_width, bottom_height));
        dock_object.addDockWidgetAsTab(dock_symbol, PreserveCurrent);
        dock_object.addDockWidgetAsTab(dock_watch, PreserveCurrent);
        dock_object.addDockWidgetAsTab(dock_breakpoints, PreserveCurrent);

        // Setup memory area
        dockWidgetArea.addDockWidget(dock_cpu, KDDW.KDDockWidgets.Location_OnRight, null, Qt.size(regmemcol_width, reg_height));
        dockWidgetArea.addDockWidget(dock_hexdump, KDDW.KDDockWidgets.Location_OnBottom, dock_cpu, Qt.size(regmemcol_width, memdump_height));
        dock_hexdump.addDockWidgetAsTab(dock_stack, PreserveCurrent);
        wrapper.needsDock = Qt.binding(() => false);
        modeVisibilityChange();
        // WASM version doesn't seem to give focus to editor without giving focus to something else first.
        // Without this workaround the text editor will not receive focus on subsequent key presses.
        if (PlatformDetector.isWASM)
            batchInput.forceFocusEditor();
        // Delay giving focus to editor until the next frame to ensure that ou
        Qt.callLater(() => userAsmEdit.forceEditorFocus());
    }
    Component.onCompleted: {
        // Must connect and disconnect manually, otherwise project may be changed underneath us, and "save" targets wrong project.
        // Do not need to update on mode change, since mode change implies loss of focus of objEdit.
        userAsmEdit.editingFinished.connect(save);
        osAsmEdit.editingFinished.connect(save);
        project.errorsChanged.connect(displayErrors);
        project.listingChanged.connect(fixListings);
        onProjectChanged.connect(fixListings);
        project.charInChanged.connect(() => batchInput.setInput(project.charIn));
        // Connect editor BPs to project
        userAsmEdit.editor.modifyLine.connect(project.onModifyUserSource);
        osAsmEdit.editor.modifyLine.connect(project.onModifyOSSource);
        userList.editor.modifyLine.connect(project.onModifyUserList);
        osList.editor.modifyLine.connect(project.onModifyOSList);
        // Connect project to editors
        project.overwriteEditors.connect(onOverwriteEditors);
        project.modifyUserSource.connect(userAsmEdit.editor.onLineAction);
        project.modifyOSSource.connect(osAsmEdit.editor.onLineAction);
        project.modifyUserList.connect(userList.editor.onLineAction);
        project.modifyOSList.connect(osList.editor.onLineAction);
        // Update breakpoints on switch
        project.projectBreakpointsCleared.connect(userList.editor.onClearAllBreakpoints);
        project.projectBreakpointsCleared.connect(osList.editor.onClearAllBreakpoints);
        project.projectBreakpointsCleared.connect(userAsmEdit.editor.onClearAllBreakpoints);
        project.projectBreakpointsCleared.connect(osAsmEdit.editor.onClearAllBreakpoints);
        project.clearListingBreakpoints.connect(userList.editor.onClearAllBreakpoints);
        project.clearListingBreakpoints.connect(osList.editor.onClearAllBreakpoints);
        project.requestSourceBreakpoints.connect(userAsmEdit.editor.onRequestAllBreakpoints);
        project.requestSourceBreakpoints.connect(osAsmEdit.editor.onRequestAllBreakpoints);
        project.switchTo.connect(wrapper.onSwitchTo);
        if (project)
            fixListings();
        onOverwriteEditors();
        project.updateGUI.connect(watchExpr.updateGUI);
        project.updateGUI.connect(bpViewer.updateGUI);
        project.markedClean.connect(wrapper.markClean);
        userAsmEdit.onDirtiedChanged.connect(wrapper.markDirty);
    }

    signal requestModeSwitchTo(string mode)
    // Must be called when the project in the model is marked non-dirty
    function markClean() {
        userAsmEdit.dirtied = Qt.binding(() => false);
    }
    function markDirty() {
        if (userAsmEdit.dirtied)
            project.markDirty();
    }

    function getLexerLangauge() {
        switch (project?.architecture) {
        case Architecture.PEP9:
            return "Pep/9 ASM";
        case Architecture.PEP10:
            return "Pep/10 ASM";
        default:
            return "Pep/10 ASM";
        }
    }

    function onSwitchTo(os) {
        sourceSelector.currentIndex = Qt.binding(() => os ? 1 : 0);
        listingSelector.currentIndex = Qt.binding(() => os ? 1 : 0);
    }

    function displayErrors() {
        userAsmEdit.addEOLAnnotations(project.assemblerErrors);
    }
    function fixListings() {
        if (!project)
            return;
        if (userList) {
            const curURO = userList.readOnly;
            userList.readOnly = false;
            userList.text = project.userList ?? "";
            userList.readOnly = curURO;
        }
        if (osList) {
            const curORO = osList.readOnly;
            osList.readOnly = false;
            osList.text = project.osList ?? "";
            osList.readOnly = curORO;
        }
    }
    function syncEditors() {
        save();
    }
    function save() {
        // Supress saving messages when there is no project.
        if (project) {
            if (!userAsmEdit.readOnly) {
                project.userAsmText = userAsmEdit.text;
            }
            if (!osAsmEdit.readOnly) {
                project.osAsmText = osAsmEdit.text;
            }
        }
    }
    function onOverwriteEditors() {
        osAsmEdit.readOnly = false;
        userAsmEdit.text = project?.userAsmText ?? "";
        osAsmEdit.text = project?.osAsmText ?? "";
        osAsmEdit.readOnly = Qt.binding(() => !project?.abstraction === Abstraction.OS4);
    }

    FontMetrics {
        id: editorFM
        font: settings.extPalette.baseMono.font
    }

    KDDW.DockingArea {
        id: dockWidgetArea
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: visibilityBar.top
        }
        // Need application-wide unique ID, otherwise opening a new project will confuse the global name resolution algorithm.
        // TODO: Not gauranteed to be unique, but should be good enough for our purposes.
        uniqueName: Math.ceil(Math.random() * 1000000000).toString(16)
        KDDW.DockWidget {
            id: dock_source
            title: "Source Editor"
            uniqueName: `SourceEditor-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": false
            }
            ColumnLayout {
                anchors.fill: parent
                ComboBox {
                    id: sourceSelector
                    model: ["User", "OS"]
                    Layout.fillWidth: true
                    onActivated: function (new_index) {
                        listingSelector.currentIndex = Qt.binding(() => new_index);
                    }
                }
                StackLayout {
                    currentIndex: sourceSelector.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Text.ScintillaAsmEdit {
                        id: userAsmEdit
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        editorFont: editorFM.font
                        language: wrapper.getLexerLangauge()
                        focus: mode === "editor" && sourceSelector.currentIndex === -1
                    }
                    Text.ScintillaAsmEdit {
                        id: osAsmEdit
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        editorFont: editorFM.font
                        language: wrapper.getLexerLangauge()
                        focus: mode === "editor" && sourceSelector.currentIndex === 1
                    }
                }
            }
        }
        KDDW.DockWidget {
            id: dock_listing
            title: "Listing"
            uniqueName: `Listing-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            ColumnLayout {
                anchors.fill: parent
                ComboBox {
                    id: listingSelector
                    model: ["User", "OS"]
                    Layout.fillWidth: true
                    visible: !sourceSelector.visible
                    onActivated: function (new_index) {
                        sourceSelector.currentIndex = Qt.binding(() => new_index);
                    }
                }
                StackLayout {
                    currentIndex: sourceSelector.currentIndex

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Text.ScintillaAsmEdit {
                        id: userList
                        readOnly: true
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        text: project?.userList ?? ""
                        editorFont: editorFM.font
                        language: wrapper.getLexerLangauge()
                        focus: mode === "debugger" && sourceSelector.currentIndex === 0
                        caretBlink: 0
                    }
                    Text.ScintillaAsmEdit {
                        id: osList
                        readOnly: true
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        height: parent.height
                        text: project?.osList ?? ""
                        editorFont: editorFM.font
                        language: wrapper.getLexerLangauge()
                        focus: mode === "debugger" && sourceSelector.currentIndex === 1
                        caretBlink: 0
                    }
                }
            }
        }
        KDDW.DockWidget {
            id: dock_object
            title: "Object Code"
            uniqueName: `ObjectCode-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            Text.ObjTextEditor {
                id: objEdit
                anchors.fill: parent
                readOnly: true
                // text is only an initial binding, the value diverges from there.
                text: project?.objectCodeText ?? ""
            }
        }
        KDDW.DockWidget {
            id: dock_symbol
            title: qsTr(`Symbol Table: ${sourceSelector.currentText}`)
            uniqueName: `SymbolTable-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            SymTab.SymbolViewer {
                id: symTab
                anchors.fill: parent
                model: project?.staticSymbolModel ?? null
                scopeFilter: sourceSelector.currentIndex === 0 ? "usr.symtab" : "os.symtab"
            }
        }
        KDDW.DockWidget {
            id: dock_watch
            title: qsTr(`Watch Expressions`)
            uniqueName: `WatchExpressions-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            Debug.WatchExpressions {
                id: watchExpr
                anchors.fill: parent
                watchExpressions: project?.watchExpressions ?? null
            }
        }
        KDDW.DockWidget {
            id: dock_breakpoints
            title: qsTr(`Breakpoint Viewer`)
            uniqueName: `BreakpointViewer-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            BreakpointViewer {
                id: bpViewer
                anchors.fill: parent
                model: project?.breakpointModel ?? null
                lineInfo: project?.lines2addr ?? null
            }
        }
        KDDW.DockWidget {
            id: dock_input
            title: "Batch Input"
            uniqueName: `BatchInput-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": true,
                "debugger": true
            }
            IO.Batch {
                id: batchInput
                anchors.fill: parent
                property bool ignoreTextChange: false
                Component.onCompleted: {
                    onTextChanged.connect(() => {
                        if (!ignoreTextChange)
                            project.charIn = text;
                    });
                }
                function setInput(input) {
                    ignoreTextChange = true;
                    batchInput.text = input;
                    ignoreTextChange = false;
                }
            }
        }
        KDDW.DockWidget {
            id: dock_output
            title: "Batch Output"
            uniqueName: `BatchOutput-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            IO.Batch {
                id: batchOutput
                anchors.fill: parent
                text: project?.charOut ?? ""
                readOnly: true
            }
        }
        KDDW.DockWidget {
            id: dock_cpu
            title: "CPU Dump"
            uniqueName: `RegisterDump-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            ColumnLayout {
                anchors.fill: parent
                property size kddockwidgets_min_size: Qt.size(registers.implicitWidth, registers.implicitHeight)
                Cpu.RegisterView {
                    id: registers
                    Layout.fillWidth: false
                    Layout.alignment: Qt.AlignHCenter

                    registers: project?.registers ?? null
                    flags: project?.flags ?? null
                }
            }
        }
        KDDW.DockWidget {
            id: dock_hexdump
            title: "Memory Dump"
            uniqueName: `MemoryDump-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            Loader {
                id: loader
                anchors.fill: parent
                Component.onCompleted: {
                    const props = {
                        "memory": project.memory,
                        "mnemonics": project.mnemonics
                    };
                    // Construction sets current address to 0, which propogates back to project.
                    // Must reject changes in current address until component is fully rendered.
                    con.enabled = false;
                    setSource("qrc:/qt/qml/edu/pepp/memory/hexdump/MemoryDump.qml", props);
                }
                asynchronous: true
                onLoaded: {
                    loader.item.scrollToAddress(project.currentAddress);
                    con.enabled = true;
                }
                Connections {
                    id: con
                    enabled: false
                    target: loader.item
                    function onCurrentAddressChanged() {
                        project.currentAddress = loader.item.currentAddress;
                    }
                }
            }
        }
        KDDW.DockWidget {
            id: dock_stack
            title: "Stack Trace"
            uniqueName: `StackTrace-${dockWidgetArea.uniqueName}`
            property var visibility: {
                "editor": false,
                "debugger": true
            }
            Stack.StackTrace {
                anchors.fill: parent
            }
        }
    }
    //  Button list of available views
    ListView {
        id: visibilityBar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            leftMargin: 5
        }
        height: 30
        spacing: 5
        orientation: Qt.Horizontal
        model: [dock_source, dock_listing, dock_object, dock_symbol, dock_watch, dock_breakpoints, dock_input, dock_output, dock_cpu, dock_stack, dock_hexdump]
        delegate: Button {
            id: button
            Layout.alignment: Qt.AlignVCenter
            checkable: true
            down: modelData.isOpen
            display: AbstractButton.TextOnly

            contentItem: Label {
                text: modelData.title
                //  Highlight color used for button down
                color: button.down ? palette.highlightedText : palette.buttonText;
            }

            background: Rectangle {
                //  Highlight color used for button down
                color: button.down ? palette.highlight : palette.button
                opacity: .25
                border.width: 1
                //  Highlight color used for button down
                border.color: button.hovered ? palette.highlight : "transparent"
                radius: 5
            }

            onClicked: {
                //  Flip current state and update model and display
                const nextMode = !modelData.visibility[wrapper.mode];
                modelData.visibility[wrapper.mode] = nextMode;
                nextMode ? modelData.open() : modelData.close();
            }
        }   //  delegate: Button
    }   //  ListView

    // Only enable binding from the actions to this project if this project is focused.
    Connections {
        enabled: wrapper.activeFocus || wrapper.isActive
        target: wrapper.actions.debug.start
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
    Connections {
        enabled: wrapper.activeFocus || wrapper.isActive
        target: wrapper.actions.build.execute
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
}
