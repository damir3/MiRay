pragma Singleton

import QtQuick 2.2
import QtQuick.Dialogs 1.3

ColorDialog {
	property var param

	color: param ? param.value : ""
	onAccepted: param.value = this.color
}
