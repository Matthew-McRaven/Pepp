pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import edu.pepp 1.0

ColumnLayout {

    Text {

        onLinkActivated: link => {
            Qt.openUrlExternally(link);
        }
        Component.onCompleted: {
            const url = "https://github.com/Matthew-McRaven/Pepp/issues";
            const l1 = `Report any issues to our <a href=\"${url}\">issue tracker</a>`;
            const l2 = "Please include a copy of the diagnostic information on this page.";

            text = `${l1}<br/>${l2}`;
        }
    }

    Item {
        implicitHeight: 30
    }

    Text {
        onLinkActivated: link => {
            Qt.openUrlExternally(link);
        }
        Component.onCompleted: {
            let url = "https://github.com/Matthew-McRaven/Pepp/commit/" + Version.git_sha;
            text = `Pepp build: <a href=\"${url}\">${Version.git_sha.substring(0, 7)}</a>`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Qt Version: ${Version.qt_version},debug=${Version.qt_debug},shared=${Version.qt_shared}`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Machine OS: ${Version.target_platform}`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Machine ABI: ${Version.target_abi}`;
        }
    }

    Text {
        Component.onCompleted: {
            text = `Build date: ${Version.build_timestamp}`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Build OS: ${Version.build_system}`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Compiler ID: ${Version.cxx_compiler}`;
        }
    }
    Item {
        implicitHeight: 30
    }
    Button {
        text: "Copy to Clipboard"
        onClicked: {
            Version.copy_diagnostics_to_clipboard();
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
