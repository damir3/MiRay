import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

PopupDialog {
	id: dialogWindow
	title: "UV Mapping"

	width: paramsLayout.width + 2 * Theme.dialogHMargins
	height: 410 + Theme.titleBarHeight

	Item {
		anchors.fill: parent

		ColumnLayout {
			id: paramsLayout
			width: 256
			spacing: Theme.paramSpacing

			EnumParam {
				param: uvMappingModel ? uvMappingModel.mapping : null
				comboBoxWidth: 0.5
			}

			EnumParam {
				param: uvMappingModel ? uvMappingModel.fitTo : null
				comboBoxWidth: 0.5
			}

			EnumParam {
				param: uvMappingModel ? uvMappingModel.uvSet : null
				comboBoxWidth: 0.5
			}

			BoolParam {
				param: uvMappingModel ? uvMappingModel.flipU : null
			}

			BoolParam {
				param: uvMappingModel ? uvMappingModel.flipV : null
			}

			BoolParam {
				param: uvMappingModel ? uvMappingModel.normalize : null
			}

			Vec3Param {
				param: uvMappingModel ? uvMappingModel.scale : null
			}

			Vec2Param {
				param: uvMappingModel ? uvMappingModel.repeat : null
			}
		}

		RowLayout {
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.bottom: parent.bottom
			spacing: Theme.buttonSpacing

			CustomButton {
				text: "Cancel"
				onClicked: dialogWindow.close()
			}

			CustomButton {
				text: "OK"
				onClicked: {
					presenter.acceptUVMapping()
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: dialogWindow.close()
	}

	onAboutToShow: presenter.beginUVMapping()
	onAboutToHide: presenter.endUVMapping()
}
