import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import myqml 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    TrackModel {
        id: trackModel
    }
    TrackLoader {
        id: trackLoader
        onLoaded: {
            trackModel.setList(list)
        }
    }

    ColumnLayout {
        anchors.fill: parent

        Button {
            id: loadButton
            text: "Load tracks"
            enabled: !trackLoader.loading
            onClicked: {
                trackLoader.load("tracks.txt")
            }

            Layout.fillWidth: true
        }

        ListView {
            model: trackModel
            Layout.fillWidth: true
            Layout.fillHeight: true
            delegate: Text {
                text: name
            }
        }
    }
}
