import QtQuick 2.2

Item {
	property var param
	property var next
	property var valueValidator: RegExpValidator { regExp : /^-?[0-9]{1,10}$/ }

	visible: !!param && param.visible
	enabled: !!param && param.enabled
	width: parent.width
	height: Theme.paramHeight

	Text {
		text: param ? param.title : ""
		anchors.verticalCenter: parent.verticalCenter
		font: Theme.paramFont
		color: enabled ? Theme.paramTitleEnabled : Theme.paramTitleDisabled
	}

	ParamInput {
		id: valueInput
		text: param ? param.value : 0

		x: parent.width - 100
		width: 100

		validator: valueValidator
		function setValue(value) {
			if (param) param.value = parseInt(value)
		}

		/*Connections {
			target: param || null
			function onValueChanged() {
				valueInput.text = param.value
			}
		}*/
	}

//	Rectangle {
//		x: 0
//		anchors.fill: parent
//		color: "transparent"
//		border.color: "#000000"
//	}
}
