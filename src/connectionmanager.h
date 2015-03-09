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

#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QNetworkSession>
#include <QNetworkConfiguration>
#include <QNetworkConfigurationManager>
#include <QMutex>

class StorageManager;
class LoginStatusTracker;

class ConnectionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString connectionStateString READ connectionStateString NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isScanningForWifi READ isScanningForWifi NOTIFY scanningForWifiChanged)
    Q_PROPERTY(QString iapConfigurationName READ iapConfigurationName NOTIFY iapConfigurationChanged)
    Q_PROPERTY(bool isIapConfigurationWifi READ isIapConfigurationWifi NOTIFY iapConfigurationChanged)
public:
    explicit ConnectionManager(StorageManager *storageManager, QObject *parent = 0);
    ~ConnectionManager();

    QString connectionStateString();
    bool isScanningForWifi() const;
    QString iapConfigurationName() const;
    bool isIapConfigurationWifi() const;

    QNetworkConfiguration selectedConfiguration() const;
    bool requestConnection(bool *alreadyConnected);

signals:
    void connectionStateChanged();
    void scanningForWifiChanged();
    void iapConfigurationChanged();
    void connected();
    void disconnected();
    void showConnectionMessage(const QString &code, const QString &title, const QString &msg);


public slots:
    void startScanningForNetworks();
    void scanFinished();
    void sessionStateChanged(QNetworkSession::State state);
    void onlineStateChanged(bool isOnline);

private:
    QNetworkConfiguration primaryIapConfiguration();
    QNetworkConfiguration activeIapConfiguration();
    void setCurrentIapConfigurationToWhateverIsActive();
    void setCurrentIapConfigurationToWhateverIsPrimary();
    void invalidateCurrentIapConfiguration();
    void ensureConnectionStateIsUpToDate();
    void ensureSessionUseIsRegistered();
    void checkIAPOrdering(const QNetworkConfiguration &nwConf);

    enum ConnectionState {
        InitialState,
        WillConnect,
        Connecting,
        Connected,
        Disconnected
    };

    ConnectionState m_connectionState;
    bool m_isScanningForWifi;
    bool m_isSessionUseRegistered;
    QNetworkConfiguration m_iapConfiguration; // the actual InternetAccessPoint under the ServiceNetwork

    QNetworkConfigurationManager m_configManager;
    QNetworkSession *m_session;
    StorageManager *m_storageManager;
    bool m_isWifiAutoConnectEnabled;
    bool m_isOnline;
};

#endif // CONNECTIONMANAGER_H
