pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import edu.pepp 1.0

ColumnLayout {

    Label {
        textFormat: Text.RichText
        onLinkActivated: link => Qt.openUrlExternally(link)
        text: {
            const url = "https://github.com/Matthew-McRaven/Pepp/issues";
            return `Report any issues to our <a href="${url}">issue tracker</a><br/>` + "Please include a copy of the diagnostic information on this page.";
        }
    }
    TextArea {
        id: area
        Layout.fillWidth: true
        textFormat: TextEdit.RichText
        onLinkActivated: link => {
            Qt.openUrlExternally(link);
        }
        Component.onCompleted: {
            const hd2_url = "https://github.com/Matthew-McRaven/Pepp/commit/" + Version.git_sha;
            const hd2 = `Pepp build: <a href=\"${hd2_url}\">${Version.git_describe_short}</a>`;
            const hd3 = `Qt Version: ${Version.qt_version},debug=${Version.qt_debug},shared=${Version.qt_shared}`;
            const app_ver = [hd2, hd3];

            // Details of the target machine on which this applicaiton is running
            const tgt = [];
            tgt.push(`Machine OS: ${Version.target_platform}`);
            tgt.push(`Machine ABI: ${Version.target_abi}`);
            tgt.push(`Machine Graphics API: ${Version.target_graphics_api}`);
            tgt.push(`Machine Qt Platform: ${Version.target_qt_platform}`);

            // Details of the machine on which this application was built
            const build_1 = `Build date: ${Version.build_timestamp}`;
            const build_2 = `Build OS: ${Version.build_system}`;
            const build_3 = `Compiler ID: ${Version.cxx_compiler}`;
            const build = [build_1, build_2, build_3];

            const blocks = [...app_ver, ...tgt, ...build];
            area.text = blocks.join("<br/>");
            area.readOnly = true;
        }
    }
    Button {
        text: "Copy to Clipboard"
        onClicked: {
            // Rich text encodes <br/> as U+2028 in plain text.
            Version.copy_diagnostics_to_clipboard(area.getText(0, area.length).replace(/\u2028/g, "\n"));
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
