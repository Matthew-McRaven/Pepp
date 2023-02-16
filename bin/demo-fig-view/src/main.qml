import QtQuick

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    // Derived from: https://doc.qt.io/qt-6/qml-qtquick-treeview.html
    TreeView {
      id: treeView
      anchors.fill: parent
      model: global_model
      delegate: Item {
        id: treeDelegate
        implicitWidth: padding + label.x + label.implicitWidth + padding
        implicitHeight: label.implicitHeight * 1.5
        readonly property real indent: 10
        readonly property real padding: 5
        // Assigned to by TreeView:
        required property TreeView treeView
        required property bool isTreeNode
        required property bool expanded
        required property int hasChildren
        required property int depth
        required property var payload
        required property var kind
        required property var display

        TapHandler {
            onTapped: treeView.toggleExpanded(row)
        }

        Text {
            id: indicator
            visible: treeDelegate.isTreeNode && treeDelegate.hasChildren
            x: padding + (treeDelegate.depth * treeDelegate.indent)
            anchors.verticalCenter: label.verticalCenter
            text: "▸"
            rotation: treeDelegate.expanded ? 90 : 0
        }

        Text {
            id: label
            x: padding + (treeDelegate.isTreeNode ? (treeDelegate.depth + 1) * treeDelegate.indent : 0)
            width: treeDelegate.width - treeDelegate.padding - x
            clip: true
            text: {
              if(model.kind === "figure") {
                let x = model.payload.elements;
                /*console.log(Object.keys(x));*/
              } else if(model.kind === "book") {
                for(let fig of model.payload) {
                  /*console.log(fig.isOS)*/
                }
              }

              return model.display + " ---- " + model.kind;
            }
        }
      }
    }
}
