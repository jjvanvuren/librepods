pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Effects

// Noise control modes as round icon buttons with the mode's name underneath,
// like the AirPods controls on Apple devices. The icons are white masks
// (assets/noise-*.png) tinted with the system palette; the selected mode's
// circle is filled with the highlight colour.
//
// model: [{text, icon}], indexed by NoiseControlMode value. Choosing a mode
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
        spacing: 8

        Repeater {
            model: root.model

            delegate: Column {
                id: mode
                required property int index
                required property var modelData
                readonly property bool selected: root.currentIndex === index

                width: 84
                spacing: 6

                AbstractButton {
                    id: modeButton
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 52
                    height: 52
                    hoverEnabled: true
                    focusPolicy: Qt.NoFocus // Let the root control handle focus
                    Accessible.name: mode.modelData.text
                    Accessible.role: Accessible.RadioButton
                    Accessible.checked: mode.selected

                    background: Rectangle {
                        radius: width / 2
                        color: mode.selected ? systemPalette.highlight
                             : modeButton.down ? Qt.darker(systemPalette.button, 1.2)
                             : modeButton.hovered ? Qt.lighter(systemPalette.button, 1.25)
                             : systemPalette.button
                        border.width: root.activeFocus && mode.selected ? 2 : 0
                        border.color: systemPalette.highlightedText
                    }

                    contentItem: Item {
                        Image {
                            anchors.centerIn: parent
                            width: 36
                            height: 36
                            source: mode.modelData.icon
                            sourceSize: Qt.size(108, 108)
                            fillMode: Image.PreserveAspectFit
                            mipmap: true
                            // The icon is white: colourise it to the text colour
                            layer.enabled: true
                            layer.effect: MultiEffect {
                                colorization: 1.0
                                colorizationColor: mode.selected ? systemPalette.highlightedText : systemPalette.buttonText
                            }
                        }
                    }

                    onClicked: {
                        if (!mode.selected)
                            root.activated(mode.index)
                    }
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: mode.modelData.text
                    font.pointSize: Math.max(7, root.font.pointSize - 1)
                    color: mode.selected ? systemPalette.highlight : systemPalette.windowText
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
