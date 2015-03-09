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

#include "connectionmanager.h"
#include "storage/storagemanager.h"
#include "loginstatustracker.h"
#include "qplatformdefs.h"
#include <QTimer>
#include <QCoreApplication>

ConnectionManager::ConnectionManager(StorageManager *storageManager, QObject *parent)
    : QObject(parent)
    , m_session(new QNetworkSession(m_configManager.defaultConfiguration(), this))
    , m_isScanningForWifi(false)
    , m_isSessionUseRegistered(false)
    , m_connectionState(InitialState)
    , m_storageManager(storageManager)
#if defined(Q_OS_SYMBIAN)
    , m_isWifiAutoConnectEnabled(true)
#else
    , m_isWifiAutoConnectEnabled(false)
#endif
    , m_isOnline(false)
{
    connect(&m_configManager, SIGNAL(updateCompleted()), SLOT(scanFinished()));
    connect(&m_configManager, SIGNAL(onlineStateChanged(bool)), SLOT(onlineStateChanged(bool)));
    if (m_configManager.isOnline()) {
        onlineStateChanged(true);
    }
    QTimer::singleShot(0, this, SLOT(startScanningForNetworks()));
    connect(m_session, SIGNAL(stateChanged(QNetworkSession::State)), SLOT(sessionStateChanged(QNetworkSession::State)));
    m_storageManager->log("ConnectionManager::ConnectionManager()");
    ensureConnectionStateIsUpToDate();
}

ConnectionManager::~ConnectionManager()
{
    if (m_isSessionUseRegistered) {
        m_session->close();
    }
}

void ConnectionManager::startScanningForNetworks()
{
    m_storageManager->log(QString("ConnectionManager::startScanningForNetworks() alreadyScanning=%1").arg(m_isScanningForWifi));
    if (!m_isScanningForWifi) {
        m_isScanningForWifi = true;
        emit scanningForWifiChanged();
        m_configManager.updateConfigurations();
    }
}

static QNetworkConfiguration chooseIapConfiguration(const QList<QNetworkConfiguration> &configList, QNetworkConfiguration::StateFlags state, QNetworkConfiguration::BearerType bearerType)
{
    foreach (const QNetworkConfiguration &nwConf, configList) {
        if ((nwConf.type() == QNetworkConfiguration::InternetAccessPoint) &&
            ((nwConf.state() & state) == state) &&
            ((bearerType == QNetworkConfiguration::BearerUnknown) || (nwConf.bearerType() == bearerType))) {
            return nwConf;
        }
    }
    return QNetworkConfiguration();
}

static QNetworkConfiguration chooseIapConfiguration(const QList<QNetworkConfiguration> &configList)
{
    QNetworkConfiguration nwConf;
    // Choose the active configuration, if any
    nwConf = chooseIapConfiguration(configList, QNetworkConfiguration::Active, QNetworkConfiguration::BearerUnknown);
    if (nwConf.isValid()) {
        return nwConf;
    }
    // Choose a Wifi configuration, if there's one in the range
    nwConf = chooseIapConfiguration(configList, QNetworkConfiguration::Discovered, QNetworkConfiguration::BearerWLAN);
    if (nwConf.isValid()) {
        return nwConf;
    }
    // Choose something
    nwConf = chooseIapConfiguration(configList, QNetworkConfiguration::Discovered, QNetworkConfiguration::BearerUnknown);
    if (nwConf.isValid()) {
        return nwConf;
    }
    return QNetworkConfiguration();
}

QNetworkConfiguration ConnectionManager::primaryIapConfiguration()
{
    const QNetworkConfiguration &nwConf = m_session->configuration();
    if (!nwConf.isValid()) {
        return nwConf;
    }
    if (nwConf.type() == QNetworkConfiguration::ServiceNetwork) {
        return chooseIapConfiguration(nwConf.children());
    }
    if (nwConf.type() == QNetworkConfiguration::UserChoice) {
        return chooseIapConfiguration(m_configManager.allConfigurations());
    }
    return nwConf;
}

QNetworkConfiguration ConnectionManager::activeIapConfiguration()
{
    QList<QNetworkConfiguration> activeConfigs = m_configManager.allConfigurations(QNetworkConfiguration::Active);
    foreach (const QNetworkConfiguration &nwConf, activeConfigs) {
        if (nwConf.type() == QNetworkConfiguration::InternetAccessPoint) {
            return nwConf;
        }
    }
    return QNetworkConfiguration();
}

