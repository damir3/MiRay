pragma Singleton

import QtQuick 2.15
import QtQuick.Controls 2.15

Menu {
	property var target: null

	topPadding: Theme.menuMargins
	bottomPadding: Theme.menuMargins

	Action {
		text: "Cut"
		shortcut: StandardKey.Cut
		enabled: target && target.selectedText
		onTriggered: target.cut()
	}
	Action {
		text: "Copy"
		shortcut: StandardKey.Copy
		enabled: target && target.selectedText
		onTriggered: target.copy()
	}
	Action {
		text: "Paste"
		enabled: target && target.canPaste
		shortcut: StandardKey.Paste
		onTriggered: target.paste()
	}
	MenuSeparator {}
	Action {
		text: "Select All"
		shortcut: StandardKey.SelectAll
		enabled: target && target.text
		onTriggered: target.selectAll()
	}

	delegate: MenuItem {
		id: menuItem
		implicitHeight: Theme.menuItemHeight

		contentItem: Text {
			leftPadding: Theme.menuMargins
			rightPadding: Theme.menuMargins
			text: menuItem.text
			font: Theme.paramFont
			color: enabled ? (menuItem.highlighted ? Theme.selectionTextColor : Theme.paramTextColorEnabled) : Theme.paramTextColorDisabled
			verticalAlignment: Text.AlignVCenter
			elide: Text.ElideRight
		}

		background: Rectangle {
			anchors.fill: parent
			color: menuItem.highlighted ? Theme.mainColor : "transparent"
			radius: 4
			anchors.leftMargin: Theme.menuMargins
			anchors.rightMargin: Theme.menuMargins
		}
	}

	background: Rectangle {
		implicitWidth: 80
		color: "#fff"
		border.color: "#ccc"
		radius: 4
	}

	onClosed: target.forceActiveFocus()

	function showMenuX(x, y, t) {
		parent = t
		target = t
		popup(x, y, t)
	}
}
