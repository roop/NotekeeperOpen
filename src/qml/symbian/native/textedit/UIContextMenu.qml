import QtQuick 1.0

// UIContextMenu

Item {
    id: uiContextMenu
    width: contextMenuItemsRow.width
    height: 50
    x: 10
    y: 10
    property variant menuItemTexts: [ "Cut", "Trim", "Shave" ]
    property variant flickable;

    signal clicked(string itemText)
    signal pressed()
    signal released()

    Row {
        id: contextMenuItemsRow
        anchors { left: parent.left; top: parent.top; }
        height: uiContextMenu.height
        spacing: 0
        Repeater {
            model: uiContextMenu.menuItemTexts.length
            Item {
                id: contextMenuItem
                height: uiContextMenu.height
                width: contextMenuItemText.width + 40
                property bool isLeftmostItem: (index == 0)
                property bool isRightmostItem: (index == menuItemTexts.length - 1)
                BorderImage {
                    id: contextMenuItemBgImage
                    source: (contextMenuItem.isLeftmostItem ? "images/menu_left_lightblue.png" :
                                (contextMenuItem.isRightmostItem? "images/menu_right_lightblue.png" : "images/menu_middle_lightblue.png"))
                    border { left: 25; top: 0; right: 25; bottom: 15; }
                    anchors { fill: parent; bottomMargin: -5;
                              leftMargin: (contextMenuItem.isLeftmostItem ? -5: 0);
                              rightMargin: (contextMenuItem.isRightmostItem ? -5: 0); }
                    smooth: true
                }
                Text {
                    id: contextMenuItemText
                    anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter; }
                    text: uiContextMenu.menuItemTexts[index]
                    color: "#ededed"
                    font.pixelSize: parent.height - 30
                    font.weight: Font.DemiBold
                }
                MouseArea {
                    id: contextMenuItemMouseArea
                    anchors.fill: parent;
                    onClicked: {
                        uiContextMenu.clicked(contextMenuItemText.text);
                    }
                    onPressed: uiContextMenu.pressed();
                    onReleased: uiContextMenu.released();
                }
                states: [
                    State {
                        name: "pressed"; when: contextMenuItemMouseArea.pressed
                        PropertyChanges {
                            target: contextMenuItemBgImage
                            source: (contextMenuItem.isLeftmostItem ? "images/menu_left_darkblue.png" :
                                        (contextMenuItem.isRightmostItem? "images/menu_right_darkblue.png" : "images/menu_middle_darkblue.png"))
                        }
                    }
                ]
            }
        }
    }
}
