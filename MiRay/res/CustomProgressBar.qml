import QtQuick 2.15

Rectangle {
	property real value: 0
	property bool showPercentage: true

	color: "#DDDFE3"
	radius: 30
	height: 16

	Rectangle {
		id: indicator
		anchors.verticalCenter: parent.verticalCenter
		width: parent.width * value
		height: Math.min(parent.height, width)
		radius: 30
		color: Theme.mainColor
	}

	Text {
		visible: showPercentage
		anchors.centerIn: parent
		font.family: "Verdana"
		font.pixelSize: 12
		color: "#000"
		text: (value * 100).toFixed(1) + "%"
	}
}