void ConnectionManager::scanFinished()
{
    m_storageManager->log("ConnectionManager::scanFinished()");
    m_isScanningForWifi = false;
    emit scanningForWifiChanged();

    ensureConnectionStateIsUpToDate();
    if (m_connectionState == Connected || m_connectionState == Connecting) {
        return;
    }

    m_storageManager->log(QString("scanFinished() current_connection_state=%1 (%2)").arg(static_cast<int>(m_connectionState)).arg(connectionStateString()));
#if defined(Q_OS_SYMBIAN)
    checkIAPOrdering(m_session->configuration());
#endif
    QNetworkConfiguration availableConfig = primaryIapConfiguration();
    if (availableConfig.isValid()) {
        m_storageManager->log(QString("scanFinished() Selected config: bearer_type=%1").arg(static_cast<int>(availableConfig.bearerType())));
        if (m_connectionState == InitialState || m_connectionState == WillConnect || m_connectionState == Disconnected) {
            if (availableConfig.bearerType() == QNetworkConfiguration::BearerWLAN) {
                if ((availableConfig.state() == QNetworkConfiguration::Active) || m_isWifiAutoConnectEnabled) {
                    m_storageManager->log("scanFinished() Opening session");
                    m_session->open();
                    m_isSessionUseRegistered = true;
                    m_connectionState = Connecting;
                } else {
                    m_connectionState = WillConnect;
                }
                emit connectionStateChanged();
            } else {
                m_connectionState = WillConnect;
                emit connectionStateChanged();
            }
            if (m_iapConfiguration != availableConfig) {
                m_iapConfiguration = availableConfig;
                emit iapConfigurationChanged();
            }
        }
    } else {
        m_storageManager->log("scanFinished() No IAPs available");
    }
}

void ConnectionManager::checkIAPOrdering(const QNetworkConfiguration &nwConf)
{
    // check if there an available non-wifi Internet Access Point before an available wi-fi Internet Access Point
    if (nwConf.type() == QNetworkConfiguration::ServiceNetwork) {
        bool mobileDataConnectionFound = false;
        QNetworkConfiguration childMobileDataNwConf;
        foreach (const QNetworkConfiguration &childNwConf, nwConf.children()) {
            if ((childNwConf.state() & QNetworkConfiguration::Discovered) != QNetworkConfiguration::Discovered) {
                continue;
            }
            QNetworkConfiguration::BearerType bearerType = childNwConf.bearerType();
            if (bearerType == QNetworkConfiguration::BearerUnknown || bearerType == QNetworkConfiguration::BearerEthernet || bearerType == QNetworkConfiguration::BearerBluetooth) {
                continue;
            }
            if (bearerType == QNetworkConfiguration::BearerWLAN) {
                // wifi
                if (mobileDataConnectionFound) {
                    QString title("Unusual access points order");
                    QString msg = QString("Mobile data connection '%1' is listed before wifi connection '%2' "
                                          "in the default network destination in the phone's settings. "
                                          "This would cause %3 to use mobile data for "
                                          "Internet connectivity even though '%2' is available.")
                                  .arg(childMobileDataNwConf.name()).arg(childNwConf.name()).arg(qApp->applicationName());
                    emit showConnectionMessage("UNUSUAL_ACCESS_POINT_ORDERING", title, msg);
                }
            } else {
                // mobile data
                mobileDataConnectionFound = true;
                childMobileDataNwConf = childNwConf;
            }
        }
    }
}

void ConnectionManager::setCurrentIapConfigurationToWhateverIsActive()
{
    QNetworkConfiguration iapConfig = activeIapConfiguration();
    if (m_iapConfiguration != iapConfig) {
        m_iapConfiguration = iapConfig;
        emit iapConfigurationChanged();
    }
}

void ConnectionManager::setCurrentIapConfigurationToWhateverIsPrimary()
{
    QNetworkConfiguration iapConfig = primaryIapConfiguration();
    if (m_iapConfiguration != iapConfig) {
        m_iapConfiguration = iapConfig;
        emit iapConfigurationChanged();
    }
}

void ConnectionManager::invalidateCurrentIapConfiguration()
{
    if (m_iapConfiguration.isValid()) {
        m_iapConfiguration = QNetworkConfiguration();
        emit iapConfigurationChanged();
    }
}

QString ConnectionManager::connectionStateString()
{
    switch (m_connectionState) {
#if defined(Q_OS_SYMBIAN)
    case WillConnect: return "Will connect";
#else
    case WillConnect: return "Can connect";
#endif
    case Connecting: return "Connecting";
    case Connected: return "Connected";
    case Disconnected: return "Disconnected";
    case InitialState:
    default:
        return "Not connected";
    }
}

bool ConnectionManager::isScanningForWifi() const
{
    return m_isScanningForWifi;
}

QString ConnectionManager::iapConfigurationName() const
{
    return m_iapConfiguration.name();
}

bool ConnectionManager::isIapConfigurationWifi() const
{
    return (m_iapConfiguration.bearerType() == QNetworkConfiguration::BearerWLAN);
}

QNetworkConfiguration ConnectionManager::selectedConfiguration() const
{
    if (m_session && m_connectionState == Connected) {
        return m_session->configuration();
    }
    return QNetworkConfiguration();
}

