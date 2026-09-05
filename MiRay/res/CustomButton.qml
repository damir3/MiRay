import QtQuick 2.2
import QtQuick.Controls 1.4

Item {
	id: button
	property string text: "Button"
	property int elide: Text.ElideNone
	property bool tooltipEnabled: false
	enabled: visible

	signal clicked()

	width: Math.max(title.contentWidth + 20, 100)
	height: Theme.buttonHeight

	Rectangle {
		id: buttonRect
		color: Theme.buttonColor(enabled, mouseArea.containsPress, mouseArea.containsMouse, button.activeFocus)
		anchors.fill: parent
		radius: 4
	}

	MouseArea {
		id: mouseArea
		anchors.fill: buttonRect
		hoverEnabled: true

		CustomToolTip {
			visible: tooltipEnabled && mouseArea.containsMouse && text
			text: button.text
		}

		onPressed: button.forceActiveFocus()
	}

	Text {
		id: title
		anchors.fill: parent
		anchors.leftMargin: 10
		anchors.rightMargin: 10
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		text: button.text
		font: Theme.buttonFont
		color: "#fff"
		elide: button.elide
	}

	Component.onCompleted: {
		mouseArea.clicked.connect(clicked)
		Keys.onEnterPressed.connect(clicked)
		Keys.onReturnPressed.connect(clicked)
	}
}
