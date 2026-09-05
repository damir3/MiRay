import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls 1.4 as QuickControls1
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import QtQml.Models 2.2

Item {
	property var dataModel
	property var treeModel

	anchors.fill: parent

	QuickControls1.SplitView {
		anchors.fill: parent
		orientation: Qt.Vertical

		handleDelegate: Rectangle {
			height: 4
			color: styleData.hovered || styleData.pressed ? Theme.mainColor : Theme.edgeColor
		}

		Item {
			implicitHeight: 190
			Layout.minimumHeight: 4 * Theme.rowHeight + 70
			Layout.maximumHeight: 14 * Theme.rowHeight + 70

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
				// border.color: "#C0C0C0"
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
				onTextChanged: dataModel.setFilter(text)
			}

			Rectangle {
				anchors.top: inputRect.bottom
				anchors.topMargin: 4
				width: parent.width
				height: 1
				color: "#ccc"
			}

			QuickControls1.TreeView {
				id: treeView
				focus: true
				width: parent.width
				anchors.top: inputRect.bottom
				anchors.bottom: parent.bottom
				anchors.topMargin: 5
				backgroundVisible: false

				model: treeModel
				selectionMode: QuickControls1.SelectionMode.SingleSelection
				selection: ItemSelectionModel {
					model: treeModel
					onCurrentChanged: {
						dataModel.setFolder(treeModel.data(current, 0x102))
						//treeView.forceActiveFocus()
					}
				}

				horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
				verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
				frameVisible: false
				headerVisible: false

				ScrollBar {
					id: vbar
					orientation: Qt.Vertical
					anchors.top: treeView.top
					anchors.right: treeView.right
					anchors.bottom: treeView.bottom
					position: treeView.flickableItem.contentY / treeView.flickableItem.contentHeight
					size: treeView.height / treeView.flickableItem.contentItem.height
					width: 10

					onPositionChanged: {
						//console.log("onPositionChanged", position, treeView.flickableItem.moving)
						active = true
						treeView.flickableItem.contentY = position * treeView.flickableItem.contentHeight
						active = false
					}
				}

				style: TreeViewStyle {
					branchDelegate: Image {
						visible: styleData.hasChildren
						anchors.verticalCenter: parent.verticalCenter
						source: styleData.selected ? "../triangle-sel.png" : "../triangle-unsel.png"
						rotation: styleData.isExpanded ? 90 : 0
						Behavior on rotation { NumberAnimation { duration: 100 } }
					}
					itemDelegate: Text {
						text: styleData.value
						verticalAlignment: Text.AlignVCenter
						font: Theme.rowFont
						color: styleData.selected ? "#fff" : "#000"
					}
					rowDelegate: Rectangle {
						color: Theme.rowColor(styleData.selected, /*styleData.hasActiveFocus*/true, styleData.row)
						height: Theme.rowHeight
					}
				}

				QuickControls1.TableViewColumn {
					role: "name"
				}
				/*itemDelegate: Text {
					text: styleData.value
					verticalAlignment: Text.AlignVCenter
					font: Theme.rowFont
					color: styleData.selected ? "#fff" : "#000"
				}
				rowDelegate: Rectangle {
					color: Theme.rowColor(styleData.selected, styleData.hasActiveFocus, styleData.row)
					height: Theme.rowHeight
				}*/
				Component.onCompleted: {
					var index = treeModel.index(0, 0)
					treeView.selection.setCurrentIndex(index, ItemSelectionModel.Select);
					treeView.expand(index)
				}
			}
		}

		Item {
			Layout.minimumHeight: 95
			Layout.fillHeight: true
			// Rectangle {
			// 	anchors.topMargin: 5
			// 	border.color: "#808080"
			// 	color: "transparent"
			// 	anchors.fill: parent
			// }

			GridView {
				id: view
				clip: true
				anchors.fill: parent
				// anchors.topMargin: 6
				anchors.leftMargin: (parent.width - cellWidth * Math.floor(parent.width / cellWidth)) / 2
				cellWidth: 95
				cellHeight: 110
				focus: false
				model: dataModel

				ScrollBar.vertical: ScrollBar {}

				header: Item {
					height: 12
				}

				delegate: Item {
					id: myItem
					width: view.cellWidth
					height: view.cellHeight
//					visible: itemNameFilter.text.length === 0 || name.toLowerCase().indexOf(itemNameFilter.text.toLowerCase()) !== -1

					Drag.active: dragArea.drag.active
					Drag.supportedActions: Qt.CopyAction
					Drag.dragType: Drag.Automatic
					Drag.mimeData: {
						"text/plain": name,
						"text/uri-list": shortPath
					}
					//Drag.hotSpot.x: 40
					//Drag.hotSpot.y: 40
					//Drag.imageSource: thumbnail
//					Drag.onDragStarted: {
//						view.currentIndex = index
//						console.log(index, model.index, view.currentIndex)
//					}
//					Drag.onDragFinished: console.log(Drag.target)

					Item {
						id: myThumbnail
						y: 6
						width: 80
						height: 80
						anchors.horizontalCenter: parent.horizontalCenter
						Image {
							anchors.fill: parent
							fillMode: Image.PreserveAspectFit
							anchors.centerIn: parent
							source: thumbnail
						}
					}
					Rectangle {
						id: titleRect
						visible: myItem.GridView.isCurrentItem
						anchors.horizontalCenter: parent.horizontalCenter
						y: 90
						width: myText.contentWidth + 10
						height: 20
						color: (myItem.activeFocus ? Theme.mainColor : Theme.mainColorInactive)
						radius: 4
					}
					Text {
						id: myText
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: titleRect.verticalCenter
						horizontalAlignment: Text.AlignHCenter
						text: name
						font.pixelSize: 12
						width: view.cellWidth - 10
						elide: Text.ElideRight
						color: myItem.GridView.isCurrentItem ? "#fff" : "#666"
					}
					MouseArea {
						id: dragArea
						anchors.fill: parent
						hoverEnabled: true
						drag.target: dragArea
						onPressed: {
							view.currentIndex = index
							if (view.currentItem) {
								view.currentItem.forceActiveFocus()
							}
						}
						//hoverEnabled: true
						//cursorShape: containsMouse ? Qt.OpenHandCursor : Qt.ArrowCursor
					}
					CustomToolTip {
						visible: dragArea.containsMouse && text
						text: localPath
					}
				}
			}
		}
	}
}
