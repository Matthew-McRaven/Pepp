import QtQuick 2.15

Text {
    TextMetrics {
        id: titleMetrics
        font: title.font
        text: title.text
    }
    id: title
    color: palette.text
    font.bold: true

    // Ensure that wordwrap does not extend outside of the card.
    wrapMode: Text.WordWrap
    width: parent.width

    // Align text to bottom left corner like Qt Creator's "Welcome" mode cards.
    // height: titleMetrics.height * 2 + padding
    // padding: 5
    // topPadding: 0
}
