import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.12

Popup {
	id: control

	property string title: ""
	property bool dimBackground: false
	property bool resizable: false
	property real minWidth: 400
	property real minHeight: 400

	modal: true
	focus: true
	dim: dimBackground
	closePolicy: Popup.CloseOnEscape
	x: parent ? Math.round((parent.width - width) / 2) : 0
	y: parent ? Math.round((parent.height - height) / 2) : 0

	leftPadding: Theme.dialogHMargins
	rightPadding: Theme.dialogHMargins
	topPadding: Theme.titleBarHeight + Theme.dialogVMargins
	bottomPadding: Theme.dialogVMargins

	Overlay.modal: Rectangle { color: Theme.dimColor }

	// Connections {
	// 	target: control
	// 	function onAboutToShow() {
	// 		control.x = Qt.binding(function() { return control.parent ? Math.round((control.parent.width - control.width) / 2) : 0 })
	// 		control.y = Qt.binding(function() { return control.parent ? Math.round((control.parent.height - control.height) / 2) : 0 })
	// 	}
	// }

	background: Item {
		id: bgContainer

		Rectangle {
			id: bgRect
			anchors.fill: parent
			color: "#fff"
			radius: Theme.dialogCornerRadius
			border.color: Theme.edgeColor
			border.width: 1

			Item {
				width: parent.width
				height: Theme.titleBarHeight
				clip: true

				Rectangle {
					width: parent.width
					height: Theme.titleBarHeight + Theme.dialogCornerRadius
					color: "#f2f4f7"
					radius: Theme.dialogCornerRadius
				}

				MouseArea {
					id: dragArea
					anchors.fill: parent
					enabled: !control.dimBackground
					property point clickPos: "0,0"

					onPressed: {
						clickPos = Qt.point(mouse.x, mouse.y)
					}

					onPositionChanged: {
						var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y)
						var newX = control.x + delta.x
						var newY = control.y + delta.y
						if (control.parent) {
							newX = Math.max(0, Math.min(newX, control.parent.width - control.width))
							newY = Math.max(0, Math.min(newY, control.parent.height - control.height))
						}
						control.x = newX
						control.y = newY
					}
				}
			}

			Text {
				id: popupTitle
				text: control.title
				font: Theme.popupTitleFont
				color: "#17181A"
				height: Theme.titleBarHeight
				width: parent.width
				horizontalAlignment: Text.AlignHCenter
				verticalAlignment: Text.AlignVCenter
			}

			Rectangle {
				width: parent.width
				height: 1
				color: "#d9d9d9"
				y: Theme.titleBarHeight
			}
		}

		DropShadow {
			anchors.fill: bgRect
			horizontalOffset: 0
			verticalOffset: 6
			radius: 24
			samples: 32
			color: "#42000000"
			source: bgRect
			z: -1
		}

		// Visual resize handle indicator
		Image {
			visible: control.resizable
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			anchors.rightMargin: Theme.dialogHMargins / 2
			anchors.bottomMargin: Theme.dialogVMargins / 2
			width: 10
			height: 10
			source: "qrc:/resize_handle.png"
			z: 10
		}

		// Resize Mouse Area
		MouseArea {
			id: resizeHandle
			visible: control.resizable
			width: Theme.dialogHMargins
			height: Theme.dialogHMargins
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			cursorShape: Qt.SizeFDiagCursor
			z: 11

			property point clickPos: "0,0"
			property real startWidth: 0
			property real startHeight: 0

			onPressed: {
				var scenePos = mapToItem(control.parent, mouse.x, mouse.y)
				clickPos = Qt.point(scenePos.x, scenePos.y)
				startWidth = control.width
				startHeight = control.height
			}

			onPositionChanged: {
				var scenePos = mapToItem(control.parent, mouse.x, mouse.y)
				var deltaX = scenePos.x - clickPos.x
				var deltaY = scenePos.y - clickPos.y
				var maxWidth = control.parent ? control.parent.width : 800
				var maxHeight = control.parent ? control.parent.height : 600
				var newWidth = Math.max(control.minWidth, Math.min(maxWidth, startWidth + 2 * deltaX))
				var newHeight = Math.max(control.minHeight, Math.min(maxHeight, startHeight + 2 * deltaY))
				control.width = newWidth
				control.height = newHeight
			}
		}
	}
}
