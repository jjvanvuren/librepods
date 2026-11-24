pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Shapes
import "NoiseControlIcons.js" as Icons

// Noise control modes as round icon buttons, like the AirPods controls on
// Apple devices, with the mode's name as a tooltip. The icons are vector paths
// (NoiseControlIcons.js) filled from the system palette; the selected mode's
// circle is filled with the highlight colour.
//
// model: [{text, icon}], icon being a key of Icons.paths, indexed by
// NoiseControlMode value. Choosing a mode
// emits activated(index) and leaves currentIndex to its binding, so the owner
// decides when the selection changes (e.g. once the AirPods confirm it).
Control {
    id: root

    property var model: []
    property int currentIndex: 0
    signal activated(int index)

    padding: 0
    focusPolicy: Qt.StrongFocus
    activeFocusOnTab: true

    SystemPalette {
        id: systemPalette
    }

    contentItem: Row {
        spacing: 16

        Repeater {
            model: root.model

            delegate: AbstractButton {
                id: modeButton
                required property int index
                required property var modelData
                readonly property bool selected: root.currentIndex === index

                width: 52
                height: 52
                hoverEnabled: true
                focusPolicy: Qt.NoFocus // Let the root control handle focus
                ToolTip.text: modelData.text
                ToolTip.visible: hovered
                ToolTip.delay: 500
                Accessible.name: modelData.text
                Accessible.role: Accessible.RadioButton
                Accessible.checked: selected

                background: Rectangle {
                    radius: width / 2
                    color: modeButton.selected ? systemPalette.highlight
                         : modeButton.down ? Qt.darker(systemPalette.button, 1.2)
                         : modeButton.hovered ? Qt.lighter(systemPalette.button, 1.25)
                         : systemPalette.button
                    border.width: root.activeFocus && modeButton.selected ? 2 : 0
                    border.color: systemPalette.highlightedText
                }

                contentItem: Item {
                    // Drawn in the icons' own frame, scaled down to 36 px
                    Shape {
                        anchors.centerIn: parent
                        width: Icons.size
                        height: Icons.size
                        scale: 36 / Icons.size
                        preferredRendererType: Shape.CurveRenderer // sharp at any scale
                        antialiasing: true

                        ShapePath {
                            strokeWidth: -1
                            fillRule: ShapePath.OddEvenFill
                            fillColor: modeButton.selected ? systemPalette.highlightedText : systemPalette.buttonText

                            PathSvg {
                                path: Icons.paths[modeButton.modelData.icon]
                            }
                        }
                    }
                }

                onClicked: {
                    if (!selected)
                        root.activated(index)
                }
            }
        }
    }

    // Keyboard: arrows, Home/End and 1-9 pick a mode
    Keys.onPressed: event => {
        let target = -1
        if (event.key === Qt.Key_Left)
            target = root.currentIndex - 1
        else if (event.key === Qt.Key_Right)
            target = root.currentIndex + 1
        else if (event.key === Qt.Key_Home)
            target = 0
        else if (event.key === Qt.Key_End)
            target = root.model.length - 1
        else if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9)
            target = event.key - Qt.Key_1

        if (target >= 0 && target < root.model.length) {
            if (target !== root.currentIndex)
                root.activated(target)
            event.accepted = true
        }
    }
}
