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

#ifndef NOTEKEEPERSHAREUIPLUGIN_H
#define NOTEKEEPERSHAREUIPLUGIN_H

#include <ShareUI/PluginBase>
#include <ShareUI/MethodBase>

class NotekeeperShareUIPlugin : public ShareUI::PluginBase
{
    Q_OBJECT
public:
    explicit NotekeeperShareUIPlugin(QObject *parent = 0);
    virtual ~NotekeeperShareUIPlugin();

    QList<ShareUI::MethodBase*> methods(QObject *parent = 0);

protected:
    virtual ShareUI::MethodBase* createNotekeeperShareUiMethod(QObject *parent = 0) = 0;
};

class NotekeeperOpenShareUIPlugin : public NotekeeperShareUIPlugin
{
    Q_OBJECT
public:
    explicit NotekeeperOpenShareUIPlugin(QObject *parent = 0) : NotekeeperShareUIPlugin(parent) { }
protected:
    ShareUI::MethodBase* createNotekeeperShareUiMethod(QObject *parent = 0);
};

#endif // NOTEKEEPERSHAREUIPLUGIN_H
