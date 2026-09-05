import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
	property alias title: label.text
	property alias value: progressBar.value

	modal: true
	focus: false
	dim: true
	closePolicy: Popup.NoAutoClose

	width: 400
	height: 80
	padding: Theme.dialogPaddings
	anchors.centerIn: parent

	Overlay.modal: Rectangle { color: Theme.dimColor }

	background: Rectangle {
		id: backgroundImage

		color: "#fff"
		radius: 8
	}

	Column {
		width: parent.width
		anchors.centerIn: parent
		spacing: 10

		Text {
			id: label
			width: parent.width
			color: "#000"
			horizontalAlignment: Text.AlignHCenter
			font: Theme.labelFont
		}

		CustomProgressBar {
			id: progressBar
			width: parent.width
		}
	}
}
