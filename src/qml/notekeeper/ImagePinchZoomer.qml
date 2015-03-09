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

Item {
    id: root
    property int actualImageWidth: 0;
    property int actualImageHeight: 0;
    property string imageSource: ""
    property alias imageStatus: zoomableImage.status
    property bool zoomedOnX: (flickable.contentWidth > flickable.width)
    property bool pinchPanEnabled: true

    signal goToNextImage()
    signal goToPreviousImage()
    signal clicked()

    QtObject {
        id: internal
        property int maxSourceSizeSide: 1200;
        property double sourceSizeScale: Math.min(1.0, maxSourceSizeSide / Math.max(root.actualImageWidth, root.actualImageHeight));
        property int sourceSizeWidth: root.actualImageWidth * sourceSizeScale // scale down memory footprint if image is huge
        property int sourceSizeHeight: root.actualImageHeight * sourceSizeScale
        property double scaleToFit: Math.min(1.0, Math.min(root.width / sourceSizeWidth, root.height / sourceSizeHeight));
        property int scrolledToRightEdgeCount: 0
        property int scrolledToLeftEdgeCount: 0
        property bool isPortrait: _UI.isPortrait;
        onIsPortraitChanged: {
            flickable.contentX = 0;
            flickable.contentY = 0;
            zoomableImage.scale = 1; // reset zoom
        }
    }
    PinchArea {
        id: pinchArea
        anchors.fill: parent
        enabled: root.pinchPanEnabled
        property double minScale: 1
        property double maxScale: 1 / internal.scaleToFit
        property int pinchStartedPointInViewportX: 0
        property int pinchStartedPointInViewportY: 0
        property real pinchStartedPointInImageNormalizedX: 0
        property real pinchStartedPointInImageNormalizedY: 0
        pinch {
            target: zoomableImage
            minimumScale: pinchArea.minScale * 0.8
            maximumScale: pinchArea.maxScale * 1.2
        }
        onPinchStarted: {
            pinchStartedPointInViewportX = pinch.center.x;
            pinchStartedPointInViewportY = pinch.center.y;
            pinchStartedPointInImageNormalizedX = (flickable.contentX + pinchStartedPointInViewportX) / flickable.contentWidth;
            pinchStartedPointInImageNormalizedY = (flickable.contentY + pinchStartedPointInViewportY) / flickable.contentHeight;
        }
        onPinchFinished: {
            var cx = flickable.contentX;
            var cy = flickable.contentY;
            flickable.contentX = Math.max(0, Math.min(flickable.contentWidth - flickable.width, cx));
            flickable.contentY = Math.max(0, Math.min(flickable.contentHeight - flickable.height, cy));
            zoomableImage.scale = Math.max(pinchArea.minScale, Math.min(pinchArea.maxScale, zoomableImage.scale));
        }
    }
    Flickable {
        id: flickable
        anchors.fill: parent
        contentWidth: zoomableImageContainer.width
        contentHeight: zoomableImageContainer.height
        boundsBehavior: Flickable.DragOverBounds;
        interactive: root.pinchPanEnabled
        clip: true;
        Item {
            id: zoomableImageContainer
            width: Math.max((transpose? zoomableImage.height : zoomableImage.width) * zoomableImage.scale, root.width)
            height: Math.max((transpose? zoomableImage.width : zoomableImage.height) * zoomableImage.scale, root.height)
            property bool transpose: false
            Image {
                id: zoomableImage
                anchors.centerIn: parent
                width: internal.sourceSizeWidth * internal.scaleToFit
                height: internal.sourceSizeHeight * internal.scaleToFit
                source: root.imageSource
                fillMode: Image.PreserveAspectFit;
                sourceSize { width: internal.sourceSizeWidth; height: internal.sourceSizeHeight; }
                smooth: true
                cache: true
                asynchronous: true
                onStatusChanged: {
                    if (zoomableImage.status == Image.Ready) {
                        var angle = qmlDataAccess.rotationForCorrectOrientation(root.imageSource);
                        zoomableImage.rotation = angle;
                        if (angle == 90 || angle == 270) {
                            internal.scaleToFit = Math.min(1.0, Math.min(root.width / internal.sourceSizeHeight, root.height / internal.sourceSizeWidth));
                            zoomableImageContainer.transpose = true;
                        }
                    }
                }
            }
        }
        onContentWidthChanged: {
            if (pinchArea.pinch.active) {
                var cx = (contentWidth * pinchArea.pinchStartedPointInImageNormalizedX) - pinchArea.pinchStartedPointInViewportX;
                contentX = cx;
            }
        }
        onContentHeightChanged: {
            if (pinchArea.pinch.active) {
                var cy = (contentHeight * pinchArea.pinchStartedPointInImageNormalizedY) - pinchArea.pinchStartedPointInViewportY;
                contentY = cy;
            }
        }
        onFlickStarted: {
            // if user scrolls beyond the right/left edge twice, go to next/prev image
            if (contentWidth > width) {
                if (atXEnd && horizontalVelocity > 100) {
                    if (++internal.scrolledToRightEdgeCount >= 2) {
                        root.goToNextImage();
                    }
                } else {
                    internal.scrolledToRightEdgeCount = 0;
                }
                if (atXBeginning && horizontalVelocity < -100) {
                    if (++internal.scrolledToLeftEdgeCount >= 2) {
                        root.goToPreviousImage();
                    }
                } else {
                    internal.scrolledToLeftEdgeCount = 0;
                }
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: { root.clicked(); }
        }
    }

    ScrollDecorator {
        flickableItem: flickable
    }

    Timer {
        id: delayedResetZoomTimer
        interval: 400
        onTriggered: root.resetZoom();
    }

    function resetZoom() {
        zoomableImage.scale = 1.0;
        flickable.contentX = 0;
        flickable.contentY = 0;
    }

    function delayedResetZoom() {
        delayedResetZoomTimer.start();
    }
}
