import QtQuick 2.2
import QtQuick.Controls 1.4
import QtGraphicalEffects 1.12

Item {
	property var param
	property var valueValidator: RegExpValidator { regExp : /[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?/ }

	id: mainItem
	visible: !!param && param.visible
	enabled: !!param && param.enabled
	width: parent.width
	height: Theme.paramHeight

//	Rectangle {
//		x: 0
//		anchors.fill: parent
//		color: "transparent"
//		border.color: "#000000"
//	}

	Text {
		id: title
		text: param ? param.title : ""
		font: Theme.paramFont
		//anchors.verticalCenter: parent.verticalCenter
		color: enabled ? Theme.paramTitleEnabled : Theme.paramTitleDisabled
	}

	Rectangle {
		id: colorArea
		x: (param && param.texture ? textureEnabled.x - 5 : parent.width) - width
		width: 24
		height: 24
		anchors.verticalCenter: title.verticalCenter
		color: enabled && param ? param.value : "#E1E2E6"
		radius: 4
		border.color: enabled ? (colorMouseArea.containsPress ? Theme.mainColorPressed : colorMouseArea.containsMouse ? Theme.mainColor : "#9198A7") : "#E1E2E6"
		border.width: 2

		/*Rectangle {
			anchors.fill: parent
			anchors.margins: 3
			radius: parent.radius - 3
			color: enabled ? param.value : "#eee"
//			radius: height * 0.5
		}*/

		MouseArea {
			id: colorMouseArea
			anchors.fill: parent
			onClicked: {
				parent.focus = true

				ColorDialog.param = param
				ColorDialog.open()
			}
			hoverEnabled: true
			cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
		}
	}

	//DropShadow {
	//	visible: enabled && (colorMouseArea.containsMouse && !colorMouseArea.containsPress)
	//	anchors.fill: colorArea
	//	source: colorArea
	//	color: "#ccc"
	//	radius: 6
	//}

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

	/*DropShadow {
		visible: textureButton.enabled && textureButton.visible && textureMouseArea.containsMouse
		anchors.fill: textureButton
		source: textureButton
		color: "#000"
		radius: textureMouseArea.containsPress ? 2 : 4
	}*/
}
