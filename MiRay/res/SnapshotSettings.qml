import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls 1.4 as QuickControls1
import QtQml.Models 2.2

Item {
	property var currentSnapshot: null

	anchors.fill: parent

	Text {
		id: panelTitle
		x: Theme.margins
		y: Theme.margins
		text: "Snapshots"
		color: enabled ? "#000" : "#ccc"
		font: Theme.groupTitleFont
	}

	Row {
		visible: !!snapshotsList
		anchors.verticalCenter: panelTitle.verticalCenter
		anchors.right: parent.right
		anchors.rightMargin: Theme.margins
		spacing: 4

		ToolToggleButton {
			width: 24
			enabled: list.count > 0 && list.currentIndex >= 0
			iconPressed: "../button-minus-pressed.png"
			iconNormal: "../button-minus-normal.png"
			iconHover: "../button-minus-hover.png"
			iconDisabled: "../button-minus-disabled.png"
			tooltipText: "Delete Snapshot"
			onClicked: snapshotPresenter.deleteSnapshot()
		}

		ToolToggleButton {
			width: 24
			iconPressed: "../button-plus-pressed.png"
			iconNormal: "../button-plus-normal.png"
			iconHover: "../button-plus-hover.png"
			iconDisabled: "../button-plus-disabled.png"
			tooltipText: "Create Snapshot"
			onClicked: snapshotPresenter.createSnapshot()
		}
	}

	QuickControls1.SplitView {
		visible: !!snapshotsList
		enabled: visible
		anchors.fill: parent
		anchors.topMargin: 45
		orientation: Qt.Vertical

		handleDelegate: Rectangle {
			height: 4
			color: styleData.hovered || styleData.pressed ? Theme.mainColor : Theme.edgeColor
		}

		Item {
			width: parent.width
			Layout.minimumHeight: 4 * Theme.rowHeight + 1 + Theme.listVMargins * 2
			implicitHeight: 5 * Theme.rowHeight + 1 + Theme.listVMargins * 2

			Rectangle {
				width: parent.width
				height: 1
				color: "#ccc"
			}

			ListView {
				id: list
				anchors.fill: parent
				anchors.topMargin: 1
				header: Item { height: Theme.listVMargins }

				focus: true
				clip: true

				model: snapshotsList
				onModelChanged: if (!snapshotsList) {
					currentIndex = -1
				}

				ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

				property int dropIndex : -1

				onCurrentIndexChanged: {
					if (snapshotPresenter) {
						snapshotPresenter.setSnapshot(currentIndex)
					}

					currentSnapshot = snapshotsList ? snapshotsList.data(snapshotsList.index(currentIndex, 0), 0x101) : null;
					if (currentSnapshot) {
						currentSnapshot.name.changed()
						currentSnapshot.hasCameraState.changed()
						currentSnapshot.hasTransformations.changed()
						currentSnapshot.hasVisibility.changed()
						currentSnapshot.hasAssignedMaterials.changed()
						currentSnapshot.hasEnvironment.changed()
						currentSnapshot.hasBackground.changed()
					}
				}

				QuickControls1.Menu {
					id: snapshotMenu
					property bool opened: false

					QuickControls1.MenuItem {
						text: "New"
						onTriggered: snapshotPresenter.createSnapshot()
					}
					QuickControls1.MenuItem {
						text: "Delete"
						enabled: !!currentSnapshot
						onTriggered: snapshotPresenter.deleteSnapshot()
					}
					QuickControls1.MenuItem {
						text: "Rename"
						shortcut: "F2"
						enabled: !!currentSnapshot
						onTriggered: list.currentItem.editName()
					}
					QuickControls1.MenuSeparator {}
					QuickControls1.MenuItem {
						text: "Capture"
						enabled: !!currentSnapshot
						onTriggered: snapshotPresenter.updateSnapshot()
					}
					QuickControls1.MenuItem {
						text: "Activate"
						shortcut: "Space"
						enabled: !!currentSnapshot
						onTriggered: snapshotPresenter.activateSnapshot()
					}

					onAboutToShow: opened = true
					onAboutToHide: opened = false
				}

				MouseArea {
					anchors.fill: parent
					z: -1
					onClicked: {
						list.currentIndex = -1
						list.parent.forceActiveFocus()
					}
				}

				MouseArea {
					anchors.fill: parent
					z: -1
					acceptedButtons: Qt.RightButton
					onClicked: snapshotMenu.popup()
				}

				DropArea {
					z: -1
					anchors.fill: parent
					onEntered: {
						drag.accepted = snapshotPresenter.canDropUrls(drag.urls, list.count)
						list.dropIndex = drag.accepted ? list.count : -1
					}
					onPositionChanged: {
						list.dropIndex = list.count
					}
					onDropped: {
						var dropIndex = list.count
						var urls = drop.urls;
						list.dropIndex = -1
						if (snapshotPresenter.canDropUrls(urls, dropIndex)) {
							drop.accept()
							setTimeout(function() {
								snapshotPresenter.dropUrls(urls, dropIndex)
							}, 100);
						}
					}
					onExited: {
						list.dropIndex = -1
					}
				}

				delegate: Rectangle {
					property bool selected : index === list.currentIndex
					x: Theme.listHMargins
					width: list.width - Theme.listHMargins * 2
					height: Theme.rowHeight
					radius: 4
					color: Theme.rowColor(selected, activeFocus, index)

					MouseArea {
						id: dragArea
						anchors.fill: parent
						hoverEnabled: true
						drag.target: dragArea
						onPressed: {
							list.currentIndex = index
							if (list.currentItem) {
								list.currentItem.forceActiveFocus()
							}
						}
						onDoubleClicked: snapshotPresenter.activateSnapshot()
					}

					MouseArea {
						anchors.fill: parent
						acceptedButtons: Qt.RightButton
						onClicked: {
							if (!selected) {
								list.currentIndex = index
								if (list.currentItem) {
									list.currentItem.forceActiveFocus()
								}
							}
							snapshotMenu.popup()
						}
					}

					// ToolTip {
					// 	visible: dragArea.containsMouse && !snapshotMenu.opened

					// 	delay: 1000
					// 	timeout: 10000
					// 	padding: 3

					// 	contentItem: Image {
					// 		source: model.snapshot.preview
					// 	}

					// 	background: Rectangle {
					// 		border.color: "#bbb"
					// 		color: Theme.emptyPreviewColor
					// 	}
					// }

					Drag.active: dragArea.drag.active
					Drag.supportedActions: Qt.CopyAction
					Drag.dragType: Drag.Automatic
					Drag.mimeData: {
						"text/plain": title.text,
						"text/uri-list": "miray://snapshot/" + index
					}

					Text {
						id: title
						visible: !nameEdit.visible
						x: Theme.listPaddings
						width: parent.width - Theme.listPaddings * 2 - (captureButton.visible ? captureButton.width : 0)
						height: parent.height
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight

						text: model.snapshot.name.value
						font: Theme.rowFont
						color: selected ? Theme.selectionTextColor : "#000"
					}

					TextField {
						id: nameEdit
						visible: false
						anchors.fill: parent
						verticalAlignment: Text.AlignVCenter
						font: Theme.rowFont
						text: model.snapshot.name.value
						clip: true
						leftPadding: Theme.listPaddings
						rightPadding: Theme.listPaddings
						topPadding: 0
						bottomPadding: 0

						selectByMouse: true
						selectionColor: Theme.mainColor
						selectedTextColor: "#fff"
						persistentSelection: true

						TextContextMenu {
							target: nameEdit
						}

						background: Rectangle {
							color: "#fff"
							border.color: "#ccc"
							radius: 4
						}

						onEditingFinished: if (!TextEditMenu.visible) { stop(true) }
						Keys.onReturnPressed: stop(true)
						Keys.onTabPressed: stop(true)
						Keys.onBacktabPressed: stop(true)
						Keys.onEscapePressed: stop(false)

						function start() {
							text = model.snapshot.name.value
							visible = true
							selectAll()
							forceActiveFocus()
						}

						function stop(accept) {
							if (accept && text.trim()) {
								model.snapshot.name.value = text
							}
							visible = false
							if (list.currentItem) {
								list.currentItem.forceActiveFocus()
							}
						}
					}

					function editName() {
						nameEdit.start()
					}

					Keys.onSpacePressed: snapshotPresenter.activateSnapshot()

					Keys.onPressed: {
						if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_F2) {
							event.accepted = true
							editName()
						}
					}

					Rectangle {
						visible: list.dropIndex === index
						width: parent.width
						height: 2
						color: Theme.dropTargetColor
					}

					DropArea {
						id: dropArea
						anchors.fill: parent

						onEntered: {
							var dropIndex = drag.y < (height >> 1) ? index : index + 1
							drag.accepted = snapshotPresenter.canDropUrls(drag.urls, dropIndex)
							list.dropIndex = drag.accepted ? dropIndex : -1
						}
						onPositionChanged: {
							list.dropIndex = drag.y < (height >> 1) ? index : index + 1
						}
						onDropped:  {
							var dropIndex = list.dropIndex
							var urls = drop.urls
							list.dropIndex = -1
							if (snapshotPresenter.canDropUrls(urls, dropIndex)) {
								drop.accept()
								setTimeout(function() {
									snapshotPresenter.dropUrls(urls, dropIndex)
								}, 100);
							}
						}
						onExited: {
							list.dropIndex = -1
						}
					}

					Rectangle {
						id: captureButton
						visible: selected && !nameEdit.visible
						anchors.verticalCenter: parent.verticalCenter
						width: buttonTitle.contentWidth + 10
						height: 16
						x: parent.width - width - 4
						radius: 4
						color: saveButtonMouseArea.containsPress ? "#E0E2E4" : "#fff"

						Text {
							id: buttonTitle
							anchors.centerIn: parent
							text: "Capture"
							font: Theme.buttonFont
							color: Theme.mainColor
						}

						MouseArea {
							id: saveButtonMouseArea
							anchors.fill: parent
							hoverEnabled: true
							cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

							onClicked: snapshotPresenter.updateSnapshot()
						}
					}
				} // delegate: Rectangle

				footer: Rectangle {
					visible: list.dropIndex === list.count
					width: parent.width
					height: 2
					color: Theme.dropTargetColor
				}
			} // ListView
		}

		Flickable {
			visible: list.count > 0 && list.currentIndex >= 0
			enabled: visible
			Layout.minimumHeight: 300
			width: parent.width
			clip: true

			contentHeight: paramsLayout.height

			ScrollBar.vertical: ScrollBar {
				policy: ScrollBar.AsNeeded
			}

			ColumnLayout {
				visible: !!currentSnapshot
				id: paramsLayout
				x: 10
				width: parent.parent.width - 20
				spacing: Theme.paramSpacing

				GroupTitle {
					text: "Saved information"
				}

				BoolParam {
					param: currentSnapshot ? currentSnapshot.hasCameraState : null
				}

				BoolParam {
					param: currentSnapshot ? currentSnapshot.hasTransformations : null
				}

				BoolParam {
					param: currentSnapshot ? currentSnapshot.hasVisibility : null
				}

				BoolParam {
					param: currentSnapshot ? currentSnapshot.hasAssignedMaterials : null
				}

				BoolParam {
					param: currentSnapshot ? currentSnapshot.hasEnvironment : null
				}

				BoolParam {
					param: currentSnapshot ? currentSnapshot.hasBackground : null
				}

				Item { height: 10 }
			} // ColumnLayout
		}
	}

	function setSnapshot(index) {
		list.currentIndex = index
	}

	Timer { id: timer }
	function setTimeout(callback, delayTime) {
		timer.interval = delayTime;
		timer.repeat = false;
		timer.triggered.connect(callback);
		timer.triggered.connect(function release() {
			timer.triggered.disconnect(callback);
			timer.triggered.disconnect(release);
		});
		timer.start();
	}
}
