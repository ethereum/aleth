import QtQuick 2.0
import QtQuick.Controls 1.1
Item
{
	property alias value: textinput.text
	property alias readOnly: textinput.readOnly
	id: editRoot
	width: 200
	DebuggerPaneStyle {
		id: dbgStyle
	}

	TextField {
		anchors.verticalCenter: parent.verticalCenter
		id: textinput
		selectByMouse: true
		text: value
		implicitWidth: 200
		MouseArea {
			id: mouseArea
			anchors.fill: parent
			hoverEnabled: true
			onClicked: textinput.forceActiveFocus()
		}
	}
}
