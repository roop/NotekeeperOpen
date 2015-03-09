/*
  This file is part of Notekeeper Open and is licensed under the MIT License

  Copyright (C) 2011-2015 Roopesh Chander <roop@roopc.net>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject
  to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

import QtQuick 1.1
import "../native"

ListView {
    id: listView
    property variant window;

    anchors.fill: parent

    property string modelKeyUrl: "url";
    property string modelKeyWidth: "width";
    property string modelKeyHeight: "height";
    property bool useModelData: false;

    signal clicked()

    QtObject {
        id: internal
        property Item previousItem;
        property bool isSnapped: true;
        property bool pinchPanEnabled: (!listView.flicking && !listView.moving && internal.isSnapped);
        // when the phone is rotated, if the highlightRangeMode is set to StrictlyEnforceRange
        // it might cause the currentIndex to change (because contentX doesn't change)
        function onOrientationChangeAboutToStart() {
            listView.highlightRangeMode = ListView.NoHighlightRange;
        }
        function onOrientationChangeFinished() {
            if (listView.highlightRangeMode != ListView.StrictlyEnforceRange) {
                listView.positionViewAtIndex(listView.currentIndex, listView.Contain);
                listView.highlightRangeMode = ListView.StrictlyEnforceRange;
            }
        }
        Component.onCompleted: {
            root.window.orientationChangeAboutToStart.connect(onOrientationChangeAboutToStart);
            root.window.orientationChangeFinished.connect(onOrientationChangeFinished);
        }
        Component.onDestruction: {
            root.window.orientationChangeAboutToStart.disconnect(onOrientationChangeAboutToStart);
            root.window.orientationChangeFinished.disconnect(onOrientationChangeFinished);
        }
    }

    onContentXChanged: {
        // when swipe between images is ongoing, disable pinch-zoom
        if (listView.currentItem) {
            internal.isSnapped = (mapFromItem(listView.currentItem, 0, 0).x === 0);
        }
    }

    orientation: ListView.Horizontal
    snapMode: ListView.SnapOneItem
    highlightRangeMode: ListView.StrictlyEnforceRange
    boundsBehavior: Flickable.DragOverBounds
    cacheBuffer: ((count <= 3)? 0 : listView.width)
    focus: true
    delegate: Component {
        ImagePinchZoomer {
            width: listView.width
            height: listView.height
            imageSource: (listView.useModelData? modelData[listView.modelKeyUrl] : model[listView.modelKeyUrl])
            actualImageWidth: (listView.useModelData? modelData[listView.modelKeyWidth] : model[listView.modelKeyWidth])
            actualImageHeight: (listView.useModelData? modelData[listView.modelKeyHeight] : model[listView.modelKeyHeight])
            pinchPanEnabled: internal.pinchPanEnabled
            BusyIndicator {
                anchors.centerIn: parent
                running: (parent.imageStatus == Image.Loading)
                opacity: (running? 1 : 0)
                colorScheme: (_UI.isDarkColorScheme? Qt.white : Qt.black)
            }
            onGoToNextImage: {
                listView.goToIndex(model.index + 1);
            }
            onGoToPreviousImage: {
                listView.goToIndex(model.index - 1);
            }
            onZoomedOnXChanged: {
                if (model.index == listView.currentIndex) {
                    listView.interactive = (!zoomedOnX);
                }
            }
            onClicked: { listView.clicked(); }
        }
    }
    onCurrentItemChanged: {
        if (internal.previousItem && internal.previousItem != currentItem) {
            internal.previousItem.delayedResetZoom();
        }
        internal.previousItem = currentItem;
        listView.interactive = true;
    }
    onFlickingChanged: {
        eventGrabber.enabled = flicking;
    }

    MouseArea {
        id: eventGrabber
        anchors.fill: parent
        enabled: false
    }

    function goToIndex(i) {
        listView.currentIndex = Math.min(Math.max(i, 0), listView.count - 1);
    }

    function goNext() {
        goToIndex(listView.currentIndex + 1);
    }

    function goPrevious() {
        goToIndex(listView.currentIndex - 1);
    }
}
