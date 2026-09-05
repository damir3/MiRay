import QtQuick 2.15
import QtQuick.Controls 1.4 as QuickControls1
import QtQuick.Controls 2.15
import QtQuick.Controls.Styles 1.4
import QtQml.Models 2.2

Item {
	anchors.fill: parent
	visible: !!sceneTreePresenter

	Image {
		id: loop
		x: 2
		anchors.verticalCenter: itemNameFilter.verticalCenter
		source: "../search.png"
	}

	Rectangle {
		id: inputRect
		anchors.fill: itemNameFilter
		anchors.leftMargin: -10
		anchors.rightMargin: -10
		anchors.topMargin: -3
		anchors.bottomMargin: -3
		color: "#eee"
		radius: 4
	}

	MouseArea {
		anchors.fill: itemNameFilter
		hoverEnabled: true
		cursorShape: containsMouse ? Qt.IBeamCursor : Qt.ArrowCursor
	}

	TextInput {
		id: itemNameFilter
		selectByMouse: true
		focus: true
		clip: true
		anchors.left: loop.right
		anchors.leftMargin: 15
		y: 48
		width: parent.width - x - 15
		font: Theme.paramFont
		onTextChanged: sceneTreePresenter.setFilter(text)
	}

	Rectangle {
		id: topBorderRect
		anchors.top: inputRect.bottom
		anchors.topMargin: 5
		width: parent.width
		height: 1
		color: "#ccc"
	}

	ListView {
		id: treeView
		focus: true
		clip: true
		width: parent.width
		anchors.top: topBorderRect.bottom
		anchors.bottom: parent.bottom

		model: sceneTreeModel

		ScrollBar.vertical: ScrollBar {
			policy: ScrollBar.AsNeeded
		}

		Keys.onUpPressed: if (currentIndex >= 0) {
			decrementCurrentIndex()
			currentItem.forceActiveFocus()
			sceneTreePresenter.select(currentIndex, event.modifiers)
		}

		Keys.onDownPressed: if (currentIndex >= 0) {
			incrementCurrentIndex()
			currentItem.forceActiveFocus()
			sceneTreePresenter.select(currentIndex, event.modifiers)
		}

		MouseArea {
			anchors.fill: parent
			z: -1
			onClicked: {
				treeView.currentIndex = -1
				treeView.parent.forceActiveFocus()
				sceneTreePresenter.select(-1, mouse.modifiers)
			}
		}

		MouseArea {
			anchors.fill: parent
			z: -1
			acceptedButtons: Qt.RightButton
			onClicked: contextMenu.popup()
		}

		delegate: Rectangle {
			id: tableRow
			property int offsetX : Math.min(model.level * 16, 128)
			property bool selected : model.selected === true && !nameEdit.visible

			width: treeView.width
			height: Theme.rowHeight
			color: dropArea.dropDelta === 0 ? Theme.dropTargetColor2 : Theme.rowColor(selected, activeFocus, index)

			Drag.active: dragArea.dragging === 1
			Drag.supportedActions: Qt.MoveAction
			Drag.proposedAction: Qt.MoveAction
			Drag.dragType: Drag.Automatic
			Drag.source: tableRow
			Drag.onDragFinished: dragArea.dragging = -1

			MouseArea { // tooltip
				anchors.fill: parent
				preventStealing: true
				propagateComposedEvents: true
				hoverEnabled: true
				CustomToolTip {
					visible: parent.containsMouse
					text: model.material != null ? model.material : model.name
				}
			}

			MouseArea {
				id: dragArea
				anchors.fill: parent
				preventStealing: true
				propagateComposedEvents: true

				property int dragging: -1
				property point pressedPos: Qt.point(0, 0)

				onPressed: {
					//console.log("onPressed", index)
					treeView.currentIndex = index
					treeView.currentItem.forceActiveFocus()
					pressedPos.x = mouse.x
					pressedPos.y = mouse.y
					dragging = -1
					//parent.Drag.start()
				}
				onClicked: {
					treeView.currentIndex = index
					treeView.currentItem.forceActiveFocus()
					sceneTreePresenter.select(index, mouse.modifiers)
				}
				onPositionChanged: {
					if (dragging < 0 && (Math.abs(pressedPos.x - mouse.x) > 4 || Math.abs(pressedPos.y - mouse.y) > 4)) {
						dragging = 0;
						//console.log("positionChanged")
						if (!selected) {
							sceneTreePresenter.select(index, 0);
						}
						var mimeData = sceneTreePresenter.createDragMimeData(index)
						//console.log("mimeData", mimeData["text/plain"])
						if (mimeData["miray/dragged-rows"]) {
							tableRow.Drag.mimeData = mimeData;
							dragging = 1;
							//console.log("dragging")
						}
					}
				}
				onReleased: {
					//console.log("onReleased")
					dragging = -1
				}

				onDoubleClicked: nameEdit.start()
			}

			MouseArea {
				anchors.fill: parent
				acceptedButtons: Qt.RightButton
				onClicked: {
					if (!selected) {
						sceneTreePresenter.select(index, 0)
					}
					contextMenu.popup()
				}
			}

			Image {
				visible: model.expanded !== undefined
				anchors.verticalCenter: parent.verticalCenter
				x: offsetX + 5
				source: selected ? "../triangle-sel.png" : "../triangle-unsel.png"
				rotation: model.expanded === true ? 90 : 0
				Behavior on rotation { NumberAnimation { duration: 100 } }

				MouseArea {
					anchors.fill: parent
					onClicked: model.expanded = !model.expanded
				}
			}

			CustomCheckBox {
				visible: model.visible !== undefined
				checked: model.visible === true
				inverted: selected
				onToggled: model.visible = !model.visible
				x: offsetX + 18
			}

			Text {
				id: nodeName
				visible: !nameEdit.visible
				x: offsetX + 40
				width: Math.max(parent.width - x - Theme.margins - (model.material === undefined ? 0 : 80), 0)
				height: parent.height
				verticalAlignment: Text.AlignVCenter
				font: Theme.rowFont
				text: model.name
				elide: Text.ElideRight
				color: selected ? Theme.selectionTextColor : (model.material === undefined ? "#000" : "#444")
			}

			Text {
				visible: model.material !== undefined
				x: nodeName.x + nodeName.contentWidth + Theme.margins
				width: parent.width - x - Theme.margins
				height: parent.height
				horizontalAlignment: Text.AlignRight
				verticalAlignment: Text.AlignVCenter
				elide: Text.ElideRight
				font: Theme.rowFont
				text: model.material || ""
				color: selected ? Theme.selectionTextColor : "#444"
			}

			TextField {
				id: nameEdit
				visible: false
				x: nodeName.x - 6
				width: parent.width - x - Theme.margins
				height: parent.height
				verticalAlignment: Text.AlignVCenter
				font: Theme.rowFont
				text: model.name
				clip: true
				leftPadding: 6
				rightPadding: 6
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
					if (model.material === undefined) {// not geometry
						text = model.name
						visible = true
						selectAll()
						forceActiveFocus()
					}
				}

				function stop(accept) {
					if (accept) {
						model.name = text
					}
					visible = false
					if (treeView.currentItem) {
						treeView.currentItem.forceActiveFocus()
					}
				}
			}

			Rectangle {
				x: offsetX + 15
				width: parent.width - x
				height: 2
				color: dropArea.dropDelta === -1 ? Theme.dropTargetColor : "transparent"
			}
			Rectangle {
				x: model.expanded === true ? offsetX + 35 : offsetX + 15
				width: parent.width - x
				height: 2
				y: parent.height - height
				color: dropArea.dropDelta === 1 ? Theme.dropTargetColor : "transparent"
			}

			DropArea {
				id: dropArea
				anchors.fill: parent

				property int dropDelta : -2
				property bool draggedRows : false
				property var mimeData: null

				function update(y) {
					if (draggedRows) {
						var dy = dropArea.height >> 2
						dropDelta = y < dy ? -1 : (y > dropArea.height - dy ? 1 : 0)
					} else {
						dropDelta = 0
					}
				}

				onEntered: {
					mimeData = makeMime(drag)
					draggedRows = !!mimeData["miray/dragged-rows"]
					console.log("onEntered", mimeData)
					update(drag.y)
					drag.accepted = sceneTreePresenter.canDropMimeData(mimeData, drag.hasUrls ? drag.urls[0] : "", index, dropDelta)
					if (!drag.accepted) {
						dropDelta = -2
					}
				}
				onPositionChanged: {
					update(drag.y)
					drag.accepted = sceneTreePresenter.canDropMimeData(mimeData, drag.hasUrls ? drag.urls[0] : "", index, dropDelta)
					if (!drag.accepted) {
						dropDelta = -2
					}
				}
				onDropped:  {
					var mimeData = makeMime(drop)
					var dd = dropDelta
					dropDelta = -2
					var url = drop.hasUrls ? drop.urls[0] : ""
					if (sceneTreePresenter.canDropMimeData(mimeData, url, index, dd)) {
						drop.accept()
						setTimeout(function() {
							sceneTreePresenter.dropMimeData(mimeData, url, index, dd)
						}, 100);
					}
				}
				onExited: {
					dropDelta = -2
				}
			}

			function canEditName() {
				return model.material === undefined
			}
			function editName() {
				nameEdit.start()
			}

			Keys.onPressed: {
				if (event.key === Qt.Key_Space && model.visible !== undefined) {
					event.accepted = true
					model.visible = !model.visible;
					return;
				}

				if (event.key === Qt.Key_Right && model.expanded === false) {
					event.accepted = true
					model.expanded = true;
				}

				if (event.key === Qt.Key_Left && model.expanded === true) {
					event.accepted = true
					model.expanded = false;
				}

				if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_F2) {
					event.accepted = true
					editName()
				}
			}
		} // delegate: Rectangle

		footer: Rectangle {
			x: 15
			width: parent.width - x
			height: 2
			color: globalDropArea.accepted ? Theme.dropTargetColor : "transparent"
		}

		DropArea {
			id: globalDropArea
			property bool accepted : false
			z: -1
			anchors.fill: parent

			onEntered: {
				var mimeData = makeMime(drag)
				drag.accepted = accepted = !!mimeData["miray/dragged-rows"] &&
					sceneTreePresenter.canDropMimeData(mimeData, "", treeView.count, 1)
			}
			onDropped: {
				if (accepted) {
					var mimeData = makeMime(drop)
					drop.accept()
					setTimeout(function() {
						sceneTreePresenter.dropMimeData(mimeData, "", treeView.count, 1)
					}, 100);
				}
				accepted = false
			}
			onExited: {
				accepted = false
			}
		}
	} // ListView

	function makeMime(event) {
		var res = {};
		event.formats.forEach(function(format) {
			res[format] = event.getDataAsString(format);
		});
		return res;
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

	QuickControls1.Menu {
		id: contextMenu
		QuickControls1.MenuItem {
			text: "Cut"
			shortcut: StandardKey.Cut
			enabled: presenter.hasSelection
			onTriggered: presenter.editCut()
		}
		QuickControls1.MenuItem {
			text: "Copy"
			shortcut: StandardKey.Copy
			enabled: presenter.hasSelection
			onTriggered: presenter.editCopy()
		}
		QuickControls1.MenuItem {
			text: "Paste"
			shortcut: StandardKey.Paste
			enabled: presenter.hasCopiedNodes
			onTriggered: presenter.editPaste()
		}
		QuickControls1.MenuItem {
			text: "Delete"
			shortcut: StandardKey.Delete
			enabled: presenter.hasSelection
			onTriggered: presenter.editDelete()
		}
		QuickControls1.MenuItem {
			text: "Rename"
			shortcut: "F2"
			enabled: !!treeView.currentItem && treeView.currentItem.canEditName()
			onTriggered: treeView.currentItem.editName()
		}
		QuickControls1.MenuSeparator { }
		QuickControls1.MenuItem {
			text: "Extract Meshes"
			enabled: presenter.hasGeomSelection
			onTriggered: presenter.extractMeshes()
		}
		QuickControls1.MenuItem {
			text: "Combine Meshes"
			enabled: presenter.canCombineMeshes
			onTriggered: presenter.combineMeshes()
		}
		QuickControls1.MenuItem {
			text: "Group Selected"
			enabled: presenter.canGroupSelected
			onTriggered: presenter.groupSelected()
		}
		QuickControls1.MenuSeparator { }
		QuickControls1.MenuItem {
			text: "Metadata Editor..."
			enabled: presenter.canOpenMetadataEditor
			onTriggered: runMetadataEditor()
		}
		QuickControls1.MenuSeparator { }
		QuickControls1.MenuItem {
			text: "Expand Selected"
			enabled: presenter.hasCommonNodeSelection
			onTriggered: sceneTreePresenter.expandSelected(false)
		}
		QuickControls1.MenuItem {
			text: "Expand Selected and Children"
			enabled: presenter.hasCommonNodeSelection
			onTriggered: sceneTreePresenter.expandSelected(true)
		}
		QuickControls1.MenuItem {
			text: "Collapse Selected"
			enabled: presenter.hasCommonNodeSelection
			onTriggered: sceneTreePresenter.collapseSelected(true)
		}
	}
}
