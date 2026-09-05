import QtQuick 2.2
import QtQuick.Controls 1.4

Item {
	property string text
	property color color: "#000"

	width: parent.width
	height: Theme.groupTitleHeight

	Item {
		width: parent.width
		height: Theme.paramHeight
		y: parent.height - height

		Text {
			anchors.fill: parent
			text: parent.parent.text
			color: parent.parent.color
			font: Theme.groupTitleFont
			verticalAlignment: Text.AlignVCenter
		}
	}
}
