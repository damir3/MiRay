import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
	property bool pressed : false
	property var iconPressed
	property var iconNormal
	property var iconHover
	property var iconDisabled
	property string tooltipText

	id: root
	signal clicked()

	width: 32
	height: 48

	MouseArea {
		id: mouseArea
		anchors.fill: parent
		onClicked: root.clicked()
		hoverEnabled: true
		cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

		CustomToolTip {
			id: id_tooltip
			visible: tooltipText ? (mouseArea.containsMouse && text) : false
			text: tooltipText || ""
		}
	}

	Image {
		fillMode: Image.Pad
		anchors.centerIn: parent
		source: enabled ? (pressed || mouseArea.containsPress ? iconPressed : (mouseArea.containsMouse ? iconHover : iconNormal)) : iconDisabled
	}
}
