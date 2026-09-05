import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Shapes 1.15

Item {
	property var tabView
	property var index
	property var iconNormal
	property var iconActive
	property var iconHover
	property var iconSelected
	property var tooltipText
	property int markSize : 6

	width: 32
	height: 48

	MouseArea {
		id: mouseArea
		anchors.fill: parent
		onClicked: tabView.currentIndex = index
		hoverEnabled: true
		cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

		CustomToolTip {
			id: id_tooltip
			visible: tooltipText ? mouseArea.containsMouse : false
			text: tooltipText || ""
		}
	}

	Image {
		fillMode: Image.Pad
		anchors.centerIn: parent
		source: mouseArea.containsPress ? iconActive : (tabView.currentIndex === index ? iconSelected : (mouseArea.containsMouse ? iconHover : iconNormal))
	}

	Image {
		visible: tabView.currentIndex === index
		anchors.bottom: parent.bottom
		anchors.horizontalCenter: parent.horizontalCenter
		source: "../tab-triangle.png"
		fillMode: Image.Pad
	}

	// Shape {
	// 	visible: tabView.currentIndex === index
	// 	anchors.fill: parent
	// 	ShapePath {
	// 		property int cx: parent.width / 2
	// 		property int cy: parent.height - 1
	// 		strokeColor: "#888"
	// 		fillColor: "#fff"
	// 		startX: 16 - markSize
	// 		startY: 48
	// 		PathLine { x: 16; y: 48 - markSize }
	// 		PathLine { x: 16 + markSize; y: 48 }
	// 	}
	// }
}
