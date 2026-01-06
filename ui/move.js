.import QtQuick as Quick

/* This script file handles the element movement logic */
var majorX = 75;
var majorY = 75;
var minorX = 5;
var minorY = 5;
var component = null;

//  Create new diagram
function createBlock(parent, x, y) {
    //  Cache component for creating diagrams
    if (component === null)
        component = Qt.createComponent("Diagram.qml");

    //  Create instance of a diagram and place at indicated x,y coordinate
    var diagram;
    if (component.status === Quick.Component.Ready) {
        diagram = component.createObject(parent);
        if (diagram === null) {
            console.log("Error creating diagram");
            console.log(component.errorString());
            return null;
        }
        diagram.text = parent.curName;
        diagram.file = parent.curFile;
        moveObjectTo(diagram, x, y);
        //diagram.width = majorX;
        //diagram.height = majorY;
    } else {
        console.log("Error loading individual diagram");
        console.log(component.errorString());
    }
    return diagram;
}

//  Move object along a grid pattern
function moveObjectTo(obj, x, y) {
    if (obj === null)
        return;
    const row = Math.floor(x / majorX) * majorX;
    const col = Math.floor(y / majorY) * majorY;
    obj.x = row;
    obj.y = col;
}

function lineX(from, to) {
    var left = from.x;
    const width = to.x - from.x;
    console.log("left", left, "width", width, "from.x", from.x);

    //  If width is negative, To object is right of From. Use x coordinate
    if (width > 0) {
        if (width > from.width) {
            //  To object is not aigned with From object. Connect to right
            left += from.width;
            //console.log("right");
        } else {
            // Objects are vertically aligned. Connect to center
            left += from.width / 2;
            //console.log("center");
        }
    }
    //else
    //console.log("left");

    return left;
}

function lineY(from, to) {
    var top = from.y;
    const height = to.y - from.y;
    console.log("top", top, "height", height, "from.height", from.height);

    //if (Math.abs(height) > from.height) {
    //  Objects are vertically aligned. Connect
    top += from.height / 2;
    /*} else {
        // Objects are vertically aligned. Connect to center
        //        top += (height > 0) ? from.height : -from.height;
    }*/

    return top;
}
