import QtQuick 2.15
import QtQml 2.2
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls 1.4 as QuickControls1
import QtQml.Models 2.2

Item {
	property var currentMaterial : 0
	property var currentGroup : 0
	property var currentLayer : 0
	property int buttonSize: 20

	anchors.fill: parent

	Text {
		id: panelTitle
		x: Theme.margins
		y: Theme.margins
		text: "Scene Materials"
		color: enabled ? "#000" : "#ccc"
		font: Theme.groupTitleFont
	}

	Row {
		visible: !!materialsList
		anchors.verticalCenter: panelTitle.verticalCenter
		anchors.right: parent.right
		anchors.rightMargin: Theme.margins
		spacing: 4

		ToolToggleButton {
			width: 24
			enabled: mList.count > 0 && mList.currentIndex > -1
			iconPressed: "../button-minus-pressed.png"
			iconNormal: "../button-minus-normal.png"
			iconHover: "../button-minus-hover.png"
			iconDisabled: "../button-minus-disabled.png"
			tooltipText: "Delete Material"
			onClicked: materialPresenter.deleteMaterial()
		}

		ToolToggleButton {
			width: 24
			iconPressed: "../button-plus-pressed.png"
			iconNormal: "../button-plus-normal.png"
			iconHover: "../button-plus-hover.png"
			iconDisabled: "../button-plus-disabled.png"
			tooltipText: "Create Material"
			onClicked: materialPresenter.createMaterial()
		}
	}

	QuickControls1.SplitView {
		visible: !!materialsList
		enabled: visible

		anchors.fill: parent
		anchors.topMargin: 45
		orientation: Qt.Vertical

		handleDelegate: Rectangle {
			height: 4
			color: styleData.hovered || styleData.pressed ? Theme.mainColor : Theme.edgeColor
		}

		Item {
			enabled: !!materialsList && materialsList.hasChildren()
			width: parent.width
			Layout.minimumHeight: 4 * Theme.rowHeight + 1 + Theme.listVMargins * 2
			implicitHeight: 5 * Theme.rowHeight + 1 + Theme.listVMargins * 2

			Rectangle {
				width: parent.width
				height: 1
				color: Theme.edgeColor
			}

			ListView {
				id: mList
				property int dropIndex : -1

				anchors.fill: parent
				anchors.topMargin: 1
				header: Item { height: Theme.listVMargins }
				footer: Item {
					height: Theme.listVMargins
					Rectangle {
						visible: mList.dropIndex === mList.count
						width: mList.width
						height: 2
						color: Theme.dropTargetColor
					}
				}

				focus: true
				clip: true

				model: materialsList

				ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

				onCurrentIndexChanged: {
					if (materialPresenter) {
						materialPresenter.setMaterial(currentIndex)
					}
				}

				QuickControls1.Menu {
					id: materialsListMenu
					QuickControls1.MenuItem {
						text: "New"
						onTriggered: materialPresenter.create()
					}
					QuickControls1.MenuItem {
						text: "Delete"
						enabled: mList.currentIndex >= 0
						onTriggered: materialPresenter.deleteMaterial()
					}
					QuickControls1.MenuItem {
						text: "Clone"
						enabled: mList.currentIndex >= 0
						onTriggered: materialPresenter.cloneMaterial()
					}
					QuickControls1.MenuItem {
						text: "Rename"
						shortcut: "F2"
						enabled: mList.currentIndex >= 0 && !!mList.currentItem
						onTriggered: mList.currentItem.editName()
					}
					QuickControls1.MenuItem {
						text: "Save"
						enabled: mList.currentIndex >= 0
						onTriggered: materialPresenter.saveMaterial()
					}
					QuickControls1.MenuSeparator {}
					QuickControls1.MenuItem {
						text: "Select Geometries By Material"
						enabled: mList.currentIndex >= 0
						onTriggered: materialPresenter.selectGeometry()
					}
					QuickControls1.MenuItem {
						text: "Apply Material To Selected Geometries"
						enabled: mList.currentIndex >= 0
						onTriggered: materialPresenter.applyMaterial()
					}
					QuickControls1.MenuItem {
						text: "Remove Unused Materials"
						enabled: mList.count > 1
						onTriggered: materialPresenter.removeUnusedMaterials()
					}
				}

				DropArea {
					z: -1
					anchors.fill: parent
					onEntered: {
						drag.accepted = materialPresenter.canDropUrls(drag.urls, mList.count)
						mList.dropIndex = drag.accepted ? mList.count : -1
					}
					onPositionChanged: {
						mList.dropIndex = mList.count
					}
					onDropped: {
						var dropIndex = mList.count
						var urls = drop.urls;
						mList.dropIndex = -1
						if (materialPresenter.canDropUrls(urls, dropIndex)) {
							drop.accept()
							setTimeout(function() {
								materialPresenter.dropUrls(urls, dropIndex)
							}, 100);
						}
					}
					onExited: {
						mList.dropIndex = -1
					}
				}

				MouseArea {
					anchors.fill: parent
					z: -1
					onClicked: {
						mList.currentIndex = -1
						mList.parent.forceActiveFocus()
					}
				}

				MouseArea {
					anchors.fill: parent
					z: -1
					acceptedButtons: Qt.RightButton
					onClicked: materialsListMenu.popup()
				}

				delegate: Rectangle {
					property bool selected : index === mList.currentIndex
					x: Theme.listHMargins
					width: mList.width - Theme.listHMargins * 2
					height: Theme.rowHeight
					radius: 4
					color: Theme.rowColor(selected, activeFocus, index)

					MouseArea {
						id: dragArea
						anchors.fill: parent
						drag.target: dragArea
						onPressed: {
							mList.currentIndex = index
							if (mList.currentItem) {
								mList.currentItem.forceActiveFocus()
							}
						}
						onDoubleClicked: nameEdit.start()
					}

					MouseArea {
						anchors.fill: parent
						acceptedButtons: Qt.RightButton
						onClicked: {
							if (!selected) {
								mList.currentIndex = index
								if (mList.currentItem) {
									mList.currentItem.forceActiveFocus()
								}
							}
							materialsListMenu.popup()
						}
					}

					Drag.active: dragArea.drag.active
					Drag.supportedActions: Qt.CopyAction
					Drag.dragType: Drag.Automatic
					Drag.mimeData: {
						"text/plain": model.name.value,
						"text/uri-list": "miray://material/" + model.name.value
					}

					Text {
						visible: !nameEdit.visible
						x: Theme.listPaddings
						width: parent.width - Theme.listPaddings * 2
						height: parent.height
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight

						text: model.name.value
						font: Theme.rowFont
						color: selected ? Theme.selectionTextColor : "#000"
					}

					TextField {
						id: nameEdit
						visible: false
						anchors.fill: parent
						verticalAlignment: TextInput.AlignVCenter
						font: Theme.rowFont
						text: model.name.value
						clip: true
						leftPadding: Theme.listPaddings
						rightPadding: Theme.listPaddings
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

						onEditingFinished: stop(true)
						Keys.onReturnPressed: stop(true)
						Keys.onTabPressed: stop(true)
						Keys.onBacktabPressed: stop(true)
						Keys.onEscapePressed: stop(false)

						function start() {
							if (model.name.enabled) {
								text = model.name.value
								visible = true
								selectAll()
								forceActiveFocus()
							}
						}

						// Note: This is an internal helper called when editing names of materials.
						function stop(accept) {
							if (accept && text.trim()) {
								model.name.value = text
							}
							visible = false
							if (mList.currentItem) {
								mList.currentItem.forceActiveFocus()
							}
						}
					}

					function editName() {
						nameEdit.start()
					}

					Keys.onPressed: {
						if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_F2) {
							event.accepted = true
							editName()
						}
					}

					Rectangle {
						visible: mList.dropIndex === index
						width: parent.width
						height: 2
						color: Theme.dropTargetColor
					}

					DropArea {
						id: dropArea
						anchors.fill: parent

						onEntered: {
							var dropIndex = Math.max(drag.y < (height >> 1) ? index : index + 1, 1)
							drag.accepted = materialPresenter.canDropUrls(drag.urls, dropIndex)
							mList.dropIndex = drag.accepted ? dropIndex : -1
							if (!drag.accepted) {
								mList.currentIndex = index
							}
						}
						onPositionChanged: {
							mList.dropIndex = Math.max(drag.y < (height >> 1) ? index : index + 1, 1)
						}
						onDropped: {
							var dropIndex = mList.dropIndex
							var urls = drop.urls
							mList.dropIndex = -1
							if (materialPresenter.canDropUrls(urls, dropIndex)) {
								drop.accept()
								setTimeout(function() {
									materialPresenter.dropUrls(urls, dropIndex)
								}, 100);
							}
						}
						onExited: {
							mList.dropIndex = -1
						}
					}
				} // delegate: Rectangle
			} // ListView
		}

		Flickable {
			visible: mList.count > 0 && mList.currentIndex >= 0
			Layout.minimumHeight: 200
			width: parent.width
			clip: true

			contentHeight: paramsLayout.height + 50

			ScrollBar.vertical: ScrollBar {
				policy: ScrollBar.AsNeeded
			}

			ColumnLayout {
				id: paramsLayout
				width: parent.width
				spacing: 0

				RowLayout {
					id: materialPreviewArea
					enabled: !!material
					Layout.fillWidth: true
					Layout.preferredHeight: 245 + 2
					spacing: 4

					ColumnLayout {
						Layout.alignment: Qt.AlignLeft | Qt.AlignTop
						Layout.leftMargin: 4
						Layout.topMargin: 4
						spacing: 4

						ToolToggleButton {
							width: 24
							height: 24
							iconPressed: "../button-update-pressed.png"
							iconNormal: "../button-update-normal.png"
							iconHover: "../button-update-hover.png"
							iconDisabled: "../button-update-disabled.png"
							tooltipText: "Update Preview"
							onClicked: materialPresenter.updateThumbnail()
						}

						ToolToggleButton {
							id: previewSceneBtn
							width: 24
							height: 24
							iconPressed: "../button-preview-pressed.png"
							iconNormal: "../button-preview-normal.png"
							iconHover: "../button-preview-hover.png"
							iconDisabled: "../button-preview-disabled.png"
							tooltipText: "Preview Scene"
							onClicked: previewMenu.popup()

							Menu {
								id: previewMenu
								Instantiator {
									model: materialPresenter ? materialPresenter.getPreviewModels() : []
									MenuItem {
										text: modelData
										onTriggered: {
											materialPresenter.setPreviewModel(index)
										}
									}
									onObjectAdded: previewMenu.insertItem(index, object)
									onObjectRemoved: previewMenu.removeItem(object)
								}
							}
						}
					}

					Item {
						Layout.fillWidth: true
						Layout.fillHeight: true

						Image {
							id: preview
							anchors.centerIn: parent
							width: parent.height - 4
							height: parent.height - 4
							fillMode: Image.PreserveAspectFit
							cache: false
							source: materialThumbnail != null ? materialThumbnail : "../no_material_preview.png"
						}

						Rectangle {
							color: "transparent"
							border.color: "#808080"
							anchors.centerIn: parent
							width: preview.paintedWidth + 2
							height: preview.paintedHeight + 2
						}

						CustomProgressBar {
							id: previewProgressBar
							visible: value < 1
							value: materialPresenter ? materialPresenter.previewProgress : 1
							showPercentage: false
							height: 8
							anchors.bottom: preview.bottom
							anchors.bottomMargin: 8
							anchors.horizontalCenter: preview.horizontalCenter
							width: Math.max(120, preview.paintedWidth - 16)
						}
					}
				}

				Item {
					Layout.fillWidth: true
					height: 4 * Theme.rowHeight + 8 + Theme.listVMargins * 2

					Rectangle {
						anchors.top: parent.top
						width: parent.width
						height: 1
						color: Theme.edgeColor
					}
					Rectangle {
						anchors.bottom: parent.bottom
						width: parent.width
						height: 1
						color: Theme.edgeColor
					}

					RowLayout {
						anchors.fill: parent
						anchors.leftMargin: 4
						anchors.rightMargin: 4
						spacing: 4

						ColumnLayout {
							Layout.alignment: Qt.AlignTop
							Layout.topMargin: 4
							spacing: 4
							width: 24

							ToolToggleButton {
								width: 24
								height: 24
								enabled: currentMaterial !== 0 || currentGroup !== 0
								iconPressed: "../button-plus-pressed.png"
								iconNormal: "../button-plus-normal.png"
								iconHover: "../button-plus-hover.png"
								iconDisabled: "../button-plus-disabled.png"
								tooltipText: currentMaterial !== 0 ? "Add Group" : "Add Layer"
								onClicked: materialPresenter.editAdd(treeView.selection.currentIndex)
							}
							ToolToggleButton {
								width: 24
								height: 24
								enabled: currentLayer !== 0 || currentGroup !== 0
								iconPressed: "../button-minus-pressed.png"
								iconNormal: "../button-minus-normal.png"
								iconHover: "../button-minus-hover.png"
								iconDisabled: "../button-minus-disabled.png"
								tooltipText: currentLayer !== 0 ? "Delete Layer" : "Delete Group"
								onClicked: materialPresenter.editDelete(treeView.selection.currentIndex)
							}

							ToolToggleButton {
								width: 24
								height: 24
								enabled: (currentLayer !== 0 || currentGroup !== 0) && (treeView.selection.currentIndex.row > 0)
								iconPressed: "../button-up-pressed.png"
								iconNormal: "../button-up-normal.png"
								iconHover: "../button-up-hover.png"
								iconDisabled: "../button-up-disabled.png"
								tooltipText: "Move Up"
								onClicked: materialPresenter.editMoveUp(treeView.selection.currentIndex)
							}

							ToolToggleButton {
								width: 24
								height: 24
								enabled: (currentLayer !== 0 || currentGroup !== 0) && (treeView.selection.currentIndex.row + 1 < (material ? material.rowCount(treeView.selection.currentIndex.parent) : 0))
								iconPressed: "../button-down-pressed.png"
								iconNormal: "../button-down-normal.png"
								iconHover: "../button-down-hover.png"
								iconDisabled: "../button-down-disabled.png"
								tooltipText: "Move Down"
								onClicked: materialPresenter.editMoveDown(treeView.selection.currentIndex)
							}
						}

						QuickControls1.TreeView {
							id: treeView
							focus: false
							frameVisible: false
							Layout.fillWidth: true
							Layout.fillHeight: true
							Layout.topMargin: 1
							Layout.bottomMargin: 1
							model: material
							headerVisible: false
							selectionMode: QuickControls1.SelectionMode.SingleSelection

							rowDelegate: Item {
								height: Theme.rowHeight

								Rectangle {
									width: treeView.width
									height: parent.height
									color: Theme.rowColor(styleData.selected, true, styleData.row)
									radius: 4
								}
							}

							onModelChanged: {
								expandTree(selection.currentIndex);
							}

							Component.onCompleted: {
								expandTree(selection.currentIndex);
								flickableItem.topMargin = 4
								flickableItem.bottomMargin = 4
							}

							selection: ItemSelectionModel {
								model: material
								onCurrentChanged: {
									materialPresenter.setEditIndex(currentIndex);
									var data = material.data(currentIndex, 0x101);
									var type = material.data(currentIndex, 0x102);
									currentMaterial = type === 1 ? data : 0
									currentGroup = type === 2 ? data : 0
									currentLayer = type === 3 ? data : 0
									if (currentMaterial) {
										currentMaterial.doubleSided.changed();
										currentMaterial.medium.changed();
										currentMaterial.subsurfaceScattering.changed();
										currentMaterial.emissionColor.changed();
										currentMaterial.emissionScale.changed();
									}
									if (currentGroup) {
										currentGroup.name.changed();
										currentGroup.mask.changed();
									}
									if (currentLayer) {
										currentLayer.name.changed();
										currentLayer.mask.changed();
										currentLayer.bump.changed();
										currentLayer.diffuseLayer.changed();
										currentLayer.diffuseColor.changed();
										currentLayer.diffuseOpacity.changed();
										currentLayer.diffuseTransmission.changed();
										currentLayer.diffuseTextureLayerMask.changed();
										currentLayer.emissiveLayer.changed();
										currentLayer.emissiveColor.changed();
										currentLayer.emissiveIntensity.changed();
										currentLayer.specularLayer.changed();
										currentLayer.iorType.changed();
										currentLayer.iorN.changed();
										currentLayer.iorK.changed();
										currentLayer.iorFilename.changed();
										currentLayer.reflection.changed();
										currentLayer.transmission.changed();
										currentLayer.reflection90Level.changed();
										currentLayer.reflection90.changed();
										currentLayer.roughness.changed();
										currentLayer.anisotropy.changed();
										currentLayer.anisotropyAngle.changed();
										currentLayer.thinFilmInterference.changed();
										currentLayer.thickness.changed();
										currentLayer.minThickness.changed();
										currentLayer.filmIorType.changed();
										currentLayer.filmIorN.changed();
										currentLayer.filmIorK.changed();
										currentLayer.filmIorFilename.changed();
									}
								}
							}

							QuickControls1.TableViewColumn {
								width: treeView.viewport.width
								delegate: Item {
									MouseArea {
										anchors.fill: parent
										acceptedButtons: Qt.RightButton
										onClicked: {
											if (!styleData.selected) {
												treeView.selection.setCurrentIndex(styleData.index, ItemSelectionModel.ClearAndSelect)
											}
											treeViewContextMenu.popup()
										}
									}
									CustomCheckBox {
										visible: model && model.type > 1
										checked: model && model.data.enabled.value
										inverted: styleData.selected
										onToggled: model.data.enabled.value = !model.data.enabled.value
										// anchors.verticalCenter: title.verticalCenter
									}
									Text {
										x: model && model.type > 1 ? 20 : 0
										width: parent.width - x - 4
										text: model ? model.data.name.value : ""
										height: parent.height
										verticalAlignment: Text.AlignVCenter
										elide: Text.ElideRight
										font: Theme.rowFont
										color: styleData.selected ? "#fff" : "#000"
									}
								}
							} // TableViewColumn

							QuickControls1.Menu {
								id: treeViewContextMenu
								QuickControls1.MenuItem {
									text: currentMaterial !== 0 ? "Add Group" : "Add Layer"
									visible: currentMaterial !== 0 || currentGroup !== 0
									onTriggered: materialPresenter.editAdd(treeView.selection.currentIndex)
								}
								QuickControls1.MenuItem {
									text: currentLayer !== 0 ? "Delete Layer" : "Delete Group"
									visible: currentLayer !== 0 || currentGroup !== 0
									onTriggered: materialPresenter.editDelete(treeView.selection.currentIndex)
								}
								QuickControls1.MenuItem {
									text: "Move Up"
									visible: currentLayer !== 0 || currentGroup !== 0
									enabled: (currentLayer !== 0 || currentGroup !== 0) && (treeView.selection.currentIndex.row > 0)
									onTriggered: materialPresenter.editMoveUp(treeView.selection.currentIndex)
								}
								QuickControls1.MenuItem {
									text: "Move Down"
									visible: currentLayer !== 0 || currentGroup !== 0
									enabled: (currentLayer !== 0 || currentGroup !== 0) && (treeView.selection.currentIndex.row + 1 < (material ? material.rowCount(treeView.selection.currentIndex.parent) : 0))
									onTriggered: materialPresenter.editMoveDown(treeView.selection.currentIndex)
								}
							}

							MouseArea {
								anchors.fill: parent
								acceptedButtons: Qt.RightButton
								z: -1
								onClicked: {
									treeViewContextMenu.popup()
								}
							}
						} // TreeView
					}
				}

				Column {
					spacing: Theme.paramSpacing
					Layout.fillWidth: true
					Layout.margins: 10
					Layout.topMargin: 14

					// --- MATERIAL PARAMETERS ---
					StringParam {
						param: currentMaterial ? currentMaterial.name : null
					}

					ScalarParam {
						param: currentMaterial ? currentMaterial.bump : null
					}

					BoolParam {
						param: currentMaterial ? currentMaterial.doubleSided : null
					}

					EnumParam {
						param: currentMaterial ? currentMaterial.medium : null
					}

					ScalarParam {
						param: currentMaterial ? currentMaterial.iorN : null
					}

					ColorParam {
						param: currentMaterial ? currentMaterial.absorptionColor : null
					}

					ScalarParam {
						param: currentMaterial ? currentMaterial.absorptionAttenuation : null
					}

					ColorParam {
						param: currentMaterial ? currentMaterial.emissionColor : null
					}

					ScalarParam {
						param: currentMaterial ? currentMaterial.emissionScale : null
					}

					IntParam {
						param: currentMaterial ? currentMaterial.priority : null
					}

					BoolParam {
						param: currentMaterial ? currentMaterial.subsurfaceScattering : null
						group: true
					}

					ColorParam {
						param: currentMaterial ? currentMaterial.scatteringColor : null
					}

					ScalarParam {
						param: currentMaterial ? currentMaterial.scatteringScale : null
					}

					ScalarParam {
						param: currentMaterial ? currentMaterial.scatteringAsymmetry : null
					}

					// --- GROUP PARAMETERS ---
					StringParam {
						param: currentGroup ? currentGroup.name : null
					}

					ScalarParam {
						param: currentGroup ? currentGroup.mask : null
					}

					// --- LAYER PARAMETERS ---
					StringParam {
						param: currentLayer ? currentLayer.name : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.mask : null
						units: "%"
					}

					ScalarParam {
						param: currentLayer ? currentLayer.bump : null
					}

					BoolParam {
						param: currentLayer ? currentLayer.diffuseLayer : null
						group: true
					}

					ColorParam {
						param: currentLayer ? currentLayer.diffuseColor : null
					}

					BoolParam {
						param: currentLayer ? currentLayer.diffuseTextureLayerMask : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.diffuseOpacity : null
						units: "%"
					}

					ColorParam {
						param: currentLayer ? currentLayer.diffuseTransmission : null
					}

					BoolParam {
						param: currentLayer ? currentLayer.emissiveLayer : null
						group: true
					}

					ColorParam {
						param: currentLayer ? currentLayer.emissiveColor : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.emissiveIntensity : null
					}

					BoolParam {
						param: currentLayer ? currentLayer.specularLayer : null
						group: true
					}

					EnumParam {
						param: currentLayer ? currentLayer.iorType : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.iorN : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.iorK : null
					}

					FileNameParam {
						param: currentLayer ? currentLayer.iorFilename : null
					}

					ColorParam {
						param: currentLayer ? currentLayer.reflection : null
					}

					ColorParam {
						param: currentLayer ? currentLayer.transmission : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.reflection90Level : null
						units: "%"
					}

					ColorParam {
						param: currentLayer ? currentLayer.reflection90 : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.roughness : null
						units: "%"
					}

					ScalarParam {
						param: currentLayer ? currentLayer.anisotropy : null
						units: "%"
					}

					ScalarParam {
						param: currentLayer ? currentLayer.anisotropyAngle : null
						units: "°"
					}

					BoolParam {
						param: currentLayer ? currentLayer.thinFilmInterference : null
						group: true
					}

					ScalarParam {
						param: currentLayer ? currentLayer.thickness : null
						units: "nm"
					}

					ScalarParam {
						param: currentLayer ? currentLayer.minThickness : null
						units: "nm"
					}

					EnumParam {
						param: currentLayer ? currentLayer.filmIorType : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.filmIorN : null
					}

					ScalarParam {
						param: currentLayer ? currentLayer.filmIorK : null
					}

					FileNameParam {
						param: currentLayer ? currentLayer.filmIorFilename : null
					}
				}
			} // ColumnLayout
		}
	}

	function expandTree(index) {
		if (material && material.hasChildren()) {
			var materialIndex = material.index(0, 0);
			treeView.expand(materialIndex);
			var groupCount = material.rowCount(materialIndex);
			for (var gi = 0; gi < groupCount; gi++) {
				var groupIndex = material.index(gi, 0, materialIndex);
				treeView.expand(groupIndex);
			}
			treeView.selection.setCurrentIndex(index, ItemSelectionModel.Select);
		} else {
			currentMaterial = 0;
			currentGroup = 0;
			currentLayer = 0;
		}
	}

	function setMaterial(index) {
		mList.currentIndex = index
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
