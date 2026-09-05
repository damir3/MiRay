import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.15

PopupDialog {
	property var model
	property int columnWidth: 255

	id: dialogWindow

	title: model ? model.title : ""

	leftPadding: 0
	rightPadding: 0

	width: columnWidth + Theme.dialogHMargins * 2
	height: 800 + Theme.titleBarHeight

	Item {
		anchors.fill: parent
		focus: true

		ColumnLayout {
			anchors.fill: parent
			spacing: 0

			Item {
				Layout.alignment: Qt.AlignHCenter
				//color: "#ccc"
				width: columnWidth
				height: columnWidth
				Image {
					id: imageBG
					source: "../bg_checkers.png"
					fillMode: Image.Tile
					x: Math.floor((parent.width - preview.paintedWidth) / 2)
					y: Math.floor((parent.height - preview.paintedHeight) / 2)
					width: Math.floor(preview.paintedWidth)
					height: Math.floor(preview.paintedHeight)
				}
				Image {
					id: preview
					anchors.centerIn: parent
					anchors.fill: parent
					fillMode: Image.PreserveAspectFit
					source: (model && model.texture.preview) || "../no_texture.png"
				}
				Rectangle {
					color: "transparent"
					border.color: "#808080"
					anchors.fill: imageBG
					anchors.margins: -1
					//onWidthChanged: console.log("!!!", x, width, parent.width, preview.paintedWidth, Math.floor((parent.width - preview.paintedWidth) / 2) - 2)
				}
				Rectangle {
					id: cropRect
					visible: !!model

					property double cropLeft: model && model.cropLeft ? model.cropLeft.value : 0.0
					property double cropTop: model && model.cropTop ? model.cropTop.value : 0.0
					property double cropRight: model && model.cropRight ? model.cropRight.value : 1.0
					property double cropBottom: model && model.cropBottom ? model.cropBottom.value : 1.0

					x: imageBG.x + Math.min(cropLeft, cropRight) * imageBG.width - 1
					y: imageBG.y + Math.min(cropTop, cropBottom) * imageBG.height - 1
					width: Math.abs(cropRight - cropLeft) * imageBG.width + 2
					height: Math.abs(cropBottom - cropTop) * imageBG.height + 2

					color: "transparent"
					border.color: "#808080"
					border.width: 1
				}
			}

			Item { height: Theme.paramSpacing }

			FileNameParam {
				Layout.alignment: Qt.AlignHCenter
				width: columnWidth
				param: model ? model.fileName : null
			}

			Item { height: Theme.paramSpacing }

			Rectangle {
				width: dialogWindow.width
				height: 1
				color: Theme.edgeColor
			}

			Flickable {
				id: flickable
				Layout.alignment: Qt.AlignHCenter
				width: columnWidth + Theme.dialogHMargins + 10
				Layout.fillHeight: true
				clip: true

				contentHeight: paramsLayout.height

				ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

				ColumnLayout {
					id: paramsLayout
					x: Theme.dialogHMargins / 2 + 5
					width: columnWidth
					spacing: Theme.paramSpacing

					Item { height: 1 }

					BoolParam {
						param: model ? model.normalMap : null
					}

					BoolParam {
						param: model ? model.invert : null
					}

					GroupTitle {
						text: "Color Correction"
					}

					ScalarParam {
						param: model ? model.brightness : null
					}

					ScalarParam {
						param: model ? model.contrast : null
					}

					ScalarParam {
						param: model ? model.gamma : null
					}

					EnumParam {
						param: model ? model.channel : null
					}

					GroupTitle {
						text: "Mapping"
						// visible: !!model && model.wrapX.visible
					}

					EnumParam {
						param: model ? model.mapping : null
					}

					Vec2Param {
						param: model ? model.offset : null
					}

					Vec2Param {
						param: model ? model.repeat : null
					}

					ScalarParam {
						param: model ? model.rotation : null
					}

					EnumParam {
						param: model ? model.wrapX : null
					}

					EnumParam {
						param: model ? model.wrapY : null
					}

					EnumParam {
						param: model ? model.filtering : null
					}

					GroupTitle {
						text: "Cropping"
					}

					ScalarParam {
						param: model ? model.cropLeft : null
					}

					ScalarParam {
						param: model ? model.cropTop : null
					}

					ScalarParam {
						param: model ? model.cropRight : null
					}

					ScalarParam {
						param: model ? model.cropBottom : null
					}

					Item { height: 1 }
				}
			}

			Rectangle {
				width: dialogWindow.width
				height: 1
				color: Theme.edgeColor
			}

			Item {
				height: Theme.dialogVMargins
			}

			CustomButton {
				text: "OK"
				Layout.alignment: Qt.AlignHCenter
				onClicked: {
					console.log("onModelChanged", dialogWindow.height)
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: {
			dialogWindow.close()
		}

		Keys.onPressed: {
			if (event.matches(StandardKey.Undo)) {
				event.accepted = true
				presenter.undo()
			} else if (event.matches(StandardKey.Redo)) {
				event.accepted = true
				presenter.redo()
			}
		}
	}

	onAboutToShow: presenter.beginTextureDialog()
	onAboutToHide: {
		flickable.contentY = 0
		presenter.endTextureDialog()
	}
}
