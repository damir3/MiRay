import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

PopupDialog {
	id: dialogWindow
	title: "Pivot Parameters"

	width: paramsLayout.width + 2 * Theme.dialogHMargins
	height: 250 + Theme.titleBarHeight

	Item {
		anchors.fill: parent

		ColumnLayout {
			id: paramsLayout
			width: 320
			spacing: Theme.paramSpacing

			EnumParam {
				param: pivotParamsModel ? pivotParamsModel.calculatePivot : null
				comboBoxWidth: 0.62
			}

			BoolParam {
				param: pivotParamsModel ? pivotParamsModel.includingChildrenNodes : null
			}

			EnumParam {
				param: pivotParamsModel ? pivotParamsModel.pivotX : null
				comboBoxWidth: 0.3
			}

			EnumParam {
				param: pivotParamsModel ? pivotParamsModel.pivotY : null
				comboBoxWidth: 0.3
			}

			EnumParam {
				param: pivotParamsModel ? pivotParamsModel.pivotZ : null
				comboBoxWidth: 0.3
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
					presenter.acceptPivotParameters()
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: dialogWindow.close()
	}

	onAboutToShow: presenter.beginPivotParameters()
	onAboutToHide: presenter.endPivotParameters()
}
