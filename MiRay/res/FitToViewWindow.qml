import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

PopupDialog {
	id: dialogWindow
	title: "Fit To View"

	width: paramsLayout.width + 2 * Theme.dialogHMargins
	height: 210 + Theme.titleBarHeight

	Item {
		anchors.fill: parent

		ColumnLayout {
			id: paramsLayout
			width: 256
			spacing: Theme.paramSpacing

			ScalarParam {
				param: fitToViewModel ? fitToViewModel.padding : null
				units: "%"
			}

			BoolParam {
				param: fitToViewModel ? fitToViewModel.justSelectedObjects : null
			}

			BoolParam {
				param: fitToViewModel ? fitToViewModel.includingChildrenObjects : null
			}

			BoolParam {
				param: fitToViewModel ? fitToViewModel.keepAspect : null
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
					presenter.acceptFitToView()
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: dialogWindow.close()
	}

	onAboutToShow: presenter.beginFitToView()
	onAboutToHide: presenter.endFitToView()
}
