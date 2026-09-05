pragma Singleton
import QtQuick 2.0

Item {
	property color mainColor: "#30B000"
	property color mainColorInactive: "#BABCC2"
	property color mainColorHovered: "#36C000"
	property color mainColorPressed: "#40E000"
	property color toolBarColor: "#f2f3f6"
	property color edgeColor: "#c8cace"
	property color emptyPreviewColor: "#f2f3f6"
	property color dimColor: "#88000000"

	property int margins: 10
	property int dialogPaddings: 20
	property int dialogHMargins: 20
	property int dialogVMargins: 20
	property int dialogVSpacing: 40
	property int dialogCornerRadius: 14
	property int paramHeight: 22
	property int boolParamHeight: 12
	property int paramSpacing: 12
	property color paramTextColorEnabled: "#000"
	property color paramTextColorDisabled: "#BABCC2"
	property color paramTitleEnabled: "#666A75"
	property color paramTitleDisabled: "#BABCC2"
	property color unitsColorEnabled: "#9198A7"
	property color unitsColorDisabled: "#BABCC2"
	property font paramFont: Qt.font({ family: availableFontName([ "Segoe UI", "Arial", "Helvetica", "Verdana" ]), pixelSize: 13 })
	property font paramFontBold: Qt.font({ family: availableFontName([ "Segoe UI", "Arial", "Helvetica", "Verdana" ]), pixelSize: 13, bold: true })
	property font labelFont: Qt.font({ family: availableFontName([ "Segoe UI", "Verdana", "Arial" ]), pixelSize: 15 })
	property font labelFontBold: Qt.font({ family: availableFontName([ "Segoe UI", "Verdana", "Arial" ]), pixelSize: 15, bold: true })

	property font groupTitleFont: Qt.font({ family: availableFontName([ "Segoe UI", "Gill Sans", "Arial", "Helvetica", "Verdana" ]), pixelSize: 20, weight: Font.DemiBold })
	property int groupTitleHeight: 32

	property font popupTitleFont: Qt.font({ family: availableFontName([ "Segoe UI", "Arial", "Helvetica", "Verdana" ]), pixelSize: 16, bold: true })
	property font aboutTitleFont: Qt.font({ family: availableFontName([ "Segoe UI Light", "Gill Sans", "Arial", "Helvetica", "Verdana" ]), pixelSize: 26, weight: Font.Light })
	property int titleBarHeight: 28

	property int menuItemHeight: 25
	property int menuMargins: 6

	//property int buttonHeight: 28
	property int buttonHeight: 32
	property int buttonSpacing: 10
	property font buttonFont: Qt.font({ family: availableFontName([ "Segoe UI", "Arial", "Helvetica", "Verdana" ]), pixelSize: 14, bold: true })

	property int textSize: 12
	property int tooltipTextSize: 11

	property int tabHMargins: 8

	property int listHMargins: 6
	property int listVMargins: 6
	property int listPaddings: 10
	property font rowFont: Qt.font({ family: availableFontName([ "Segoe UI", "Arial", "Helvetica", "Verdana" ]), pixelSize: 13 })
	property font rowFontBold: Qt.font({ family: availableFontName([ "Segoe UI", "Arial", "Helvetica", "Verdana" ]), pixelSize: 13, bold: true })
	property int rowHeight: 24
	function rowColor(selected, activeFocus, index) {
		//return selected ? (activeFocus ? mainColor : mainColorInactive) : (index & 1 ? "#f5f5f5" : "transparent")
		return selected ? (activeFocus ? mainColor : mainColorInactive) : "transparent"
	}

	function buttonColor(enabled, pressed, hovered, hasActiveFocus) {
		if (!enabled) {
			return "#E1E2E6"
		} else if (pressed) {
			return mainColorPressed
		} else if (hovered) {
			return hasActiveFocus ? mainColorHovered : "#9CA2B0"
		} else {
			return hasActiveFocus ? mainColor : "#9198A7"
		}
	}

	function comboBoxColor(enabled, pressed, hovered) {
		return enabled ? (pressed ? "#E1E2E6" : (hovered ? "#F2F3F6" : "#ECEEF2")) : "#f0f0f0"
	}

	//property color selectionBackgroundColor: "#0c0"
	property color selectionTextColor: "#fff"

	property color dropTargetColor: "#888"
	property color dropTargetColor2: "#ccc"

	function availableFontName(names) {
		var fontFamilies = Qt.fontFamilies();
		for (var i = 0; i < names.length; ++i) {
			if (fontFamilies.indexOf(names[i]) !== -1) {
				return names[i]
			}
		}
		return ""
	}
}
