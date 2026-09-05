import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

PopupDialog {
	id: dialogWindow
	title: "Resource Manager"
	dimBackground: true
	resizable: true
	minWidth: 800
	minHeight: 450

	width: 1024
	height: 500 + Theme.titleBarHeight

	Item {
		anchors.fill: parent
		focus: true

		Item {
			anchors.fill: parent
			anchors.topMargin: Theme.buttonHeight + Theme.dialogVMargins
			anchors.bottomMargin: Theme.buttonHeight + Theme.dialogVMargins

			Item {
				width: parent.width - 258 - Theme.dialogHMargins
				height: parent.height

				Rectangle {
					y: 1
					x: 1
					width: parent.width - 2
					height: 23
					color: "#eee"

					/*Text {
						anchors.leftMargin: 10
						anchors.fill: parent
						text: "Status"
						font: Theme.rowFontBold
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}*/

					Text {
						anchors.leftMargin: list.column0Width + 10
						anchors.fill: parent
						text: "Old File"
						font: Theme.rowFontBold
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}

					Text {
						anchors.leftMargin: list.splitPos + 10
						anchors.fill: parent
						text: "New File"
						font: Theme.rowFontBold
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}

					MouseArea {
						anchors.fill: parent
						hoverEnabled: true

						property int mouseOffset: 0
						property bool dragEnabled: false
						property bool dragAllowed: false

						cursorShape: (containsMouse && dragAllowed) || dragEnabled ? Qt.SplitHCursor : Qt.ArrowCursor

						onPressed: {
							if (dragAllowed) {
								dragEnabled = true
								mouseOffset = list.splitPos - mouse.x
							}
						}

						onPositionChanged: {
							dragAllowed = mouse.x >= list.splitPos - 5 && mouse.x <= list.splitPos + 5
							if (dragEnabled) {
								list.setSplitPos(mouse.x + mouseOffset);
							}
						}

						onReleased: {
							dragEnabled = false
						}
					}
				}

				Rectangle {
					y: 24
					width: parent.width
					height: 1
					color: Theme.edgeColor
				}

				ListView {
					id: list
					anchors.topMargin: 25
					anchors.fill: parent
					anchors.margins: 1
					currentIndex: 0
					clip: true
					focus: true

					ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

					property int column0Width: 25
					property int minColumnWidth: (740 - 30 - column0Width - 256) >> 1
					property double splitPosF: 0.5
					property int splitPos: column0Width + (width - column0Width) * splitPosF

					model: resManagerPresenter ? resManagerPresenter.images : null

					Keys.onUpPressed: if (currentIndex >= 0) {
						decrementCurrentIndex()
						currentItem.forceActiveFocus()
						resManagerPresenter.select(currentIndex, event.modifiers)
					}

					Keys.onDownPressed: if (currentIndex >= 0) {
						incrementCurrentIndex()
						currentItem.forceActiveFocus()
						resManagerPresenter.select(currentIndex, event.modifiers)
					}

					function setSplitPos(pos) {
						list.splitPosF = Math.min(Math.max(pos - column0Width, minColumnWidth), width - column0Width - minColumnWidth) / (width - column0Width)
						//console.log("setSplitPos", pos, width, minColumnWidth, list.splitPosF)
					}

					onWidthChanged: if (width > column0Width) { setSplitPos(column0Width + splitPosF * (width - column0Width)) }

					delegate: Rectangle {
						//property bool selected : index == list.currentIndex
						width: list.width
						height: Theme.rowHeight
						color: Theme.rowColor(modelData.selected, activeFocus, index)

						Image {
							x: (list.column0Width - width) >> 1
							anchors.verticalCenter: parent.verticalCenter
							source: modelData.selected ?
								(modelData.status ? "../imagemanager/image-good-s.png" : "../imagemanager/image-error-s.png") :
								(modelData.status ? "../imagemanager/image-good-u.png" : "../imagemanager/image-error-u.png");
						}

						Text {
							x: list.column0Width + 5
							width: list.splitPos - x - 5
							height: parent.height
							verticalAlignment: Text.AlignVCenter
							elide: Text.ElideLeft

							text: modelData.oldPath
							font: Theme.rowFont
							color: modelData.selected ? Theme.selectionTextColor : "#000"
						}

						Text {
							x: list.splitPos + 5
							width: parent.width - x - 5
							height: parent.height
							verticalAlignment: Text.AlignVCenter
							elide: Text.ElideLeft

							text: modelData.newPath
							font: Theme.rowFont
							color: modelData.selected ? Theme.selectionTextColor : "#000"
						}

						MouseArea {
							id: mouseArea
							anchors.fill: parent
							hoverEnabled: true
							onPressed: {
								list.currentIndex = index
								list.currentItem.forceActiveFocus()
								resManagerPresenter.select(index, mouse.modifiers)
							}
							property int mousePos: 0
							onPositionChanged: mousePos = mouse.x


							/*ToolTip {
								id: control

								visible: mouseArea.containsMouse
								x: mouseArea.mousePos
								delay: 1000
								timeout: 10000
								padding: 3

								contentItem: Item {
									width: 258
									height: preview.height + title.contentHeight

									Image {
										id: preview
										fillMode: Image.Pad
										anchors.horizontalCenter: parent.horizontalCenter
										source: modelData.preview || "../no_texture.png"
									}

									Text {
										id: title
										anchors.top: preview.bottom
										text: modelData[mouseArea.mousePos < list.splitPos ? "oldPath" : "newPath"]
										font.pixelSize: Theme.tooltipTextSize
										wrapMode: Text.WrapAnywhere
									}
								}

								background: Rectangle {
									border.color: "#bbb"
									color: "#eee"
								}
							}*/

							CustomToolTip {
								x: mouseArea.mousePos
								visible: mouseArea.containsMouse && text
								text: modelData[mouseArea.mousePos < list.splitPos ? "oldPath" : "newPath"]
							}
						}
					}
				}

				Rectangle {
					x: list.column0Width
					width: 1
					height: parent.height
					color: Theme.edgeColor
				}

				Rectangle {
					x: list.splitPos
					width: 1
					height: parent.height
					color: Theme.edgeColor
				}

				Rectangle {
					anchors.fill: parent
					color: "transparent"
					border.color: Theme.edgeColor
					radius: 4
				}
			}

			Item {
				visible: !!list.currentItem

				width: 258
				height: 258
				anchors.right: parent.right
				Image {
					id: imageBG
					source: "../checkers.png"
					fillMode: Image.Tile
					anchors.centerIn: preview
					width: Math.floor(preview.paintedWidth)
					height: Math.floor(preview.paintedHeight)
				}
				Image {
					id: preview
					anchors.centerIn: parent
					width: parent.height - 2
					height: parent.height - 2
					fillMode: Image.PreserveAspectFit
					source: (list.currentItem && resManagerPresenter ? resManagerPresenter.images[list.currentIndex].preview : null) || "../no_texture.png"
				}
				Rectangle {
					color: "transparent"
					border.color: Theme.edgeColor
					anchors.fill: imageBG
					anchors.margins: -1
				}
			}
		}

		RowLayout {
			width: parent.width
			spacing: Theme.buttonSpacing

			CustomButton {
				text: "Undo"
				enabled: !!resManagerPresenter && resManagerPresenter.canUndo
				onClicked: resManagerPresenter.undo()
			}

			CustomButton {
				text: "Redo"
				enabled: !!resManagerPresenter && resManagerPresenter.canRedo
				onClicked: resManagerPresenter.redo()
			}

			Item { Layout.fillWidth: true }

			CustomButton {
				text: "Change Folder"
				enabled: !!resManagerPresenter && resManagerPresenter.selectionCount > 0
				onClicked: resManagerPresenter.changeFolder()
			}

			CustomButton {
				text: "Change File"
				enabled: !!resManagerPresenter && resManagerPresenter.selectionCount === 1
				onClicked: resManagerPresenter.changeFile()
			}

			CustomButton {
				text: "Reset"
				enabled: !!resManagerPresenter && resManagerPresenter.selectionCount > 0
				onClicked: resManagerPresenter.reset()
			}

			CustomButton {
				text: "Clear"
				enabled: !!resManagerPresenter && resManagerPresenter.selectionCount > 0
				onClicked: resManagerPresenter.clear()
			}
		}

		Row {
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			spacing: Theme.buttonSpacing

			CustomButton {
				text: "Cancel"
				onClicked: dialogWindow.close()
			}

			CustomButton {
				text: "OK"
				onClicked: {
					resManagerPresenter.accept()
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: dialogWindow.close()

		Keys.onPressed: {
			if (event.matches(StandardKey.Undo)) {
				event.accepted = true
				resManagerPresenter.undo()
			} else if (event.matches(StandardKey.Redo)) {
				event.accepted = true
				resManagerPresenter.redo()
			}
		}
	}

	onAboutToShow: {
		presenter.beginResourceManager()
		list.currentIndex = 0
		if (list.currentItem) {
			list.currentItem.forceActiveFocus()
		}
		resManagerPresenter.select(0, 0)
	}

	onAboutToHide: presenter.endResourceManager()
}
