import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Shapes 1.15
import QtQuick.Dialogs 1.3
import FbItem 1.0
import "." as Internal

Item {
	anchors.fill: parent
	//color: "transparent"
	//border.color: "#ff0000"

	ColumnLayout {
		anchors.fill: parent
		spacing: 0

		ToolBar {
			//color: "green"
			Layout.fillWidth: true
			Layout.preferredHeight: 48
			style: ToolBarStyle {
				padding {
					left: 0
					right: 0
					top: 0
					bottom: 0
				}
				background: Rectangle {
					//border.color: "#999"
					color: Theme.toolBarColor
					//gradient: Gradient {
					//	GradientStop { position: 0 ; color: "#f2f3f6" }
					//	GradientStop { position: 1 ; color: "#f2f3f6" }
					//}
				}
			}

			Rectangle {
				color: Theme.edgeColor
				width: parent.width
				height: 1
				y: parent.height - 1
			}

			RowLayout {
				anchors.left: parent.left
				height: parent.height
				TabViewButton {
					tabView: leftTabView
					index: 0
					iconNormal: "../toolbar/panel-materials-normal.png"
					iconActive: "../toolbar/panel-materials-pressed.png"
					iconHover: "../toolbar/panel-materials-hover.png"
					iconSelected: "../toolbar/panel-materials-pressed.png"
					tooltipText: "Materials Library"
				}
				TabViewButton {
					tabView: leftTabView
					index: 1
					iconNormal: "../toolbar/panel-envmaps-normal.png"
					iconActive: "../toolbar/panel-envmaps-pressed.png"
					iconHover: "../toolbar/panel-envmaps-hover.png"
					iconSelected: "../toolbar/panel-envmaps-pressed.png"
					tooltipText: "Environment Maps Library"
				}
				TabViewButton {
					tabView: leftTabView
					index: 2
					iconNormal: "../toolbar/panel-textures-normal.png"
					iconActive: "../toolbar/panel-textures-pressed.png"
					iconHover: "../toolbar/panel-textures-hover.png"
					iconSelected: "../toolbar/panel-textures-pressed.png"
					tooltipText: "Textures Library"
				}
				TabViewButton {
					tabView: leftTabView
					index: 3
					iconNormal: "../toolbar/panel-shapes-normal.png"
					iconActive: "../toolbar/panel-shapes-pressed.png"
					iconHover: "../toolbar/panel-shapes-hover.png"
					iconSelected: "../toolbar/panel-shapes-pressed.png"
					tooltipText: "Shapes Library"
				}
				TabViewButton {
					//enabled: !!sceneTreePresenter
					tabView: leftTabView
					index: 4
					iconNormal: "../toolbar/panel-scene-tree-normal.png"
					iconActive: "../toolbar/panel-scene-tree-pressed.png"
					iconHover: "../toolbar/panel-scene-tree-hover.png"
					iconSelected: "../toolbar/panel-scene-tree-pressed.png"
					tooltipText: "Scene Tree"
				}
			}
			RowLayout {
				id: gizmoButtons
				anchors.horizontalCenter: parent.horizontalCenter
				height: parent.height
				ToolToggleButton {
					enabled: !!sceneTreeModel
					pressed: presenter.gizmo === 0
					iconPressed: "../toolbar/gizmo-select-pressed.png"
					iconNormal: "../toolbar/gizmo-select-normal.png"
					iconHover: "../toolbar/gizmo-select-hover.png"
					iconDisabled: "../toolbar/gizmo-select-disabled.png"
					tooltipText: "Selection"
					onClicked: presenter.gizmo = 0
				}
				ToolToggleButton {
					enabled: !!sceneTreeModel
					pressed: presenter.gizmo === 1
					iconPressed: "../toolbar/gizmo-move-pressed.png"
					iconNormal: "../toolbar/gizmo-move-normal.png"
					iconHover: "../toolbar/gizmo-move-hover.png"
					iconDisabled: "../toolbar/gizmo-move-disabled.png"
					tooltipText: "Translation"
					onClicked: presenter.gizmo = 1
				}
				ToolToggleButton {
					enabled: !!sceneTreeModel
					pressed: presenter.gizmo === 2
					iconPressed: "../toolbar/gizmo-rotate-pressed.png"
					iconNormal: "../toolbar/gizmo-rotate-normal.png"
					iconHover: "../toolbar/gizmo-rotate-hover.png"
					iconDisabled: "../toolbar/gizmo-rotate-disabled.png"
					tooltipText: "Rotation"
					onClicked: presenter.gizmo = 2
				}
				ToolToggleButton {
					enabled: !!sceneTreeModel
					pressed: presenter.gizmo === 3
					iconPressed: "../toolbar/gizmo-scale-pressed.png"
					iconNormal: "../toolbar/gizmo-scale-normal.png"
					iconHover: "../toolbar/gizmo-scale-hover.png"
					iconDisabled: "../toolbar/gizmo-scale-disabled.png"
					tooltipText: "Scaling"
					onClicked: presenter.gizmo = 3
				}
				ToolToggleButton {
					enabled: !!sceneTreeModel
					pressed: presenter.gizmo === 4
					iconPressed: "../toolbar/gizmo-camera-center-pressed.png"
					iconNormal: "../toolbar/gizmo-camera-center-normal.png"
					iconHover: "../toolbar/gizmo-camera-center-hover.png"
					iconDisabled: "../toolbar/gizmo-camera-center-disabled.png"
					tooltipText: "Move camera target"
					onClicked: presenter.gizmo = 4
				}

				Rectangle {
					width: 1
					y: 0
					height: 48
					color: Theme.edgeColor
				}
				ToolToggleButton {
					enabled: !!sceneTreeModel
					pressed: false
					iconPressed: "../toolbar/tool-fit-to-view-pressed.png"
					iconNormal: "../toolbar/tool-fit-to-view-normal.png"
					iconHover: "../toolbar/tool-fit-to-view-hover.png"
					iconDisabled: "../toolbar/tool-fit-to-view-disabled.png"
					tooltipText: "Fit to view"
					onClicked: runFitToView()
				}
				ToolToggleButton {
					enabled: presenter.hasCommonNodeSelection
					pressed: false
					iconPressed: "../toolbar/tool-put-on-floor-pressed.png"
					iconNormal: "../toolbar/tool-put-on-floor-normal.png"
					iconHover: "../toolbar/tool-put-on-floor-hover.png"
					iconDisabled: "../toolbar/tool-put-on-floor-disabled.png"
					tooltipText: "Drop to surface"
					onClicked: presenter.dropToSurface()
				}
				Rectangle {
					width: 1
					y: 0
					height: 48
					color: Theme.edgeColor
				}
				ToolToggleButton {
					enabled: !!sceneTreeModel
					pressed: false
					iconPressed: "../toolbar/tool-render-pressed.png"
					iconNormal: "../toolbar/tool-render-normal.png"
					iconHover: "../toolbar/tool-render-hover.png"
					iconDisabled: "../toolbar/tool-render-disabled.png"
					tooltipText: "Render scene"
					onClicked: runRenderingParameters()
				}
			}
			RowLayout {
				anchors.right: parent.right
				height: parent.height
				TabViewButton {
					tabView: rightTabView
					index: 0
					iconNormal: "../toolbar/panel-node-normal.png"
					iconActive: "../toolbar/panel-node-pressed.png"
					iconHover: "../toolbar/panel-node-hover.png"
					iconSelected: "../toolbar/panel-node-pressed.png"
					tooltipText: "Node Parameters"
				}
				TabViewButton {
					tabView: rightTabView
					index: 1
					iconNormal: "../toolbar/panel-scene-materials-normal.png"
					iconActive: "../toolbar/panel-scene-materials-pressed.png"
					iconHover: "../toolbar/panel-scene-materials-hover.png"
					iconSelected: "../toolbar/panel-scene-materials-pressed.png"
					tooltipText: "Scene Materials"
				}
				TabViewButton {
					tabView: rightTabView
					index: 2
					iconNormal: "../toolbar/panel-camera-normal.png"
					iconActive: "../toolbar/panel-camera-pressed.png"
					iconHover: "../toolbar/panel-camera-hover.png"
					iconSelected: "../toolbar/panel-camera-pressed.png"
					tooltipText: "Camera"
				}
				TabViewButton {
					tabView: rightTabView
					index: 3
					iconNormal: "../toolbar/panel-scene-settings-normal.png"
					iconActive: "../toolbar/panel-scene-settings-pressed.png"
					iconHover: "../toolbar/panel-scene-settings-hover.png"
					iconSelected: "../toolbar/panel-scene-settings-pressed.png"
					tooltipText: "Scene Settings"
				}
				TabViewButton {
					tabView: rightTabView
					index: 4
					iconNormal: "../toolbar/panel-snapshots-normal.png"
					iconActive: "../toolbar/panel-snapshots-pressed.png"
					iconHover: "../toolbar/panel-snapshots-hover.png"
					iconSelected: "../toolbar/panel-snapshots-pressed.png"
					tooltipText: "Snapshots"
				}
			}
		}

		RowLayout {
			//anchors.fill: parent
			Layout.fillWidth: true
			Layout.fillHeight: true
			spacing: 0

			Item {
				width: 300
				Layout.fillHeight: true

				TabView {
					id: leftTabView
					anchors.fill: parent
					currentIndex: 3 // default to "Shapes" tab

					Tab {
						title: "Materials"
						CollectionView {
							dataModel: materialsDataModel
							treeModel: materialsTreeModel
						}
					}
					Tab {
						title: "Environment Maps"
						CollectionView {
							dataModel: environmentsDataModel
							treeModel: environmentsTreeModel
						}
					}
					Tab {
						title: "Textures"
						CollectionView {
							dataModel: texturesDataModel
							treeModel: texturesTreeModel
						}
					}
					Tab {
						title: "Shapes"
						CollectionView {
							dataModel: shapesDataModel
							treeModel: shapesTreeModel
						}
					}
					Tab {
						enabled: !!sceneTreePresenter
						title: "Scene Tree"
						SceneTree {
							objectName: "sceneTree"
						}
					}

					style: TabViewStyle {
						tab: null
						frame: Rectangle {
							color: "#fff"
							Text {
								//anchors.horizontalCenter: parent.horizontalCenter
								x: 10
								y: 10
								text: leftTabView.count >= 0 ? leftTabView.getTab(leftTabView.currentIndex).title : ""
								color: leftTabView.count >= 0 && leftTabView.getTab(leftTabView.currentIndex).enabled ? "#000" : "#ccc"
								font: Theme.groupTitleFont
							}
							// Rectangle {
							// 	y: 32
							// 	width: parent.width
							// 	height: 1
							// 	color: "#ccc"
							// }
						}
					}

					onCurrentIndexChanged: presenter.onLeftTabActivated(currentIndex)
				}
			} // Left Bar

			Rectangle {
				color: Theme.edgeColor
				width: 1
				Layout.fillHeight: true
			}

			Item {
				id: centerView
				Layout.fillWidth: true
				Layout.fillHeight: true

				TabView { // Center View
					id: mainTabView
					anchors.fill: parent

					Image {
						visible: mainTabView.count === 0
						fillMode: Image.Pad
						anchors.fill: parent
						anchors.centerIn: parent
						anchors.bottomMargin: 50
						scale: 0.5
						source: "../empty-view-image.png"
					}

					Text {
						visible: mainTabView.count === 0
						text: "Drop 3D Model Here to Begin"
						font.family: "Verdana"
						font.pixelSize: 24
						color: Theme.paramTitleEnabled
						anchors.horizontalCenter: parent.horizontalCenter
						y: parent.height / 2 + 110
						verticalAlignment: Text.AlignVCenter
					}

					Component { // Viewport
						id: glView
						FbItem {
							focus: true
							anchors.fill: parent

							onVisibleChanged: if (visible) { presenter.activateTab(uuid) }
							Component.onCompleted: presenter.activateTab(uuid)
						}
					}

					// Shortcut {
					// 	sequence: StandardKey.PreviousChild
					// 	onActivated: { console.log("PreviousChild"); previousTab() }
					// }
					// Shortcut {
					// 	sequence: StandardKey.NextChild
					// 	onActivated: { console.log("NextChild"); nextTab() }
					// }

					style: TabViewStyle {
						frameOverlap: 0
						tabOverlap: 1
						tabsMovable: true
						tab: Rectangle {
							implicitWidth: 140
							implicitHeight: 25
							color: styleData.selected ? Theme.mainColor : styleData.pressed ? "#d4d4d4" : styleData.hovered ? "#f6f6f6" : "#ececec"
							property bool modified: false

							CustomToolTip {
								visible: styleData.hovered
								text: styleData.title
							}

							Text {
								id: text
								anchors.fill: parent
								anchors.leftMargin: Theme.tabHMargins
								anchors.rightMargin: Theme.tabHMargins * 2 + 12
								verticalAlignment: Text.AlignVCenter
								text: styleData.title
								elide: Text.ElideRight
								font: Theme.labelFont
								color: styleData.selected ? "white" : "#464646"
							}

							ToolToggleButton {
								property string iconName: "../tab-close-" + (modified ? "modified" : "unmodified") + "-" + (styleData.selected ? "active" : "inactive")
								anchors.right: parent.right
								anchors.rightMargin: Theme.tabHMargins
								anchors.verticalCenter: parent.verticalCenter
								width: 12
								iconPressed: iconName + "-pressed.png"
								iconNormal: iconName + "-normal.png"
								iconHover: iconName + "-hover.png"
								iconDisabled: iconName + "-normal.png"
								onClicked: {
									mainTabView.currentIndex = styleData.index
									presenter.closeCurrentTab()
								}
							}

							Rectangle {
								x: parent.width - 1
								height: parent.height
								width: 1
								color: styleData.selected ? "transparent" : "#C8CACE"
							}

							Component.onCompleted: {
								presenter.undoChanged.connect(function() {
									if (mainTabView && mainTabView.currentIndex === styleData.index) {
										modified = presenter.undoEnabled
									}
								})
							}
						}
						frame: Rectangle {
							color: Theme.emptyPreviewColor
						}
						tabBar: Rectangle {
							color: Theme.emptyPreviewColor
							DropArea {
								anchors.fill: parent
								onEntered: {
									if (presenter.onDragEntered(drag.x, drag.y, drag.urls[0], true)) {
										drag.acceptProposedAction()
									}
								}
								onPositionChanged: {
									if (presenter.onDragPositionChanged(drag.x, drag.y, drag.urls[0], true)) {
										drag.acceptProposedAction()
									} else {
										drag.accept(Qt.IgnoreAction)
									}
								}
								onDropped: {
									if (presenter.onDropped(drop.x, drop.y, drop.urls[0], true)) {
										drop.accept()
									}
								}
							}
						}
					}

					DropArea {
						anchors.fill: parent
						onEntered: {
							//console.log("onEntered", drag.x, drag.y, drag.source, drag.urls)
							if (presenter.onDragEntered(drag.x, drag.y, drag.urls[0], false)) {
								drag.acceptProposedAction()
							}
						}
						onPositionChanged: {
							//console.log("positionChanged", drag.x, drag.y, drag.source, drag.urls)
							if (presenter.onDragPositionChanged(drag.x, drag.y, drag.urls[0], false)) {
								drag.acceptProposedAction()
							} else {
								drag.accept(Qt.IgnoreAction)
							}
						}
						onDropped: {
							//console.log("onDropped", drop.x, drop.y, drop.urls[0])
							if (presenter.onDropped(drop.x, drop.y, drop.urls[0], false)) {
								drop.accept()
							}
						}
					}

					function previousTab() {
						currentIndex = currentIndex > 0 ? currentIndex - 1 : count - 1
					}

					function nextTab() {
						currentIndex = currentIndex < count - 1 ? currentIndex + 1 : 0
					}

					function setCurrentTab(uuid) {
						for (var i = 0; i < count; i++) {
							if (getTab(i).item.uuid === uuid) {
								currentIndex = i;
							}
						}
					}
				} // TabView
			}

			Rectangle {
				color: Theme.edgeColor
				width: 1
				Layout.fillHeight: true
			}

			Item {
				width: 278
				Layout.fillHeight: true

				TabView {
					id: rightTabView
					anchors.fill: parent

					Tab {
						enabled: !!nodeModel
						NodeSettings {}
					}
					Tab {
						enabled: !!materialsList
						MaterialSettings {}
					}
					Tab {
						enabled: !!cameraModel
						CameraSettings {}
					}
					Tab {
						enabled: !!sceneModel
						SceneSettings {}
					}
					Tab {
						enabled: !!snapshotsList
						SnapshotSettings {
							objectName: "snapshotSettings"
						}
					}
					style: TabViewStyle {
						tab: null
						// tab: Rectangle {
						// 	color: styleData.selected ? "steelblue" :"lightsteelblue"
						// 	border.color:  "steelblue"
						// 	implicitWidth: 48// Math.max(text.width + 4, 80)
						// 	implicitHeight: 48
						// 	radius: 2
						// 	Text {
						// 		id: text
						// 		anchors.centerIn: parent
						// 		text: styleData.title
						// 		color: styleData.selected ? "white" : "black"
						// 	}
						// }
						frame: Rectangle {
							color: "#fff"
							// Text {
							// 	//anchors.horizontalCenter: parent.horizontalCenter
							// 	visible: !!text
							// 	x: 10
							// 	y: 10
							// 	text: rightTabView.count >= 0 ? rightTabView.getTab(rightTabView.currentIndex).title : ""
							// 	color: rightTabView.count >= 0 && rightTabView.getTab(rightTabView.currentIndex).enabled ? "#000" : "#ccc"
							// 	font: Theme.groupTitleFont
							// }
							// Rectangle {
							// 	y: 32
							// 	width: parent.width
							// 	height: 1
							// 	color: "#ccc"
							// }
						}
					}

					onCurrentIndexChanged: {
						presenter.onRightTabActivated(currentIndex)
					}
				}
			}
		}
	}

	// Window overlays and dialog instantiations removed
	// Trigger actions via the presenter instead

	TextureDialog {
		id: textureDialogWindow
		x: parent ? parent.width - width : 0
		y: parent ? (parent.height - height) / 2 : 0
		margins: 0
	}

	FitToViewWindow {
		id: fitToView
	}

	AppSettingsWindow {
		id: appSettingsWindow
	}

	EditNormalsWindow {
		id: editNormals
	}

	UVMappingWindow {
		id: uvMapping
	}

	PivotParametersWindow {
		id: pivotParametersWindow
	}

	RenderingParameters {
		id: renderingParameters
	}

	RenderingWindow {
		id: renderingWindow
	}

	ResourceManagerWindow {
		id: resourceManagerWindow
	}

	RenderingLayersManagerWindow {
		id: renderingLayersManagerWindow
	}

	MessageBox {
		id: messageBoxWindow
	}

	function openTextureDialog(texture) {
		textureDialogWindow.model = texture
		textureDialogWindow.open()
	}

	function runRenderingParameters() {
		renderingParameters.open()
	}

	function runRendering() {
		renderingWindow.open()
	}

	function runFitToView() {
		fitToView.open()
	}

	function runEditNormals() {
		editNormals.open()
	}

	function runUVMapping() {
		uvMapping.open()
	}

	function runPivotParameters() {
		pivotParametersWindow.open()
	}

	function runMetadataEditor(index) {
		presenter.beginNodeMetadata()
	}

	function runResourceManager() {
		resourceManagerWindow.open()
	}

	function runRenderingLayersManager() {
		renderingLayersManagerWindow.open()
	}

	function runAppSettings() {
		appSettingsWindow.open()
	}

	function runAbout() {
		presenter.beginAbout()
	}


	function showMessage(title, text, callback) {
		messageBoxWindow.title = title
		messageBoxWindow.messageText = text
		messageBoxWindow.onAcceptedCallback = callback
		messageBoxWindow.open()
	}

	function createTab(name) {
		console.log(">>> createTab", mainTabView.count, name)
		mainTabView.addTab(name, glView)
		mainTabView.currentIndex = Math.max(0, mainTabView.count - 1)
	}

	function removeCurrentTab() {
		console.log(">>> removeCurrentTab", mainTabView.currentIndex)
		mainTabView.removeTab(mainTabView.currentIndex)
	}

	function previousTab() {
		mainTabView.previousTab()
	}

	function nextTab() {
		mainTabView.nextTab()
	}

	function setCurrentTab(uuid) {
		mainTabView.setCurrentTab(uuid)
	}

	function setTabTitle(title) {
		mainTabView.getTab(mainTabView.currentIndex).title = title
	}

	function expandSceneTreeItem(index) {
		var sceneTreeTab = leftTabView.getTab(4).item
		if (sceneTreeTab) {
			sceneTreeTab.expandTree(index)
		}
	}

	function setMaterialLayer(index) {
		var materialTab = rightTabView.getTab(1).item
		if (materialTab) {
			materialTab.setLayer(index)
		}
	}

	function setMaterial(index) {
		var materialTab = rightTabView.getTab(1).item
		if (materialTab) {
			materialTab.setMaterial(index)
		}
	}

	function expandTree(index) {
		var materialTab = rightTabView.getTab(1).item
		if (materialTab) {
			materialTab.expandTree(index)
		}
	}

	function setSnapshot(index) {
		var snapshotTab = rightTabView.getTab(4).item
		if (snapshotTab) {
			snapshotTab.setSnapshot(index)
		}
	}

	function activateLeftTab(index) {
		leftTabView.currentIndex = index
	}

	function activateRightTab(index) {
		rightTabView.currentIndex = index
	}

	function onUpdateFound() {
		// updates checker popup not instantiated
	}

	function checkerForUpdates() {
		// updates checker popup not instantiated
	}

	ProgressPopup {
		id: progressPopup
	}

	function beginProgress(title) {
		progressPopup.title = title
		progressPopup.value = 0
		progressPopup.open()
	}

	function updateProgress(progress) {
		progressPopup.value = progress
	}

	function endProgress() {
		progressPopup.close()
	}
}
