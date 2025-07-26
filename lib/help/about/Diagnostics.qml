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
            let url = "https://github.com/Matthew-McRaven/Pepp/commit/" + Version.git_sha;
            text = `Pepp build: <a href=\"${url}\">${Version.git_sha.substring(0, 7)}</a>`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Built on: ${Version.build_timestamp}`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Build OS ID: ${Version.build_system}`;
        }
    }
    Text {
        Component.onCompleted: {
            text = `Compiler ID: ${Version.cxx_compiler}`;
        }
    }
    Item {
        Layout.fillHeight: true
    }

    // Want to know
    // Link to our issue reporting thing. Add a template which tells you to submit a screenshot of this information.
    /* OS info;
     *   Name, version, architecture
     *   Window manager
     *
     * Build information:
     *   Qt build # with URL
     *   Qt debug/shared/static
     */
}
