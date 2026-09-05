import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

PopupDialog {
	id: dialogWindow
	title: "Rendering..."
	dimBackground: true
	closePolicy: Popup.NoAutoClose
	resizable: true

	width: 800
	height: 600

	property real percentage: 0
	property real startTime: 0

	Item {
		anchors.fill: parent

		// Preview Image Area
		Item {
			anchors.top: parent.top
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.bottom: controlsArea.top
			anchors.bottomMargin: Theme.dialogVMargins
			clip: true

			// Checkerboard background
			Image {
				width: preview.paintedWidth
				height: preview.paintedHeight
				anchors.centerIn: preview
				fillMode: Image.Tile
				source: "qrc:/bg_checkers.png"
				// visible: preview.source != ""
			}

			Image {
				id: preview
				anchors.fill: parent
				anchors.margins: 1
				fillMode: Image.PreserveAspectFit
				source: ""
			}

			Rectangle {
				width: preview.paintedWidth + 2
				height: preview.paintedHeight + 2
				anchors.centerIn: preview
				// anchors.margins: -1
				color: "transparent"
				border.color: Theme.edgeColor
			}
		}

		// Controls and Progress Area
		Item {
			id: controlsArea
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			height: 30

			// Layout 1: Progress, Time, Good Enough (Visible during rendering)
			RowLayout {
				id: layoutProgress
				anchors.fill: parent
				spacing: Theme.buttonSpacing

				CustomProgressBar {
					Layout.fillWidth: true
					value: percentage * 0.01
				}

				Text {
					id: timeText
					Layout.preferredWidth: 60
					horizontalAlignment: Text.AlignRight
					font.family: "Courier New"
					font.pixelSize: 14
					color: "#17181A"
				}

				Timer {
					id: elapsedTimer
					interval: 1000
					running: true
					repeat: true
					onTriggered: updateTime()
				}

				CustomButton {
					text: "Good Enough"
					onClicked: {
						if (renderingModel) {
							renderingModel.stop();
						}
					}
				}
			}

			// Layout 2: Save, Render More, Close (Visible after rendering completes)
			RowLayout {
				id: layoutFinished
				anchors.fill: parent
				spacing: Theme.buttonSpacing
				visible: false

				Item {
					Layout.fillWidth: true
				}

				CustomButton {
					text: "Save..."
					onClicked: {
						if (renderingModel) {
							renderingModel.save();
						}
					}
				}

				CustomButton {
					text: "Render More"
					onClicked: {
						if (renderingModel) {
							renderingModel.renderMore();
						}
					}
				}

				CustomButton {
					text: "Close"
					onClicked: dialogWindow.close()
				}

				Item {
					Layout.fillWidth: true
				}
			}
		}


	}

	Connections {
		target: renderingModel

		function onPreviewChanged(imageId) {
			preview.source = "image://Preview/" + imageId
		}

		function onProgress(progress) {
			if (progress === 0) {
				layoutProgress.visible = true
				layoutFinished.visible = false
				startTime = Date.now()
				updateTime()
			}
			percentage = progress * 100
		}

		function onRenderingFinished() {
			layoutProgress.visible = false
			layoutFinished.visible = true
		}
	}

	function updateTime() {
		var totalSeconds = (Date.now() - startTime) * 0.001;
		var seconds = Math.floor(totalSeconds % 60);
		var minutes = Math.floor((totalSeconds / 60) % 60);
		var hours = Math.floor(totalSeconds / 3600);
		var text = (minutes < 10 ? "0" + minutes : minutes) + ":" + (seconds < 10 ? "0" + seconds : seconds);
		if (hours > 0) {
			text = (hours < 10 ? "0" + hours : hours) + ":" + text;
		}
		timeText.text = text;
	}

	onAboutToShow: presenter.beginRendering()
	onAboutToHide: presenter.endRendering()
}
