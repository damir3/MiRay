import QtQuick 2.12
import QtQuick.Controls 2.12

ToolTip {
	id: control

	delay: 1000
	timeout: 10000
	padding: 3

	contentItem: Text {
		text: control.text
		font.pixelSize: Theme.tooltipTextSize
		wrapMode: Text.WrapAnywhere
	}

	background: Rectangle {
		border.color: "#bbb"
		color: "#eee"
	}
}
