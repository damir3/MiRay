import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

PopupDialog {
	id: dialogWindow
	title: "Rendering Layers Manager"

	width: 400
	height: 300 + Theme.titleBarHeight

	property bool isEditing: false
	closePolicy: isEditing ? Popup.NoAutoClose : Popup.CloseOnEscape

	Item {
		anchors.fill: parent
		focus: true

		Rectangle {
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.bottomMargin: Theme.buttonHeight + Theme.dialogVMargins
			border.color: Theme.edgeColor
			border.width: 1
			color: "#fff"
			radius: 4

			ListView {
				id: list
				anchors.fill: parent
				anchors.margins: 1
				clip: true
				focus: true

				model: renderingLayersManagerPresenter ? renderingLayersManagerPresenter.layers : null

				ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

				delegate: Rectangle {
					width: list.width
					height: Theme.rowHeight
					color: Theme.rowColor(index === list.currentIndex && !dialogWindow.isEditing, activeFocus || nameEdit.activeFocus, index)

					Text {
						id: layerName
						x: 8
						anchors.verticalCenter: parent.verticalCenter
						text: modelData.value
						font: Theme.rowFont
						color: index === list.currentIndex ? Theme.selectionTextColor : "#000"
						visible: !nameEdit.visible
					}

					TextField {
						id: nameEdit
						visible: false
						x: 4
						width: parent.width - 8
						height: parent.height
						verticalAlignment: Text.AlignVCenter
						font: Theme.rowFont
						clip: true
						leftPadding: 4
						rightPadding: 4
						topPadding: 0
						bottomPadding: 0

						selectByMouse: true
						selectionColor: Theme.mainColor
						selectedTextColor: "#fff"
						persistentSelection: true

						background: Rectangle {
							color: "#fff"
							border.color: "#ccc"
							radius: 4
						}

						onEditingFinished: if (!TextEditMenu.visible) { stop(true) }
						Keys.onReturnPressed: stop(true)
						Keys.onTabPressed: stop(true)
						Keys.onBacktabPressed: stop(true)
						Keys.onEscapePressed: {
							stop(false)
							event.accepted = true
						}

						function start() {
							console.log("start editing", modelData.value)
							dialogWindow.isEditing = true
							text = modelData.value
							visible = true
							selectAll()
							forceActiveFocus()
						}

						function stop(accept) {
							console.log("stop editing", accept, text, modelData.value, visible)
							if (!visible) {
								return
							}
							if (accept && text.trim() !== "") {
								modelData.value = text.trim()
							}
							dialogWindow.isEditing = false
							visible = false
							if (list.currentItem) {
								list.currentItem.forceActiveFocus()
							} else {
								list.forceActiveFocus()
							}
						}
					}

					MouseArea {
						anchors.fill: parent
						visible: !nameEdit.visible
						onClicked: {
							list.currentIndex = index
							list.forceActiveFocus()
						}
						onDoubleClicked: {
							list.currentIndex = index
							nameEdit.start()
						}
					}

					function startEditing() {
						nameEdit.start()
					}
				}

				Keys.onUpPressed: if (currentIndex > 0) { currentIndex-- }
				Keys.onDownPressed: if (currentIndex < count - 1) { currentIndex++ }
				Keys.onPressed: {
					if (event.key === Qt.Key_F2 || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
						if (currentItem) {
							event.accepted = true
							currentItem.startEditing()
						}
					}
				}
			}
		}

		Row {
			anchors.left: parent.left
			anchors.bottom: parent.bottom
			spacing: Theme.buttonSpacing

			CustomButton {
				text: "New"
				onClicked: {
					renderingLayersManagerPresenter.createLayer()
					list.currentIndex = list.count - 1
					list.currentItem.startEditing()
				}
			}

			CustomButton {
				text: "Delete"
				onClicked: {
					var idx = list.currentIndex
					renderingLayersManagerPresenter.removeLayer(idx)
					list.currentIndex = Math.min(idx, list.count - 1)
				}
			}
		}

		Row {
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			spacing: Theme.buttonSpacing

			CustomButton {
				text: "OK"
				onClicked: dialogWindow.close()
			}
		}

		Keys.onEscapePressed: {
			if (!dialogWindow.isEditing) {
				dialogWindow.close()
			}
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

	onAboutToShow: {
		presenter.beginRenderingLayersManager()
		list.currentIndex = 0
		if (list.currentItem) {
			list.currentItem.forceActiveFocus()
		}
	}

	onAboutToHide: {
		presenter.endRenderingLayersManager()
	}
}
