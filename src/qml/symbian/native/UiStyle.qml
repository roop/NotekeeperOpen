import QtQuick 1.1
import com.nokia.symbian 1.1

QtObject {
    signal harmattanAppDeactivated(); // never emitted in Symbian

    property bool isPopup: false;
    property int standardBgColor: (isPopup ? popupBgColor : Qt.white);
    property int popupBgColor: Qt.black;
    property int menuBgColor: Qt.black;
    property bool isDarkColorScheme: ((standardBgColor == Qt.black) || (qmlDataAccess.isDarkColorSchemeEnabled && (loginStatusTracker.isLoggedIn || is_simulator)));
    property color pageBgColor: ((!isDarkColorScheme)? "white" : "black")
    property bool harmattanDarkColorSchemeEnabled: false;

    property string fontFamilyRegular: platformStyle.fontFamilyRegular;

    property int fontSizeLarge: platformStyle.fontSizeLarge;
    property int fontSizeMedium: platformStyle.fontSizeMedium;
    property int fontSizeSmall: platformStyle.fontSizeSmall;

    property int paddingLarge: platformStyle.paddingLarge;
    property int paddingMedium: platformStyle.paddingMedium;
    property int paddingSmall: platformStyle.paddingSmall;

    property int graphicSizeLarge: platformStyle.graphicSizeLarge;
    property int graphicSizeMedium: platformStyle.graphicSizeMedium;
    property int graphicSizeSmall: platformStyle.graphicSizeSmall;

    property int borderSizeMedium: platformStyle.borderSizeMedium;

    property color colorSeparatorLine: ((!isDarkColorScheme)? platformStyle.colorDisabledLightInverted : platformStyle.colorDisabledLight);
    property color colorMenuText: ((menuBgColor == Qt.white)? platformStyle.colorNormalLightInverted : platformStyle.colorNormalLight);
    property color colorDialogTitle: ((popupBgColor == Qt.white)? platformStyle.colorNormalLinkInverted : platformStyle.colorNormalLink);
    property color colorTextLabelText: ((!isDarkColorScheme)? "black" : "white")
    property color colorNoteTimestamp: ((!isDarkColorScheme)? "#6ab12f" : "gray")
    property color colorNoteContentSummary: ((!isDarkColorScheme)? "gray" : "lightgray")
    property string colorLinkString: ((!isDarkColorScheme)? "#0000ff" : "#00b2ff")
    property color colorLink: colorLinkString

    property int statusBarHeightPrivate: ((privateStyle && privateStyle.statusBarHeight)? privateStyle.statusBarHeight : 26);
    property int tabBarHeightPortraitPrivate: ((privateStyle && privateStyle.tabBarHeightPortrait)? privateStyle.tabBarHeightPortrait : 56);
    property int tabBarHeightLandscapePrivate: ((privateStyle && privateStyle.tabBarHeightLandscape)? privateStyle.tabBarHeightLandscape : 42);
    property int toolBarHeightPortraitPrivate: ((privateStyle && privateStyle.toolBarHeightPortrait)? privateStyle.toolBarHeightPortrait : 56);
    property int toolBarHeightLandscapePrivate: ((privateStyle && privateStyle.toolBarHeightLandscape)? privateStyle.toolBarHeightLandscape : 46);
    property int textFieldHeightPrivate: ((privateStyle && privateStyle.textFieldHeight)? privateStyle.textFieldHeight : 60);
    property int buttonHeightPrivate: ((privateStyle && privateStyle.buttonSize)? privateStyle.buttonSize : 42);

    property int screenHeight: screen.height
    property int screenWidth: screen.width
    property int screenPortraitHeight: Math.max(screen.displayWidth, screen.displayHeight)
    property int screenPortraitWidth: Math.min(screen.displayWidth, screen.displayHeight)
    property bool isPortrait: (screen.width < screen.height);
    property bool isNokiaE6: (screen.height == 480 || screen.width == 480)

    property int pageOrientation_LockPortrait: PageOrientation.LockPortrait
    property int dialogStatus_Closed: DialogStatus.Closed
    property int dialogStatus_Open: DialogStatus.Open
    property int dialogStatus_Opening: DialogStatus.Opening

    property bool isVirtualKeyboardOpen: inputContext.visible
    property real virtualKeyboardHeight: inputContext.height

    property int listItemImplicitHeight: platformStyle.graphicSizeLarge
    property string listItemHightlightImage: "../native/images/qt_components_graphics/qtg_fr_list_pressed" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg"
    property string listSubItemIndicatorImage: "../native/images/qt_components_graphics/qtg_graf_drill_down_indicator" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg"
    property string listHeadingImage: "../native/images/qt_components_graphics/qtg_fr_list_heading_normal" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg"
    property color listHeaderTextColor: ((!isDarkColorScheme)? platformStyle.colorNormalLightInverted : platformStyle.colorNormalLight)
    property color listTitleTextColor: ((!isDarkColorScheme)? platformStyle.colorNormalLightInverted : platformStyle.colorNormalLight)
    property color listSubtitleTextColor: ((!isDarkColorScheme)? platformStyle.colorNormalLightInverted : platformStyle.colorNormalLight)

    property string tabBarBgImage: "../native/images/qt_components_graphics/qtg_fr_tab_bar" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string textFieldBgEditable: "../native/images/qt_components_graphics/qtg_fr_textfield_editable" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string textFieldBgHighlighted: "../native/images/qt_components_graphics/qtg_fr_textfield_highlighted" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string searchIndicatorIcon: "../native/images/qt_components_graphics/qtg_graf_search_indicator" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string clearIconNormal: "../native/images/qt_components_graphics/qtg_graf_textfield_clear_normal" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property string clearIconPressed: "../native/images/qt_components_graphics/qtg_graf_textfield_clear_pressed" + ((!isDarkColorScheme)? "_inverse" : "") + ".svg";
    property color placeholderTextColor: ((!isDarkColorScheme)? platformStyle.colorNormalMidInverted : platformStyle.colorNormalMid)

    function playThemeEffect(type) {
        if (!privateStyle) {
            return;
        }
        if (type == "BasicButton") {
            privateStyle.play(Symbian.BasicButton);
        } else if (type == "PopupOpen") {
            privateStyle.play(Symbian.PopupOpen);
        } else if (type == "PopupClose") {
            privateStyle.play(Symbian.PopupClose);
        } else {
            privateStyle.play(Symbian.BasicItem);
        }
    }
}
