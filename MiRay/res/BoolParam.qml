import QtQuick 2.2
import QtQuick.Controls 1.4

Item {
	property var param
	property bool group: false

	visible: !!param && param.visible
	enabled: !!param && param.enabled
	width: parent.width
	height: group ? Theme.groupTitleHeight : Theme.boolParamHeight //Theme.paramHeight

	// color: "#dfd"

//	Rectangle {
//		x: 0
//		anchors.fill: parent
//		color: "transparent"
//		border.color: "#000000"
//	}

	MouseArea {
		id: mouseArea
		anchors.fill: parent
		onClicked: { param.value = !param.value; parent.focus = true; }
		hoverEnabled: true
		cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
	}

	Text {
		id: title
		text: param ? param.title : ""
		//x: group ? 34 : 0
		anchors.verticalCenter: parent.verticalCenter
		font: group ? Theme.groupTitleFont : Theme.paramFont
		color: enabled ? (group ? "#000" : Theme.paramTitleEnabled) : Theme.paramTitleDisabled
	}

	Rectangle {
		property bool checked: !!param && param.value
		color: enabled ? (checked ? Theme.mainColor :"#aaa") : "#ddd"
		width: height * 2 - 4
		height: 16 //group ? 20 : 16
		radius: height * 0.5
		//border.color: checked ? "transparent" : "#888"
		//x: group ? 0 : parent.width - width
		x: parent.width - width
		anchors.verticalCenter: parent.verticalCenter
		focus: true

		Rectangle {
			y: 2
			x: parent.checked ? parent.width * 0.5 : 2
			width: height
			height: parent.height - 4
			radius: height * 0.5
			color: "#fff"
		}
	}
}
