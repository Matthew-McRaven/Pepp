import QtQuick
import QtQuick.Controls

ComboBox {
    id: control

    //  Change background color to disabled
    flat: !control.enabled

    //  Hide drop down indicator (sent 0 to .3 to appear disabled)
    indicator.opacity: control.enabled ? 1 : 0
}
