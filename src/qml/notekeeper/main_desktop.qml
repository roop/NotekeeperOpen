import QtQuick 1.0

Rectangle {
    id: root
    width: 200
    height: 150

    Column {
        anchors.fill: parent
        TextLabel {
            text: (internal.isBusy? "Doing something ..." : "Idle");
        }
        TextLabel {
            id: messageBox
            text: ""
        }
        Rectangle {
            width: parent.width
            height: 50
            color: (mouseArea.pressed? "lightblue" : "blue")
            TextLabel {
                anchors.centerIn: parent
                text: "Sync"
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: root.login();
            }
        }
    }

    QtObject {
        id: internal
        property bool isBusy: false;
    }

    function login() {
        internal.isBusy = true;

        qmlDataAccess.saveEvernoteAuthData("username", "roop");
        qmlDataAccess.saveEvernoteAuthData("password", "knurdin");

        evernoteSync.startSync();
    }

    function evernoteSyncFinished(success, message) {
        if (success) {
            console.log("Sync finished");
        }
        internal.isBusy = false;
    }

    function evernoteLoginFinished(loggedIn, message) {
        if (!loggedIn) {
            messageBox.message = message;
        }
    }

    Component.onCompleted: {
        evernoteSync.loginFinished.connect(evernoteLoginFinished);
        evernoteSync.syncFinished.connect(evernoteSyncFinished);
    }
    Component.onDestruction: {
        evernoteSync.loginFinished.disconnect(evernoteLoginFinished);
    }
}

