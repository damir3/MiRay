import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

PopupDialog {
	id: dialogWindow
	title: "Rendering Parameters"

	width: paramsLayout.width + 2 * Theme.dialogHMargins
	height: 480 + Theme.titleBarHeight

	Item {
		anchors.fill: parent

		ColumnLayout {
			id: paramsLayout
			width: 256
			spacing: Theme.paramSpacing

			GroupTitle {
				text: "Resolution"
			}

			IntParam {
				param: renderingParametersModel ? renderingParametersModel.width : null
			}

			IntParam {
				param: renderingParametersModel ? renderingParametersModel.height : null
			}

			BoolParam {
				param: renderingParametersModel ? renderingParametersModel.keepAspectRatio : null
			}

			GroupTitle {
				text: "Quality"
			}

			EnumParam {
				param: renderingParametersModel ? renderingParametersModel.renderingPreset : null
				comboBoxWidth: 0.5
			}

			IntParam {
				param: renderingParametersModel ? renderingParametersModel.numIterations : null
			}

			ScalarParam {
				param: renderingParametersModel ? renderingParametersModel.maxIntensity : null
			}

			BoolParam {
				param: renderingParametersModel ? renderingParametersModel.denoise : null
			}

			GroupTitle {
				text: "Other"
			}

			BoolParam {
				param: renderingParametersModel ? renderingParametersModel.extraChannels : null
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
				text: "Start"
				onClicked: {
					presenter.acceptRenderingParameters()
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: dialogWindow.close()
	}

	onAboutToShow: presenter.beginRenderingParameters()

	onAboutToHide: presenter.endRenderingParameters()
}
