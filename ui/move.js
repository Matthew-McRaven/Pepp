.import QtQuick as Quick

/* This script file handles the element movement logic */
var majorX = 75;
var majorY = 75;
var minorX = 10;
var minorY = 5;

function moveObjectTo(obj, x, y) {
    if (obj === null)
        return;
    const row = Math.floor(x / majorX) * majorX;
    const col = Math.floor(y / majorY) * majorY;
    obj.x = row;
    obj.y = col;
}
