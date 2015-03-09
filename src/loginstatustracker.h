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

#ifndef LOGINSTATUSTRACKER_H
#define LOGINSTATUSTRACKER_H

#include <QObject>

class EvernoteOAuth;
class QmlDataAccess;

class LoginStatusTracker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn WRITE setLoggedIn NOTIFY isLoggedInChanged)
public:
    explicit LoginStatusTracker(EvernoteOAuth *evernoteOAuth, QmlDataAccess *qmlDataAccess, QObject *parent = 0);
    
    void setLoggedIn(bool loggedIn);
    bool isLoggedIn() const;

signals:
    void isLoggedInChanged(bool loggedIn);

private slots:
    void setStatusLoggedIn();
    void setStatusLoggedOut();

private:
    bool m_isLoggedIn;
};

#endif // LOGINSTATUSTRACKER_H
