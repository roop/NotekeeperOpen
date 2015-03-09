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

#ifndef NOTEKEEPERSHAREUIMETHOD_H
#define NOTEKEEPERSHAREUIMETHOD_H

#include <ShareUI/MethodBase>

class NotekeeperShareUIMethod : public ShareUI::MethodBase
{
    Q_OBJECT
public:
    explicit NotekeeperShareUIMethod(QObject *parent = 0);
    virtual ~NotekeeperShareUIMethod();

    virtual QString id();
    virtual QString title();
    virtual QString subtitle();
    virtual QString icon();

public slots:
    void currentItems(const ShareUI::ItemContainer *items);
    void selected(const ShareUI::ItemContainer *items);

protected:
    virtual QString appExecName() const = 0;
    virtual QString appDataDirName() const { return appExecName(); }

private:
    QString notekeeperSessionDirPath() const;
    QString currentlyLoggedInUser();
    bool showInShareMenu(const ShareUI::ItemContainer *items);

    bool m_isPixmapDirAdded;
};

class NotekeeperOpenShareUIMethod : public NotekeeperShareUIMethod
{
    Q_OBJECT
public:
    explicit NotekeeperOpenShareUIMethod(QObject *parent = 0) : NotekeeperShareUIMethod(parent) { }
protected:
    virtual QString title() { return "Notekeeper Open"; }
    virtual QString appExecName() const { return "notekeeper-open"; }
    virtual QString appDataDirName() const { return "notekeeper-open"; }
};

#endif // NOTEKEEPERSHAREUIMETHOD_H
