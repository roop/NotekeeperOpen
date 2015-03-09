import QtQuick 1.1
import com.nokia.meego 1.1
import "constants.js" as UI

QtObject {
    id: root
    signal harmattanAppDeactivated();

    property bool isPopup: false;
    property int standardBgColor: (isPopup ? popupBgColor : Qt.white);
    property int popupBgColor: Qt.black;
    property int menuBgColor: (isDarkColorScheme? Qt.black : Qt.white);
    property bool isDarkColorScheme: ((standardBgColor == Qt.black) || (qmlDataAccess.isDarkColorSchemeEnabled && (loginStatusTracker.isLoggedIn || is_simulator)));
    property color pageBgColor: ((!isDarkColorScheme)? "white" : "black")
    property bool harmattanDarkColorSchemeEnabled: (qmlDataAccess.isDarkColorSchemeEnabled && (loginStatusTracker.isLoggedIn || is_simulator));

    property string fontFamilyRegular: UI.FONT_FAMILY;

    property int fontSizeLarge: UI.FONT_LARGE;
    property int fontSizeMedium: UI.FONT_DEFAULT;
    property int fontSizeSmall: UI.FONT_SMALL;

    property int paddingLarge: UI.PADDING_LARGE;
    property int paddingMedium: UI.PADDING_MEDIUM;
    property int paddingSmall: UI.PADDING_SMALL;

    property int graphicSizeLarge: UI.SIZE_ICON_LARGE;
    property int graphicSizeMedium: UI.SIZE_ICON_DEFAULT * 1.25;
    property int graphicSizeSmall: UI.SIZE_ICON_DEFAULT;

    property int borderSizeMedium: UI.CORNER_MARGINS;

    property color colorSeparatorLine: ((!isDarkColorScheme)? "gray" : "lightgray");
    property color colorMenuText: ((menuBgColor == Qt.white)? "#4d4d4d" :  "lightgray");
    property color colorDialogTitle: ((!isDarkColorScheme)? "gray" : "lightgray");
    property color colorTextLabelText: ((!isDarkColorScheme)? "black" : "white")
    property color colorNoteTimestamp: ((!isDarkColorScheme)? "#6ab12f" : "gray")
    property color colorNoteContentSummary: ((!isDarkColorScheme)? "gray" : "lightgray")
    property string colorLinkString: ((!isDarkColorScheme)? "#0000ff" : "#00b2ff")
    property color colorLink: colorLinkString

    property int statusBarHeightPrivate: 36;
    property int tabBarHeightPortraitPrivate: UI.HEADER_DEFAULT_HEIGHT_PORTRAIT;
    property int tabBarHeightLandscapePrivate: UI.HEADER_DEFAULT_HEIGHT_LANDSCAPE;
    property int toolBarHeightPortraitPrivate: 56;
    property int toolBarHeightLandscapePrivate: 46;
    property int textFieldHeightPrivate: UI.FIELD_DEFAULT_HEIGHT
    property int buttonHeightPrivate: UI.BUTTON_HEIGHT

    property int screenHeight: (isPortrait? screenPortraitHeight : screenPortraitWidth)
    property int screenWidth: (isPortrait? screenPortraitWidth : screenPortraitHeight)
    property int screenPortraitHeight: Math.max(screen.displayWidth, screen.displayHeight)
    property int screenPortraitWidth: Math.min(screen.displayWidth, screen.displayHeight)
    property bool isPortrait: (screen.currentOrientation == Screen.Portrait || screen.currentOrientation == Screen.PortraitInverted)
    property bool isNokiaE6: false

    property int pageOrientation_LockPortrait: PageOrientation.LockPortrait
    property int dialogStatus_Closed: DialogStatus.Closed
    property int dialogStatus_Open: DialogStatus.Open
    property int dialogStatus_Opening: DialogStatus.Opening

    property bool isVirtualKeyboardOpen: inputContext.softwareInputPanelVisible
    property real virtualKeyboardHeight: inputContext.softwareInputPanelRect.height

    property int listItemImplicitHeight: UI.LIST_ITEM_HEIGHT
    property string listItemHightlightImage: "image://theme/meegotouch-panel" + (isDarkColorScheme? "-inverted" : "") + "-background-pressed"
    property string listSubItemIndicatorImage: "image://theme/icon-m-common-drilldown-arrow" + (isDarkColorScheme? "-inverse" : "")
    property string listHeadingImage: "../native/images/qt_components_graphics/qtg_fr_list_heading_normal" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg"
    property color listHeaderTextColor: (isDarkColorScheme? "#999" : "#3C3C3C")
    property color listTitleTextColor: (isDarkColorScheme? UI.LIST_TITLE_COLOR_INVERTED : UI.LIST_TITLE_COLOR)
    property color listSubtitleTextColor: (isDarkColorScheme? UI.LIST_SUBTITLE_COLOR_INVERTED : UI.LIST_SUBTITLE_COLOR)

    property string tabBarBgImage: "../native/images/qt_components_graphics/qtg_fr_tab_bar" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string textFieldBgEditable: "image://theme/meegotouch-textedit-background"
    property string textFieldBgHighlighted: "image://theme/meegotouch-textedit-background-selected"
    property string searchIndicatorIcon: "../native/images/qt_components_graphics/qtg_graf_search_indicator" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string clearIconNormal: "../native/images/qt_components_graphics/qtg_graf_textfield_clear_normal" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string clearIconPressed: "../native/images/qt_components_graphics/qtg_graf_textfield_clear_pressed" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property color placeholderTextColor: (isDarkColorScheme? UI.COLOR_INVERTED_SECONDARY_FOREGROUND : UI.COLOR_SECONDARY_FOREGROUND)

    function playThemeEffect(type) {
        return;
    }

    function emitDeactivated() {
        if (platformWindow) {
            if (!platformWindow.active) {
                root.harmattanAppDeactivated(); // emit signal
            }
        }
    }

    Component.onCompleted: {
        if (platformWindow) {
            platformWindow.activeChanged.connect(root.emitDeactivated);
        }
    }
}
