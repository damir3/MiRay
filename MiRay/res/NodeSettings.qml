import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flickable {
	anchors.fill: parent
	clip: true

	enabled: !!nodeModel

	contentHeight: paramsLayout.height + 70

	ScrollBar.vertical: ScrollBar {
		policy: ScrollBar.AsNeeded
	}

	Text {
		x: Theme.margins
		y: Theme.margins
		text: "Node"
		color: enabled ? "#000" : "#ccc"
		font: Theme.groupTitleFont
	}

	ColumnLayout {
		visible: !!nodeModel
		id: paramsLayout
		x: Theme.margins
		y: 45
		width: parent.width - Theme.margins * 2
		spacing: Theme.paramSpacing

		StringParam {
			param: nodeModel ? nodeModel.name : null
		}

		GroupTitle {
			visible: !!nodeModel && !!nodeModel.lightRadius && nodeModel.lightRadius.visible
			text: "Light"
		}
		GroupTitle {
			visible: !!nodeModel && !!nodeModel.lightAngularSize && nodeModel.lightAngularSize.visible
			text: "Directional Light"
		}
		ColorParam {
			param: nodeModel ? nodeModel.lightColor : null
		}
		ScalarParam {
			param: nodeModel ? nodeModel.lightIntensity : null
		}
		ScalarParam {
			param: nodeModel ? nodeModel.lightRadius : null
			units: "cm"
		}
		ScalarParam {
			param: nodeModel ? nodeModel.lightAngularSize : null
			units: "°"
		}
		ScalarParam {
			param: nodeModel ? nodeModel.lightYaw : null
			units: "°"
		}
		ScalarParam {
			param: nodeModel ? nodeModel.lightPitch : null
			units: "°"
		}

		GroupTitle {
			visible: !!nodeModel && !!nodeModel.meshRenderingLayer && nodeModel.meshRenderingLayer.visible
			text: "Rendering Properties"
		}
		EnumParam {
			param: nodeModel ? nodeModel.meshRenderingLayer : null
		}

		GroupTitle {
			visible: !!nodeModel && !!nodeModel.position && nodeModel.position.visible
			text: "Transformation"
		}
		Vec3Param {
			param: nodeModel ? nodeModel.position : null
			units: "cm"
		}
		Vec3Param {
			param: nodeModel ? nodeModel.rotation : null
			units: "°"
		}
		Vec3Param {
			param: nodeModel ? nodeModel.scale : null
		}
	}
}
