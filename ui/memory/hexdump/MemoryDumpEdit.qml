import QtQuick

Item {
  id: root
  property alias colWidth: root.implicitWidth
  property alias rowHeight: root.implicitHeight

  property alias backgroundColor: cell.color
  property alias textColor: editor.color

  property alias text: editor.text
  property alias textAlign: editor.horizontalAlignment
  property alias font: editor.font
  property alias editFocus: editor.focus

  //  Need access to tableView for cursor movement
  required property var parentTable

  //  Signals for tableview
  signal startEditing()             //  Indicates start of edit mode
  signal finishEditing(save: bool)  //  Indicates end of edit mode
  signal directionKey(key: int)     //  Indicates that user overflowed control
                                    //  Move to next control. Key contains direction

  implicitHeight: editor.implicitHeight
  implicitWidth: editor.implicitWidth

  Rectangle {
    id: cell
    anchors.fill: root
    focus: false

    TextInput {
      id: editor

      anchors.fill: cell
      padding: 0
      z: 1

      maximumLength: 2
      verticalAlignment: TextInput.AlignVCenter
      overwriteMode: false
      echoMode: TextInput.Normal
      focus: true
      selectionColor: textColor
      selectedTextColor: backgroundColor
      color: "gray" //  Used for testing. Set by model

      //  Cursor background is white by default. Use TextInput background color
      cursorDelegate: Rectangle {
        visible: editor.cursorVisible
        color: textColor
        width: editor.cursorRectangle.width

        //  Do not block TextInput with cursor
        z: -1
      }

      //  Limit input to valid hexidecimal values
      validator: RegularExpressionValidator { regularExpression: /[0-9,A-F,a-f]{2}/}

      //  Leave edit mode without saving
      Keys.onEscapePressed: {
        //  Do not save result
        root.finishEditing(false)
      }

      //  Performed when control is finished being created
      Component.onCompleted: {
        //  Highlight first character for editing
        editor.select(0,1)

        //  Notifiy parent that editing has started
        root.startEditing()

        focus=true
      }

      onActiveFocusChanged: {
        //  TableView is grabbing focus. Grab back
        //  Note, setting focus above does not prevent TableView
        //  from taking focus at start up.
        if(!focus) {
          forceActiveFocus()
        }

        //  Highlight first character for editing
        editor.select(0,1)
      }

      //  User accepts changes
      onAccepted: {
        if( tableView.model.byteRole !== text) {
          console.log("onAccept=Updated")
          //  Signal parent control that we are done
          //  Parent will save if value is different
          root.finishEditing(true)
        }
      }

      Component.onDestruction: {
        //  Do not save. Used to ensure highlight
        //  color is cleared if user hit Escape.
        root.finishEditing(false)
      }
      //  This function is only activated if valid number or character
      //  is clicked based on validator. Arrow keys and other
      //  characters are ignored.
      onTextEdited: {
        console.log("onTextEdited")
        if( editor.cursorPosition < 2) {
          highlight()
        }
        else {
          //  After update, save results and move to next cell
          accepted()

          //  Raise event to parent to handle keystroke
          root.directionKey(Qt.Key_Right)
          console.log("onTextEdited->Key_Right")
        }
      }

      function highlight() {
        //  We just edited first character, highlight second character
          editor.select(editor.cursorPosition, editor.cursorPosition+1)
          console.log("highlight 0: pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)
      }

      //  Handle key movement inside edit control
      Keys.onPressed: (event) => {
        //  Key events that we track at TableView
        const key = event.key
        let isMoving = false

        switch(key) {
        case Qt.Key_Left:
          if(editor.selectionStart === 0) {
            //  User moved past beginning of control
            //  Save result and tell tableView to move to next cell
            isMoving = true
          }
          else {
            //  Move cursor to left in edit control
            editor.cursorPosition = editor.cursorPosition - 1
          }
          break
        case Qt.Key_Right:
          //  Move cursor to right in edit control
          if(editor.cursorPosition === 2) {
            //console.log("Right Key-Accept="+editor.cursorPosition)
            isMoving = true
            console.log("onPressed Key_Right isMoving")
          }
          break
        case Qt.Key_Home:
        case Qt.Key_End:
          //  Override child control. Home and End will move to front
          //  and back of textinput. These should be treated as movements
          //  within TableView. Let parent handle.
          isMoving = true
          break
        default:
          //  Let parent handle
          event.accepted = false
          return
        }

        //  Handle key movement
        if(isMoving){
          console.log("editor.keys=" + isMoving)
          //  User moved past end or beginning of control
          //  Save result and tell tableView to move to next cell
          editor.accepted()

          //  Raise event to parent to handle keystroke
          root.directionKey(key)
        }
        else {
          //  Movement is within edit control, signal that control will handle
          console.log("editor.keys (before): pos,start,end=" + editor.cursorPosition + "," +
                      editor.selectionStart + "," + editor.selectionEnd)

          //  Highling will highlight next item in control
          highlight()
        }

        //  Signal parent that the event was handled
        event.accepted = true
      }
    }
  }
}
