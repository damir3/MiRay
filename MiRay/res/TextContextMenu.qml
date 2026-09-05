import QtQuick 2.15

MouseArea {
	property Item target

	anchors.fill: parent

	preventStealing: true
	acceptedButtons: Qt.RightButton

	onPressed: {
		target.forceActiveFocus()
	}

	onReleased: {
		TextEditMenu.parent = target
		TextEditMenu.target = target
		TextEditMenu.popup(mouse.x, mouse.y, target)
	}
}
