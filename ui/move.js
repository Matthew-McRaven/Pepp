.import QtQuick as Quick

/* This script file handles the element movement logic */
var majorX = 75;
var majorY = 75;
var minorX = 5;
var minorY = 5;
var component = null;

function createBlock(parent, x, y) {
    //  Cache component for creating diagrams
    if (component === null)
        component = Qt.createComponent("Diagram.qml");

    // Note that if Block.qml was not a local file, component.status would be
    // Loading and we should wait for the component's statusChanged() signal to
    // know when the file is downloaded and ready before calling createObject().
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

function moveObjectTo(obj, x, y) {
    if (obj === null)
        return;
    const row = Math.floor(x / majorX) * majorX;
    const col = Math.floor(y / majorY) * majorY;
    obj.x = row;
    obj.y = col;
}
