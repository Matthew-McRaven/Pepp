import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
  id: window
  width: 640
  height: 480
  visible: true
  title: qsTr("Pep/10 Help")

  //! [orientation]
  //  Used to make figure treeview appear and disappear based on orientation.
  //  Primarily used in devices like an iPad which can be flipped.
  readonly property bool inPortrait: window.width < window.height
  //! [orientation]

  Component.onCompleted: {
    //drawer.onSelectedChanged.connect(
    //      (arg) => {console.log (drawer.selected.display)}
    //      )
    drawer.onSelectedChanged.connect(
      (arg) => {

        if( drawer.selected === undefined) {
          mainWindow.source = "Topic.qml"
        } else {
          mainWindow.source = ""
          mainWindow.source = "Figure.qml"
          //figCol.help( drawer.selected.display,drawer.selected.payload );
        }
      }
    )
  }

  Rectangle {
    id: mainLoader
    anchors.fill: parent
    //function hi(){console.log("Hi")}
    //  Left pane with selections
    Drawer {
      id: drawer

      y: 0
      width: 200
      height: window.height
      property var selected: undefined

      modal: inPortrait
      interactive: inPortrait
      position: inPortrait ? 0 : 1
      visible: !inPortrait
      background: Rectangle { color: "#ffffff" }

      /*Tree {  //  TreeView fails to initialize in seperate file
        id: chapterTree
        visible: true;
        anchors.fill: parent
      }*/

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
            onTapped: {
              if(treeDelegate.hasChildren) {
                drawer.selected = undefined
                treeView.toggleExpanded(row)
              } else {
                drawer.selected = {
                  //kind,payload
                  display, payload
                }
                let d = drawer.selected
                console.log(drawer.selected)
              }
            }
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
                return "Figure " + model.display
                //console.log(Object.keys(x));
              } else if(model.kind === "book") {
                let text = ""
                for(let fig of model.payload) {
                  //console.log(fig.isOS)
                }
              }
              return model.display// + " ---- " + model.kind;
            }
          }
        }

        /*background: Rectangle {
          anchors.fill: parent;
          color: "blue"
        }*/


        ScrollIndicator.vertical: ScrollIndicator { }
      } //  TreeView
    } //  Drawer

    Flickable {
      id: flickable
      anchors.fill: parent
      anchors.topMargin: 0
      anchors.leftMargin: !inPortrait ? drawer.width : undefined

      topMargin: 10
      bottomMargin: 10

      //  Moved logic to separate qml control files
      Loader {
        id: mainWindow
        anchors.fill: parent
        source: "Topic.qml"
      }

      ScrollIndicator.vertical: ScrollIndicator { }
    } //  Flickable
  } //Loader
}
