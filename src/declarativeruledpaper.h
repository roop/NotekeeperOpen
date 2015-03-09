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

#ifndef DECLARATIVERULEDPAPER_H
#define DECLARATIVERULEDPAPER_H

#include <QDeclarativeItem>
#include <QPainter>
#include <QFontMetricsF>

class DeclarativeRuledPaper : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily)
    Q_PROPERTY(qreal fontPixelSize READ fontPixelSize WRITE setFontPixelSize)
    Q_PROPERTY(qreal lineSpacing READ lineSpacing NOTIFY lineSpacingChanged)
    Q_PROPERTY(qreal descent READ descent NOTIFY descentChanged)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(int penWidth READ penWidth WRITE setPenWidth NOTIFY penWidthChanged)
    Q_PROPERTY(int headerHeight READ headerHeight WRITE setHeaderHeight NOTIFY headerHeightChanged)

public:
    explicit DeclarativeRuledPaper(QDeclarativeItem *parent = 0);

    void setFontFamily(const QString &fontFamily);
    QString fontFamily() const;

    void setFontPixelSize(int pixelSize);
    qreal fontPixelSize() const;

    qreal lineSpacing() const;
    qreal descent() const;
    qreal xHeight() const;

    void setLineColor(const QColor &color);
    QColor lineColor() const;

    void setPenWidth(int width);
    int penWidth() const;

    void setHeaderHeight(int pixels);
    int headerHeight() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

signals:
    void lineSpacingChanged();
    void descentChanged();
    void lineColorChanged();
    void penWidthChanged();
    void headerHeightChanged();

private:
    QFont m_font;
    QFontMetricsF m_fontMetrics;
    QColor m_lineColor;
    int m_penWidth;
    int m_headerHeight;
};

QML_DECLARE_TYPE(DeclarativeRuledPaper)

#endif // RULEDPAPER_H
