import QtQuick 2.2

Item {
	property var param

	visible: !!param && param.visible
	enabled: !!param && param.enabled
	width: parent.width
	height: Theme.paramHeight

	Text {
		id: title
		text: param ? param.title : ""
		anchors.verticalCenter: parent.verticalCenter
		font: Theme.paramFont
		color: enabled ? Theme.paramTitleEnabled : Theme.paramTitleDisabled
	}

	ParamInput {
		id: valueInput
		text: param ? param.value : ""

		x: parent.width - width
		width: parent.width - title.contentWidth - 32

		horizontalAlignment: Text.AlignLeft

		function setValue(value) {
			if (param) param.value = value
		}

		Rectangle {
			x: 0
			anchors.fill: parent
			color: "transparent"
			border.color: "#eee"
			radius: 4
		}
	}
}
