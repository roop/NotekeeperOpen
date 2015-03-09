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

#include "declarativeruledpaper.h"
#include <QStyleOptionGraphicsItem>
#include <math.h>
#include "qplatformdefs.h"

DeclarativeRuledPaper::DeclarativeRuledPaper(QDeclarativeItem *parent) :
    QDeclarativeItem(parent),
    m_font(QFont()), m_fontMetrics(QFontMetricsF(m_font)),
    m_lineColor(Qt::black), m_penWidth(0)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true); // for making use of exposedRect in paint()
}

void DeclarativeRuledPaper::setFontFamily(const QString &fontFamily)
{
    m_font.setFamily(fontFamily);
    m_fontMetrics = QFontMetricsF(m_font);
    emit lineSpacingChanged();
    emit descentChanged();
}

QString DeclarativeRuledPaper::fontFamily() const
{
    return m_font.family();
}

void DeclarativeRuledPaper::setFontPixelSize(int pixelSize)
{
    m_font.setPixelSize(pixelSize);
    m_fontMetrics = QFontMetricsF(m_font);
    emit lineSpacingChanged();
    emit descentChanged();
}

qreal DeclarativeRuledPaper::fontPixelSize() const
{
    return m_font.pixelSize();
}

qreal DeclarativeRuledPaper::lineSpacing() const
{
    return m_fontMetrics.lineSpacing();
}

qreal DeclarativeRuledPaper::descent() const
{
    return m_fontMetrics.descent();
}

qreal DeclarativeRuledPaper::xHeight() const
{
    return m_fontMetrics.xHeight();
}

void DeclarativeRuledPaper::setLineColor(const QColor &color)
{
    m_lineColor = color;
    emit lineColorChanged();
}

QColor DeclarativeRuledPaper::lineColor() const
{
    return m_lineColor;
}

void DeclarativeRuledPaper::setPenWidth(int width)
{
    m_penWidth = width;
    emit penWidthChanged();
}

int DeclarativeRuledPaper::penWidth() const
{
    return m_penWidth;
}

void DeclarativeRuledPaper::setHeaderHeight(int pixels)
{
    m_headerHeight = pixels;
    emit headerHeightChanged();
}

int DeclarativeRuledPaper::headerHeight() const
{
    return m_headerHeight;
}

void DeclarativeRuledPaper::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);

    QRectF rect = option->exposedRect;
    if (rect.width() == 0 || rect.height() == 0) {
        return;
    }
    painter->setPen(QPen(m_lineColor, m_penWidth));
    qreal rh = rect.height();
    qreal rw = rect.width();
    qreal rx = rect.x();
    qreal ry = rect.y();
    qreal ls = lineSpacing();
    int hh = headerHeight();
    qreal d = descent();
#ifdef MEEGO_EDITION_HARMATTAN
    qreal xh = xHeight();
#endif
    qreal yOffset = 0;
    if (ry < hh) {
        yOffset = hh - ry;
    } else {
        yOffset = ls - fmod(ry - hh, ls); // fmod = floating-point remainder
    }
    for (qreal y = ry + yOffset; y < (ry + rh + ls); y += ls) {
#ifdef MEEGO_EDITION_HARMATTAN
        painter->drawLine(rx, y - d + xh + 2, rx + rw, y - d + xh + 2);
#else
        painter->drawLine(rx, y - d, rx + rw, y - d);
#endif
    }
}
