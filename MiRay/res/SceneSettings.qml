import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flickable {
	anchors.fill: parent
	clip: true

	contentHeight: paramsLayout.height + 25

	ScrollBar.vertical: ScrollBar {
		policy: ScrollBar.AsNeeded
	}

	Text {
		visible: !sceneModel
		x: Theme.margins
		y: Theme.margins
		text: "Scene Settings"
		color: enabled ? "#000" : "#ccc"
		font: Theme.groupTitleFont
	}

	ColumnLayout {
		visible: !!sceneModel
		id: paramsLayout
		x: Theme.margins
		y: 1
		width: parent.width - Theme.margins * 2
		spacing: Theme.paramSpacing

		GroupTitle {
			text: "Background"
		}
		EnumParam {
			param: sceneModel ? sceneModel.bgMode : null
			comboBoxWidth: 0.6
		}
		ColorParam {
			param: sceneModel ? sceneModel.bgColor : null
		}
		ColorParam {
			param: sceneModel ? sceneModel.bgColor2 : null
		}

		GroupTitle {
			text: "Environment"
		}
		ColorParam {
			param: sceneModel ? sceneModel.envColor : null
		}
		ScalarParam {
			param: sceneModel ? sceneModel.envIntensity : null
			units: "%"
		}
		ScalarParam {
			param: sceneModel ? sceneModel.envSize : null
			units: "cm"
		}
		ScalarParam {
			param: sceneModel ? sceneModel.envVerticalOffset : null
		}
		ScalarParam {
			param: sceneModel ? sceneModel.envHorizontalRotation : null
			units: "°"
		}
		ScalarParam {
			param: sceneModel ? sceneModel.envVerticalRotation : null
			units: "°"
		}
		ScalarParam {
			param: sceneModel ? sceneModel.diffuseIntensity : null
			units: "%"
		}

		BoolParam {
			param: sceneModel ? sceneModel.floorEnabled : null
			group: true
		}
		ScalarParam {
			param: sceneModel ? sceneModel.floorReflectionLevel : null
			units: "%"
		}
		ScalarParam {
			param: sceneModel ? sceneModel.floorRoughness : null
			units: "%"
		}
		ScalarParam {
			param: sceneModel ? sceneModel.floorShadowLevel : null
			units: "%"
		}
	}
}
