import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

PopupDialog {
	id: dialogWindow
	title: "MiRay Settings"

	width: paramsLayout.width + 2 * Theme.dialogHMargins
	height: (appSettingsModel && appSettingsModel.opengl ? 420 : 330) + Theme.titleBarHeight

	Item {
		anchors.fill: parent

		ColumnLayout {
			id: paramsLayout
			width: 360
			spacing: Theme.paramSpacing

			GroupTitle {
				text: "Import Settings"
			}

			BoolParam {
				param: appSettingsModel ? appSettingsModel.putLoadedOnTheFloor : null
			}

			GroupTitle {
				text: "Preview"
			}

			IntParam {
				param: appSettingsModel ? appSettingsModel.previewFrames : null
			}

			BoolParam {
				param: appSettingsModel ? appSettingsModel.previewDenoise : null
			}

			GroupTitle {
				text: "Graphics Hardware"
				visible: appSettingsModel && appSettingsModel.opengl ? appSettingsModel.opengl.visible : false
			}

			EnumParam {
				param: appSettingsModel && appSettingsModel.opengl ? appSettingsModel.opengl : null
				visible: appSettingsModel && appSettingsModel.opengl ? appSettingsModel.opengl.visible : false
				comboBoxWidth: 0.5
			}

			// GroupTitle {
			// 	text: "Job Manager"
			// }

			// StringParam {
			// 	param: appSettingsModel ? appSettingsModel.queuedJobsPath : null
			// }

			// Text {
			// 	Layout.fillWidth: true
			// 	text: "Use absolute path or keep empty to use the default folder. Requires restart."
			// 	font: Theme.paramFont
			// 	color: "#888"
			// 	wrapMode: Text.WordWrap
			// 	visible: appSettingsModel && appSettingsModel.queuedJobsPath ? appSettingsModel.queuedJobsPath.visible : false
			// }

			// GroupTitle {
			// 	text: "Updates"
			// }

			// BoolParam {
			// 	param: appSettingsModel ? appSettingsModel.checkUpdates : null
			// }

			// BoolParam {
			// 	param: appSettingsModel ? appSettingsModel.includingBetas : null
			// }
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
					presenter.acceptAppSettings()
					dialogWindow.close()
				}
			}
		}

		Keys.onEscapePressed: dialogWindow.close()
	}

	onAboutToShow: presenter.beginAppSettings()
	onAboutToHide: presenter.endAppSettings()
}
