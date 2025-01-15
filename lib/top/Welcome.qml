import QtQuick 2.15
import QtQuick.Layouts
import edu.pepp 1.0

StackLayout {
    id: root
    signal addProject(string arch, string abstraction, string features, bool reuse)

    currentIndex: 1

    WelcomeArch {
        id: architectures
        Connections {
            target: architectures
            function onGoForward() {
                root.currentIndex = 1
            }
        }
    }

    WelcomeProject {
        id: projects
        Connections {
            target: projects
            function onGoBack() {
                root.currentIndex = 0
            }
            function onAddProject(arch, abs, feats, reuse) {
                root.addProject(arch, abs, feats, reuse)
            }
        }
    }
}
