import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3

PopupDialog {
	id: control
	dimBackground: true

	property string messageText: ""
	property var onAcceptedCallback: null

	width: 400
	height: 140 + Theme.titleBarHeight

	Item {
		anchors.fill: parent

		ColumnLayout {
			anchors.fill: parent
			anchors.margins: 10
			spacing: 20

			Label {
				text: control.messageText
				wrapMode: Label.WordWrap
				color: "#000"
				Layout.fillWidth: true
				horizontalAlignment: Text.AlignHCenter
			}

			RowLayout {
				Layout.fillWidth: true
				Item { Layout.fillWidth: true }
				CustomButton {
					text: "OK"
					onClicked: {
						control.close()
						if (control.onAcceptedCallback) {
							control.onAcceptedCallback()
						}
					}
				}
				Item { Layout.fillWidth: true }
			}
		}

		Keys.onEscapePressed: {
			control.close()
			if (control.onAcceptedCallback) {
				control.onAcceptedCallback()
			}
		}
	}
}
