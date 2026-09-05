import QtQuick 2.12
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.12

Item {
	property var param
	property var comboBoxWidth: 0.5
	property bool tooltipEnabled: false

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

	onParamChanged: if (param) {
		comboBox.model = param.model
		comboBox.currentIndex = param.value
	}

	ComboBox {
		id: comboBox
		x: parent.width - width
		width: parent.width * comboBoxWidth
		height: parent.height
		focus: true
		model: param ? param.model : null
		onActivated: param.value = index
		hoverEnabled: true
//		Component.onCompleted: currentIndex = param ? param.value : 0
		Connections {
			target: param || null
			function onValueChanged() {
				comboBox.model = param.model
				comboBox.currentIndex = param.value;
			}
		}

		delegate: ItemDelegate {
			width: comboBox.width - 2
			height: 25
			highlighted: comboBox.highlightedIndex === index
			contentItem: Text {
				text: modelData
				color: highlighted ? Theme.selectionTextColor : "#000"
				//font: comboBox.currentIndex === index ? Theme.paramFontBold: Theme.paramFont
				font: Theme.paramFont
				elide: Text.ElideRight
				verticalAlignment: Text.AlignVCenter
			}
			background: Rectangle {
				anchors.fill: parent
				color: highlighted ? Theme.mainColor : "transparent"
				anchors.leftMargin: Theme.menuMargins
				anchors.rightMargin: Theme.menuMargins
				radius: 4
			}
		}

		contentItem: Text {
			leftPadding: 10
			rightPadding: 25

			text: comboBox.displayText
			font: Theme.paramFont
			color: enabled ? Theme.paramTextColorEnabled : Theme.paramTextColorDisabled
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight
		}

		background: Rectangle {
			id: bgRect
			anchors.fill: parent
			color: Theme.comboBoxColor(enabled, comboBox.pressed, comboBox.hovered)
			// radius: height / 2
			radius: 4
			//border.color: enabled ? (comboBox.activeFocus ? Theme.mainColor : "#ccc") : "transparent"

			Image {// indicator
				anchors.right: parent.right
				//anchors.rightMargin: 10
				anchors.verticalCenter: parent.verticalCenter
				source: enabled ? "../combo-indicator-enabled.png" : "../combo-indicator-disabled.png"
			}
		}

		CustomToolTip {
			visible: tooltipEnabled && comboBox.hovered && text
			text: comboBox.displayText
		}

		// DropShadow {
		// 	visible: enabled && comboBox.hovered && !comboBox.pressed
		// 	anchors.fill: bgRect
		// 	source: bgRect
		// 	color: "#ccc"
		// 	radius: 6
		// }

		indicator: Item {}

		popup: Popup {
			y: comboBox.height + 1
			width: comboBox.width
			implicitHeight: contentItem.implicitHeight + 2
			padding: 1

			contentItem: ListView {
				clip: true
				implicitHeight: contentHeight
				model: comboBox.popup.visible ? comboBox.delegateModel : null
				currentIndex: comboBox.highlightedIndex

				ScrollIndicator.vertical: ScrollIndicator { }

				header: Item { height: Theme.menuMargins }
				footer: Item { height: Theme.menuMargins }
			}

			background: Rectangle {
				border.color: "#ccc"
				//radius: 10
				radius: 4
			}
		}
	}
}
