import QtQuick 2.15

Rectangle {
	property bool checked: false
	property bool inverted: false

	width: 18
	height: 18
	radius: 4
	anchors.verticalCenter: parent.verticalCenter

	color: {
		if (!enabled) {
			return "#E1E2E5"
		}

		if (inverted) {
			return !mouseArea.containsPress ? "#FFFFFF" : "#E8E8E8"
		}

		if (checked) {
			return !mouseArea.containsPress ? Theme.mainColor : Theme.mainColorPressed
		}

		return !mouseArea.containsPress ? "#ECEEF2" : "#E0E2E4"
	}

	Image {
		visible: checked
		anchors.centerIn: parent
		source: inverted ? "../checkbox-check-green.png" : "../checkbox-check-white.png"
	}

	MouseArea {
		id: mouseArea
		anchors.fill: parent
		onClicked: parent.toggled()
	}

	signal toggled()
}
