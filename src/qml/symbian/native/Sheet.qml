import QtQuick 1.1
import com.nokia.symbian 1.1

Item {
    id: root
    property Item pageStack;
    property string acceptButtonText: "Ok";
    property string rejectButtonText: "Cancel";
    property alias content: contentContainer.children;

    signal accepted();
    signal rejected();

    Page {
        id: page
        anchors.fill: parent

        property string okButtonText: root.acceptButtonText
        property string cancelButtonText: root.rejectButtonText

        UiStyle {
            id: _UI
        }

        Rectangle {
            anchors.fill: parent
            color: "#fff"
        }

        TopButtonBar {
            id: titleBar
            anchors { left: parent.left; right: parent.right; top: parent.top; }
            colorScheme: Qt.white
            Row {
                anchors.centerIn: parent
                spacing: _UI.paddingLarge * 2
                Button {
                    text: page.okButtonText
                    onClicked: {
                        root.accepted(); // emit signal
                        root.pageStack.pop();
                    }
                    width: titleBar.width / 3
                }
                Button {
                    text: page.cancelButtonText
                    onClicked: {
                        root.rejected(); // emit signal
                        root.pageStack.pop();
                    }
                    width: titleBar.width / 3
                }
            }
        }

        Item {
            id: contentContainer
            anchors { fill: parent; topMargin: titleBar.height; }
        }

        tools: ToolBarLayout { }
    }

    function open() {
        root.pageStack.push(page);
    }
}
