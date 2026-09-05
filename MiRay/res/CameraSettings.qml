import QtQuick 2.0
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flickable {
	anchors.fill: parent
	clip: true

	contentHeight: paramsLayout.height + 70

	ScrollBar.vertical: ScrollBar {
		policy: ScrollBar.AsNeeded
	}

	Text {
		x: Theme.margins
		y: Theme.margins
		text: "Camera"
		color: enabled ? "#000" : "#ccc"
		font: Theme.groupTitleFont
	}

	ColumnLayout {
		visible: !!cameraModel
		id: paramsLayout
		x: Theme.margins
		y: 45
		width: parent.width - Theme.margins * 2
		spacing: Theme.paramSpacing

		EnumParam {
			param: cameraModel ? cameraModel.projection : null
		}
		Vec3Param {
			param: cameraModel ? cameraModel.target : null
			units: "cm"
		}
		Text {
			Layout.fillWidth: true
			text: Qt.platform.os === "osx" ?
				"Use Command + right-click in the preview to change the camera target." :
				"Use Ctrl + right-click in the preview to change the camera target."
			font: Theme.paramFont
			color: "#888"
			wrapMode: Text.WordWrap
		}
		ScalarParam {
			param: cameraModel ? cameraModel.distance : null
			units: "cm"
		}
		ScalarParam {
			param: cameraModel ? cameraModel.yaw : null
			units: "°"
		}
		ScalarParam {
			param: cameraModel ? cameraModel.pitch : null
			units: "°"
		}
		ScalarParam {
			param: cameraModel ? cameraModel.roll : null
			units: "°"
		}
		ScalarParam {
			param: cameraModel ? cameraModel.fov : null
			units: "°"
		}
		ScalarParam {
			param: cameraModel ? cameraModel.aspect : null
		}
		ScalarParam {
			param: cameraModel ? cameraModel.nearZ : null
			units: "cm"
		}
		ScalarParam {
			param: cameraModel ? cameraModel.farZ : null
			units: "cm"
		}
		ScalarParam {
			param: cameraModel ? cameraModel.gamma : null
		}
		BoolParam {
			param: cameraModel ? cameraModel.depthOfField : null
			group: true
		}
		ScalarParam {
			param: cameraModel ? cameraModel.fStop : null
		}
		ScalarParam {
			param: cameraModel ? cameraModel.focusDistance : null
			units: "cm"
		}
		Text {
			Layout.fillWidth: true
			text: Qt.platform.os === "osx" ?
				"Use Option + right-click to set the focus distance." :
				"Use Alt + right-click to set the focus distance."
			font: Theme.paramFont
			color: "#888"
			wrapMode: Text.WordWrap
			visible: cameraModel && cameraModel.depthOfField && cameraModel.depthOfField.value
		}
		IntParam {
			param: cameraModel ? cameraModel.diaphragmBlades : null
		}
		ScalarParam {
			param: cameraModel ? cameraModel.bokehRotation : null
			units: "°"
		}
	}
}
