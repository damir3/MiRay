import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

PopupDialog {
	id: dialogWindow
	title: "Edit Normals"

	width: paramsLayout.width + 2 * Theme.dialogHMargins
	height: 240 + Theme.titleBarHeight

	Item {
		anchors.fill: parent

		ColumnLayout {
			id: paramsLayout
			width: 256
			spacing: Theme.paramSpacing

			BoolParam {
				param: editNormalsModel ? editNormalsModel.calculateNormals : null
			}

			EnumParam {
				param: editNormalsModel ? editNormalsModel.makeEdges : null
				comboBoxWidth: 0.3
			}

			ScalarParam {
				param: editNormalsModel ? editNormalsModel.maxSoftAngle : null
				units: "°"
			}

			BoolParam {
				param: editNormalsModel ? editNormalsModel.flipNormals : null
			}

			BoolParam {
				param: editNormalsModel ? editNormalsModel.flipFacing : null
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
					presenter.acceptEditNormals()
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: dialogWindow.close()
	}

	onAboutToShow: presenter.beginEditNormals()
	onAboutToHide: presenter.endEditNormals()
}
