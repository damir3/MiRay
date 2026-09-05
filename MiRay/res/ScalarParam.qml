import QtQuick 2.15
import QtQuick.Controls 2.0

Item {
	property var param
	property var units
	property var valueValidator: RegExpValidator { regExp : /[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?/ }
	property bool showSlider: !!param && param.max <= 1000
	property double sliderMin: param ? param.min : 0
	property double sliderMax: param ? param.max : 100

	//color: "#ddf"

	visible: !!param && param.visible
	enabled: !!param && param.enabled
	width: parent.width
	height: showSlider ? Theme.paramHeight + 10 : Theme.paramHeight

	Slider {
		id: slider
		visible: showSlider
		y: parent.height - height
		width: parent.width
		height: 8
		topPadding: 0
		bottomPadding: 0
		leftPadding: 0
		rightPadding: 0
		from: sliderMin
		to: sliderMax
		value: param ? param.value : 0
		wheelEnabled: false

		background: Rectangle {
			x: slider.leftPadding
			y: slider.topPadding + slider.availableHeight / 2 - height / 2
			implicitWidth: 200
			implicitHeight: 8
			width: slider.availableWidth
			height: implicitHeight
			radius: 4
			color: enabled ? "#d0d0d0" : "#f0f0f0"

			Rectangle {
				visible: enabled
				width: slider.visualPosition * parent.width
				height: parent.height
				color: Theme.mainColor
				radius: 4
			}
		}

		handle: Rectangle {
			visible: false
			implicitWidth: 8
			implicitHeight: 8
			radius: 4
		}

		onMoved: if (param) param.value = value.toFixed(param.precision)
		onPressedChanged: if (!pressed && param) param.value = value.toFixed(param.precision)
	}

	Item {
		width: parent.width
		height: Theme.paramHeight
		Text {
			id: title
			text: param ? param.title : ""
			//anchors.verticalCenter: parent.verticalCenter
			font: Theme.paramFont
			color: enabled ? Theme.paramTitleEnabled : Theme.paramTitleDisabled
		}

		Text {
			id: unitsLabel
			visible: !!units
			text: units || ""
			x: (param && param.texture ? textureEnabled.x - 5 : parent.width) - width
			width: 40
			horizontalAlignment: Text.AlignRight
			//anchors.verticalCenter: parent.verticalCenter
			font: Theme.paramFont
			color: enabled ? Theme.unitsColorEnabled : Theme.unitsColorDisabled

			MouseArea {
				anchors.fill: parent
				onClicked: {
					valueInput.selectAll()
					valueInput.forceActiveFocus()
				}
			}
		}

		ParamInput {
			id: valueInput
			text: param ? param.value : 0

			x: (param && param.texture ? textureEnabled.x - 5 : parent.width) - 100
			width: 100 - (units ? unitsLabel.contentWidth : 0)
			anchors.verticalCenter: title.verticalCenter

			validator: valueValidator
			function setValue(value) {
				if (param) param.value = parseFloat(value).toFixed(param.precision)
			}

			/*Connections {
				target: param || null
				function onValueChanged() {
					if (param) valueInput.text = param.value
				}
			}*/
		}

		CustomCheckBox {
			id: textureEnabled
			visible: !!param && !!param.texture
			checked: !!param && !!param.texture && param.texture.enabled.value
			onToggled: param.texture.enabled.value = !param.texture.enabled.value
			anchors.right: textureButton.left
			anchors.rightMargin: 5
			anchors.verticalCenter: title.verticalCenter
		}

		Item {
			id: textureButton
			visible: !!param && !!param.texture
			enabled: textureEnabled.checked
			focus: true

			width: 24
			height: 24
			anchors.right: parent.right
			anchors.verticalCenter: title.verticalCenter

			Image {
				id: textureBtnImage
				property string iconName: param && param.texture && param.texture.fileName.value ? "../texture-btn-on" : "../texture-btn-off"
				fillMode: Image.Pad
				anchors.centerIn: parent
				source: iconName + (enabled ? (mouseArea.containsPress ? "-pressed.png" : (mouseArea.containsMouse ? "-hover.png" : "-normal.png")) : "-disabled.png")
			}

			MouseArea {
				id: mouseArea
				anchors.fill: parent
				drag.target: mouseArea
				onClicked: openTextureDialog(param.texture)
				hoverEnabled: true
				cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

				CustomToolTip {
					visible: mouseArea.containsMouse && text
					text: "Edit texture"
				}
			}

			Drag.active: mouseArea.drag.active
			Drag.supportedActions: Qt.CopyAction
			Drag.dragType: Drag.Automatic
			Drag.imageSource: textureBtnImage.iconName + "-hover.png"
			Drag.mimeData: {
				"text/uri-list": param && param.texture && param.texture.fileName.value.replace(/\\/g, "/") || "file://"
			}

			DropArea {
				anchors.fill: parent
				onEntered: {
					drag.accepted = presenter.isImageURL(drag.urls[0])
				}
				onDropped: {
					if (presenter.isImageURL(drop.urls[0])) {
						param.texture.fileName.value = presenter.urlToLocalFile(drop.urls[0])
						drop.accept()
					}
				}
			}
		}
	}

	//	Rectangle {
	//		width: parent.width
	//		height: parent.height
	//		color: "transparent"
	//		border.color: "#ff8000"
	//	}
}
