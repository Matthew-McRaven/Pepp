pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import edu.pepp 1.0

ColumnLayout {

    TextArea {
        id: area
        textFormat: TextEdit.RichText
        onLinkActivated: link => {
            Qt.openUrlExternally(link)
        }
        Component.onCompleted: {
            const b1_url = "https://github.com/Matthew-McRaven/Pepp/issues"
            const b1_l1 = `Report any issues to our <a href=\"${b1_url}\">issue tracker</a>`
            const b1_l2 = "Please include a copy of the diagnostic information on this page."
            const b1 = `${b1_l1}<br/>${b1_l2}<br/>`
            const b2_url = "https://github.com/Matthew-McRaven/Pepp/commit/" + Version.git_sha
            const b2 = `Pepp build: <a href=\"${b2_url}\">${Version.git_sha.substring(
                0, 7)}</a>`
            const b3 = `Qt Version: ${Version.qt_version},debug=${Version.qt_debug},shared=${Version.qt_shared}`
            const b4 = `Machine OS: ${Version.target_platform}`
            const b5 = `Machine ABI: ${Version.target_abi}`
            const b6 = `Build date: ${Version.build_timestamp}`
            const b7 = `Build OS: ${Version.build_system}`
            const b8 = `Compiler ID: ${Version.cxx_compiler}`
            const blocks = [b1, b2, b3, b4, b5, b6, b7, b8]
            area.text = blocks.join("<br/>")
            area.readOnly = true
        }
    }
    Button {
        text: "Copy to Clipboard"
        onClicked: {
            Version.copy_diagnostics_to_clipboard()
        }
    }
    Item {
        Layout.fillHeight: true
    }

    // Still want to add the following fields at some point


    /*
     *   Window manager, to debug Wayland issues.
     */
}