bool ConnectionManager::requestConnection(bool *alreadyConnected)
{
    m_storageManager->log(QString("ConnectionManager::requestConnection() current_connection_state=%1 (%2)").arg(static_cast<int>(m_connectionState)).arg(connectionStateString()));
    ensureConnectionStateIsUpToDate();

    if (m_connectionState == Connecting) {
        (*alreadyConnected) = false;
        return true;
    }

    if (m_connectionState == Connected) {
        (*alreadyConnected) = true;
        return true;
    }

    (*alreadyConnected) = false;

    QNetworkConfiguration availableConfig = primaryIapConfiguration();
    if (availableConfig.isValid()) {
        m_storageManager->log(QString("requestConnection() Selected config: bearer_type=%1").arg(static_cast<int>(availableConfig.bearerType())));
        m_storageManager->log("requestConnection() Opening session");
        m_session->open();
        m_isSessionUseRegistered = true;
        m_connectionState = Connecting;
        emit connectionStateChanged();
    } else {
        m_storageManager->log("requestConnection() No IAPs available");
    }

    return availableConfig.isValid();
}

void ConnectionManager::sessionStateChanged(QNetworkSession::State state)
{
    m_storageManager->log(QString("ConnectionManager::sessionStateChanged(%1) isOnline(%2)").arg(static_cast<int>(state)).arg(m_isOnline));
    if (state == QNetworkSession::Connecting) {
        m_connectionState = Connecting;
        emit connectionStateChanged();
    } else if (state == QNetworkSession::Connected && m_isOnline) {
        m_connectionState = Connected;
        emit connectionStateChanged();
        emit connected();
    } else if (state == QNetworkSession::Disconnected || state == QNetworkSession::NotAvailable) {
        m_connectionState = Disconnected;
        emit connectionStateChanged();
        emit disconnected();
    }

    if (state == QNetworkSession::Connecting) {
        setCurrentIapConfigurationToWhateverIsPrimary();
    } else if (state == QNetworkSession::Connected) {
        setCurrentIapConfigurationToWhateverIsActive();
    } else if (state == QNetworkSession::Disconnected || state == QNetworkSession::NotAvailable) {
        invalidateCurrentIapConfiguration();
    }

    m_storageManager->log(QString("sessionStateChanged() current_connection_state=%1 (%2)").arg(static_cast<int>(m_connectionState)).arg(connectionStateString()));
    if (m_iapConfiguration.isValid()) {
        m_storageManager->log(QString("sessionStateChanged() Selected config: bearer_type=%1").arg(static_cast<int>(m_iapConfiguration.bearerType())));
    } else {
        m_storageManager->log("sessionStateChanged() No IAP config selected yet");
    }
}

void ConnectionManager::onlineStateChanged(bool isOnline)
{
    // connectionState becomes 'Connected' only when (m_session->state() == QNetworkSession::Connected && m_configManager.isOnline()) is true
    m_storageManager->log(QString("ConnectionManager::onlineStateChanged(%1) sessionState(%2)").arg(isOnline).arg(static_cast<int>(m_session->state())));
    m_isOnline = isOnline;
    if (isOnline && m_session->state() == QNetworkSession::Connected) {
        if (m_connectionState != Connected) {
            m_connectionState = Connected;
            setCurrentIapConfigurationToWhateverIsActive();
            emit connectionStateChanged();
            emit connected();
        }
    }
    if (!isOnline) {
        if (m_connectionState != Disconnected) {
            m_connectionState = Disconnected;
            invalidateCurrentIapConfiguration();
            emit connectionStateChanged();
            emit disconnected();
        }
    }

    m_storageManager->log(QString("onlineStateChanged(%1) current_connection_state=%2 (%3)").arg(isOnline).arg(static_cast<int>(m_connectionState)).arg(connectionStateString()));
    if (m_iapConfiguration.isValid()) {
        m_storageManager->log(QString("onlineStateChanged(%1) Selected config: bearer_type=%2").arg(isOnline).arg(static_cast<int>(m_iapConfiguration.bearerType())));
    } else {
        m_storageManager->log(QString("onlineStateChanged(%1) No IAP config selected yet").arg(isOnline));
    }
}

void ConnectionManager::ensureConnectionStateIsUpToDate()
{
    QNetworkSession::State sessionState = m_session->state();
    if ((sessionState == QNetworkSession::Connected && m_connectionState != Connected) ||
        (sessionState == QNetworkSession::Connecting && m_connectionState != Connecting)) {
        // looks like we haven't realized yet that the state has changed
        m_storageManager->log(QString("ConnectionManager::ensureConnectionStateIsUpToDate() Session state = %1").arg(static_cast<int>(sessionState)));
        sessionStateChanged(sessionState);
        ensureSessionUseIsRegistered();
    }
}

void ConnectionManager::ensureSessionUseIsRegistered()
{
    if (!m_isSessionUseRegistered) {
        m_storageManager->log("ConnectionManager::ensureSessionUseIsRegistered() Opening session to register interface");
        m_session->open(); // register use of interface
        m_isSessionUseRegistered = true;
    }
}
