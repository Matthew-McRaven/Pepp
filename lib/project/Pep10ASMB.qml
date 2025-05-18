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
    property bool needsDock: true
    focus: true
    NuAppSettings {
        id: settings
    }

    // Call when the height, width have been finalized.
    // Otherwise, we attempt to layout when height/width == 0, and all our requests are ignored.
    function dock() {
        const StartHidden = 1;
        const PreserveCurrent = 2;

        const reg_height = registers.childrenRect.height;
        const regmemcol_width = registers.implicitWidth;
        const memdump_height = parent.height - registers.implicitHeight;

        const bottom_height = Math.max(200, parent.height * .1);
        const io_width = Math.max(300, parent.width * .2);
        const editor_height = (parent.height - bottom_height) / 2;
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
        userAsmEdit.forceActiveFocus();
    }

    signal requestModeSwitchTo(string mode)

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
            userList.addListingAnnotations(project.userListAnnotations);
            userList.readOnly = curURO;
        }
        if (osList) {
            const curORO = osList.readOnly;
            osList.readOnly = false;
            osList.text = project.osList ?? "";
            osList.addListingAnnotations(project.osListAnnotations);
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
        anchors.fill: parent
        // Need application-wide unique ID, otherwise opening a new project will confuse the global name resolution algorithm.
        // TODO: Not gauranteed to be unique, but should be good enough for our purposes.
        uniqueName: `${Math.ceil(Math.random() * 1_000_000_000).toString(16)}`
        KDDW.DockWidget {
            id: dock_source
            title: "Source Editor"
            uniqueName: `SourceEditor-${dockWidgetArea.uniqueName}`
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
            ColumnLayout {
                anchors.fill: parent
                ComboBox {
                    id: listingSelector
                    model: ["User", "OS"]
                    Layout.fillWidth: true
                    visible: !dock_source.visible
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
                    }
                }
            }
        }
        KDDW.DockWidget {
            id: dock_object
            title: "Object Code"
            uniqueName: `ObjectCode-${dockWidgetArea.uniqueName}`
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
            IO.Batch {
                id: batchOutput
                anchors.fill: parent
                text: project?.charOut ?? ""
                readOnly: true
            }
        }
        KDDW.DockWidget {
            id: dock_cpu
            title: "Register Dump"
            uniqueName: `RegisterDump-${dockWidgetArea.uniqueName}`
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
            Stack.StackTrace {
                anchors.fill: parent
            }
        }
    }

    // Only enable binding from the actions to this project if this project is focused.
    Connections {
        enabled: wrapper.activeFocus
        target: wrapper.actions.debug.start
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
    Connections {
        enabled: wrapper.activeFocus
        target: wrapper.actions.build.execute
        function onTriggered() {
            wrapper.requestModeSwitchTo("debugger");
        }
    }
}
