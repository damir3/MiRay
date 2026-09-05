import QtQuick 2.2

Item {
	property var param
	property var units
	property var valueValidator: RegExpValidator { regExp : /[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?/ }

	visible: !!param && param.visible
	enabled: !!param && param.enabled
	width: parent.width
	height: Theme.paramHeight * 2 + 5

	property int dx: width / 2
	property int ox: 16

//	Rectangle {
//		x: 0
//		anchors.fill: parent
//		color: "transparent"
//		border.color: "#000000"
//	}

	Item {
		height: Theme.paramHeight
		Text {
			text: param ? param.title : ""
			anchors.verticalCenter: parent.verticalCenter
			font: Theme.paramFont
			color: enabled ? Theme.paramTitleEnabled : Theme.paramTitleDisabled
		}
	}

	Item {
		width: parent.width
		height: Theme.paramHeight

		Text {
			text: "X"
			color: enabled ? "#c44" : Theme.paramTitleDisabled
			x: dx
			anchors.verticalCenter: parent.verticalCenter
			font: Theme.paramFontBold
		}

		Text {
			id: unitsLabel
			visible: !!units
			text: units || ""
			x: parent.width - width
			width: 40
			horizontalAlignment: Text.AlignRight
			anchors.verticalCenter: parent.verticalCenter
			font: Theme.paramFont
			color: enabled ? Theme.unitsColorEnabled : Theme.unitsColorDisabled
			MouseArea {
				anchors.fill: parent
				onClicked: {
					xInput.selectAll()
					xInput.forceActiveFocus()
				}
			}
		}

		ParamInput {
			id: xInput
			text: param ? param.value.x : 0

			x: parent.width - 100
			width: 100 - (units ? unitsLabel.contentWidth : 0)

			validator: valueValidator
			function setValue(value) {
				set("x", value)
			}
		}
	}

	Item {
		y: Theme.paramHeight + 5
		width: parent.width
		height: Theme.paramHeight

		Text {
			text: "Y"
			color: enabled ? "#4c4" : Theme.paramTitleDisabled
			x: dx
			anchors.verticalCenter: parent.verticalCenter
			font: Theme.paramFontBold
		}

		Text {
			visible: !!units
			text: units || ""
			x: parent.width - width
			width: 40
			horizontalAlignment: Text.AlignRight
			anchors.verticalCenter: parent.verticalCenter
			font: Theme.paramFont
			color: enabled ? Theme.unitsColorEnabled : Theme.unitsColorDisabled
			MouseArea {
				anchors.fill: parent
				onClicked: {
					yInput.selectAll()
					yInput.forceActiveFocus()
				}
			}
		}

		ParamInput {
			id: yInput
			text: param ? param.value.y : 0

			x: parent.width - 100
			width: 100 - (units ? unitsLabel.contentWidth : 0)

			validator: valueValidator
			function setValue(value) {
				set("y", value)
			}
		}
	}

	function set(i, v) {
		var value = param.value
		value[i] = v
		param.value = value
	}
}
