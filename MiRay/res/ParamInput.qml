import QtQuick 2.15
import QtQuick.Controls 2.15

TextField {
	id: textField
	focus: true
	clip: true

	font: Theme.paramFont
	horizontalAlignment: Text.AlignRight
	anchors.verticalCenter: parent.verticalCenter
	padding: 4
	color: enabled ? (hovered && !textField.activeFocus ? Theme.mainColor : Theme.paramTextColorEnabled) : Theme.paramTextColorDisabled

	selectByMouse: true
	selectionColor: Theme.mainColor
	selectedTextColor: "#fff"
	persistentSelection: true

	hoverEnabled: true
	background: Item {}

	onAccepted: setValue(text)

	onEditingFinished: if (!TextEditMenu.visible) { setValue(text) }

	onActiveFocusChanged: {
		if (!TextEditMenu.visible) {
			if (!activeFocus) {
				deselect()
			} else if (focusReason === Qt.TabFocus || focusReason === Qt.BacktabFocusReason) {
				selectAll()
			}
		}
	}

	function addStep(increase) {
		if (param) {
			var value, step;
			if (param.precision !== undefined) {
				value = parseFloat(text)
				step = Math.pow(0.1, param.precision - 1)
			} else {
				value = parseInt(text)
				step = 1
			}
			setValue(increase ? (value + step) : (value - step))
		}
	}

	Keys.onUpPressed: addStep(true)
	Keys.onDownPressed: addStep(false)

	Keys.onPressed: {
		if (event.matches(StandardKey.Undo)) {
			if (!canUndo) {
				event.accepted = true
				presenter.undo()
			}
		} else if (event.matches(StandardKey.Redo)) {
			if (!canRedo) {
				event.accepted = true
				presenter.redo()
			}
		}
	}

	TextContextMenu {
		target: textField
	}
}
