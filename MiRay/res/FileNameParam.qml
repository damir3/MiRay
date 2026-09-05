import QtQuick 2.12
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.12

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

	RoundButton {
		id: button
		x: parent.width - width
		width: parent.width - title.contentWidth - 12
		height: Theme.paramHeight
		radius: 4
		onClicked: param.openFileDialog()

		CustomToolTip {
			visible: button.hovered && !!param && !!param.value && text
			text: text.text
		}

		contentItem: Item {
			anchors.fill: parent

			Text {
				id: text
				anchors.fill: parent
				anchors.leftMargin: 5
				anchors.rightMargin: 25
				verticalAlignment: Text.AlignVCenter
				text: param ? param.value : null
				font: Theme.paramFont
				color: enabled ? "#000" : "#888"
				elide: Text.ElideLeft
			}

			ToolToggleButton {
				id: deleteButton
				visible: !!param && !!param.value
				enabled: visible
				anchors.right: parent.right
				anchors.verticalCenter: parent.verticalCenter
				width: 24
				height: 24
				iconPressed: "../filepicker-clear-pressed.png"
				iconNormal: "../filepicker-clear-normal.png"
				iconHover: "../filepicker-clear-hover.png"
				iconDisabled: "../filepicker-clear-normal.png"
				onClicked: if (param && param.value) { param.value = "" }
			}

			Image {
				visible: !deleteButton.visible
				anchors.right: parent.right
				anchors.rightMargin: 2
				anchors.verticalCenter: parent.verticalCenter
				source: button.pressed ? "../filepicker-browse-pressed.png" : (button.hovered ? "../filepicker-browse-hover.png" : (enabled ? "../filepicker-browse-normal.png" : "../filepicker-browse-normal.png"))
			}
		}

		DropArea {
			anchors.fill: parent
			onEntered: {
				console.log("FileName.onEntered", drag.urls[0]);
				drag.acceptProposedAction()
			}
			onDropped: {
				console.log("FileName.onDropped", drop.urls[0])
				param.value = presenter.urlToLocalFile(drop.urls[0]);
				drop.accept()
			}
		}
	}

	// CustomButton {
	// 	x: parent.width - width
	// 	width: parent.width - title.contentWidth - 12
	// 	height: Theme.paramHeight
	// 	text: param ? param.value : null
	// 	elide: Text.ElideLeft
	// 	tooltipEnabled: true
	// 	onClicked: param.openFileDialog()

	// 	DropArea {
	// 		anchors.fill: parent
	// 		onEntered: {
	// 			console.log("FileName.onEntered", drag.urls[0]);
	// 			drag.acceptProposedAction()
	// 		}
	// 		onDropped: {
	// 			console.log("FileName.onDropped", drop.urls[0])
	// 			param.value = drop.urls[0];
	// 			drop.accept()
	// 		}
	// 	}
	// }
}
