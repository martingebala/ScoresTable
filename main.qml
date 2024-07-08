// main.qml

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: "Client-Server Scores Board Example"

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: "Upload file:"
            leftPadding: 8
        }

        FileDialog {
            id: fileDialog
            title: "Please choose a file with race data"

            onAccepted: {
                Client.getScores(selectedFile)
            }
            nameFilters: [ "CSV files (*.csv)" ]
        }

        Button {
            text: "Upload racing data"
            onClicked: fileDialog.open()
        }

        Label {
            text: "Simulate:"
            leftPadding: 8
        }

        RowLayout {
            Repeater {
                model: 5
                Column {
                    Layout.preferredWidth: 130
                    Button {
                        text: "Kart " + (index + 1)
                        onClicked: {
                            timer.running = true
                            timer.count = 0
                            timer.lapN++
                            Client.sendMeasurement(index + 1, Date.now())
                        }
                    }
                    Text {
                        id: timerDisplay
                        text: "Click to start lap"
                        Timer {
                            id: timer
                            property int lapN: 0
                            property int count: 0
                            interval: 100
                            running: false
                            repeat: true
                            onTriggered: timerDisplay.text = "Lap " + lapN + ": "+ new Date(100*(count++)).toLocaleTimeString(Qt.locale(), "mm:ss.zzz")
                        }
                    }
                }
            }
        }

        ListView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 16
            model: Client.scores

            header: Item {
                width: ListView.view.width
                height: childrenRect.height
                RowLayout {
                    anchors.horizontalCenter: parent.horizontalCenter

                    Label {
                        Layout.preferredWidth: 64
                        color: "white"
                        text: "Position"
                    }

                    Label {
                        Layout.preferredWidth: 100
                        color: "white"
                        text: "Car"
                        font.bold: true
                    }

                    Text {
                        Layout.preferredWidth: 100
                        color: "white"
                        text: "Avg lap time"
                    }

                    Text {
                        Layout.preferredWidth: 100
                        color: "white"
                        text: "Best lap time"
                        font.bold: true
                    }

                    Text {
                        Layout.preferredWidth: 100
                        color: "white"
                        text: "Gap to leader"
                    }
                }
            }

            delegate: Rectangle {
                width: ListView.view.width
                height: 32
                color: "black"
                RowLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    Rectangle {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        Layout.margins: 6
                        color: "white"
                        radius: 4
                        Text {
                            anchors.centerIn: parent
                            text: index
                        }
                    }

                    Text {
                        Layout.preferredWidth: 100
                        Layout.leftMargin: 32
                        color: "white"
                        text: "Kart " + model.index
                        font.bold: true
                    }
                    Text {
                        Layout.preferredWidth: 100
                        color: "white"
                        text: model.averagelap
                    }
                    Text {
                        Layout.preferredWidth: 100
                        color: "white"
                        text: model.bestlap
                    }
                    Text {
                        Layout.preferredWidth: 100
                        color: "white"
                        text: model.diff
                    }
                }
            }
            displaced: Transition {
                NumberAnimation { properties: "y"; duration: 1000 }
            }
        }
    }
}
