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

#include "qthrifthttpclient.h"
#include "evernoteaccess.h"
#include <QNetworkReply>
#include <QEventLoop>
#include <QCoreApplication>
#include <QStringBuilder>
#include <QTimer>
#include <QtDebug>

// #define DEBUG

#ifdef DEBUG
#include <QtDebug>
#endif

static void waitForReadyReadSignal(QNetworkReply *reply, EvernoteAccess *evernoteAccess)
{
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));
    if (evernoteAccess) {
        QObject::connect(evernoteAccess, SIGNAL(cancelled()), &loop, SLOT(quit()));
        QObject::connect(evernoteAccess, SIGNAL(sslErrorsFound()), &loop, SLOT(quit()));
        if (evernoteAccess->isSslErrorFound()) {
            throw apache::thrift::transport::TTransportException(apache::thrift::transport::TTransportException::INTERRUPTED);
            return;
        }
    }
    if (reply->bytesAvailable() == 0) {
#ifdef DEBUG
        qDebug() << "QThriftHttpClient::read/All Waiting for readyRead on reply " << reply;
#endif
        QTimer timer;
        timer.setInterval(10 * 60 * 1000); // 10 mins
        timer.setSingleShot(true);
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        timer.start();
        loop.exec();
        if (!timer.isActive()) { // timed out
#ifdef DEBUG
        qDebug() << "QThriftHttpClient::read/All Wait for readyRead timed out";
#endif
            throw apache::thrift::transport::TTransportException(apache::thrift::transport::TTransportException::TIMED_OUT);
        }
        if (evernoteAccess && evernoteAccess->isSslErrorFound()) {
            throw apache::thrift::transport::TTransportException(apache::thrift::transport::TTransportException::INTERRUPTED);
        }
    }
}

QThriftHttpClient::QThriftHttpClient(const QUrl &url, EvernoteAccess *evernoteAccess)
    : m_url(url), m_evernoteAccess(evernoteAccess), m_networkAccessManager(new QNetworkAccessManager), m_reply(0), m_isOpened(false), m_cancelled(false)
{
    Q_ASSERT(url.scheme() == "http" || url.scheme() == "https");
}

QThriftHttpClient::~QThriftHttpClient()
{
    delete m_networkAccessManager;
}

void QThriftHttpClient::open()
{
    if (!m_isOpened) {
        m_request = QNetworkRequest(m_url);
        m_request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-thrift");
        m_request.setRawHeader("Accept", "application/x-thrift");
        QString userAgentString = qApp->applicationName() % "/v" % qApp->applicationVersion();
#if defined(Q_OS_SYMBIAN)
        userAgentString = userAgentString % "/Symbian";
#elif defined(MEEGO_EDITION_HARMATTAN)
        userAgentString = userAgentString % "/Meego";
#endif
        m_request.setRawHeader("User-Agent", userAgentString.toAscii());

        m_isOpened = true;
        m_dataToPost.clear();
#ifdef DEBUG
        m_debugBa.clear();
#endif
    }
}

void QThriftHttpClient::close()
{
    m_isOpened = false;
    if (m_reply) {
        m_reply->close();
        delete m_reply;
        m_reply = 0;
    }
    m_dataToPost.clear();
#ifdef DEBUG
    m_debugBa.clear();
#endif
}

bool QThriftHttpClient::isOpen()
{
    return m_isOpened;
}

bool QThriftHttpClient::peek()
{
    return (m_reply->bytesAvailable() > 0);
}

uint32_t QThriftHttpClient::read_virt(uint8_t* buf, uint32_t len)
{
    while (m_reply->bytesAvailable() == 0 && !m_reply->isFinished()) {
        waitForReadyReadSignal(m_reply, m_evernoteAccess);
        if (cancelled())
            throw apache::thrift::transport::TTransportException(apache::thrift::transport::TTransportException::INTERRUPTED);
    }

    uint32_t bytesRead = static_cast<uint32_t>(m_reply->read(reinterpret_cast<char*>(buf), len));
#ifdef DEBUG
    m_debugBa.append(reinterpret_cast<char*>(buf), bytesRead);
#endif
    return bytesRead;
}

uint32_t QThriftHttpClient::readAll_virt(uint8_t* buf, uint32_t len)
{
    int32_t bytesRead = 0;
    while (bytesRead < len) {
        while (m_reply->bytesAvailable() == 0 && !m_reply->isFinished()) {
            waitForReadyReadSignal(m_reply, m_evernoteAccess);
            if (cancelled())
                throw apache::thrift::transport::TTransportException(apache::thrift::transport::TTransportException::INTERRUPTED);
        }
        int32_t bytesReadThisTime = static_cast<uint32_t>(m_reply->read(reinterpret_cast<char*>(buf + bytesRead), len - bytesRead));
        bytesRead += bytesReadThisTime;
    }
    Q_ASSERT(bytesRead == len);
#ifdef DEBUG
    m_debugBa.append(reinterpret_cast<char*>(buf), bytesRead);
#endif
    return bytesRead;
}

uint32_t QThriftHttpClient::readEnd()
{
#ifdef DEBUG
    qDebug() << "QThriftHttpClient::readEnd total data read(" << m_debugBa.size() << "bytes )";
    m_debugBa.clear();
#endif
    return 0;
}

void QThriftHttpClient::write_virt(const uint8_t* buf, uint32_t len)
{
    m_dataToPost.append(reinterpret_cast<const char*>(buf), len);
}

uint32_t QThriftHttpClient::writeEnd()
{
    return 0;
}

void QThriftHttpClient::flush()
{
    m_request.setHeader(QNetworkRequest::ContentLengthHeader, m_dataToPost.size());
    delete m_reply;
    m_reply = m_networkAccessManager->post(m_request, m_dataToPost);
    QObject::connect(m_reply, SIGNAL(sslErrors(QList<QSslError>)), m_evernoteAccess, SLOT(handleSslErrors(QList<QSslError>)));
#ifdef DEBUG
    qDebug() << "QThriftHttpClient::writeEnd total data written (" << m_dataToPost.size() << "bytes )";
    qDebug() << "QThriftHttpClient::flush POST request sent. reply = " << m_reply;
#endif
    m_dataToPost.clear();
}

const uint8_t* QThriftHttpClient::borrow_virt(uint8_t* buf, uint32_t* len)
{
    if (buf) {
        int bytesPeeked = m_reply->peek(reinterpret_cast<char*>(buf), (*len));
        if (bytesPeeked == (*len)) {
            return buf;
        }
    }
    return NULL;
}

void QThriftHttpClient::consume_virt(uint32_t len)
{
    m_reply->read(len);
}

QNetworkAccessManager* QThriftHttpClient::networkAccessManager() const
{
    return m_networkAccessManager;
}

void QThriftHttpClient::setNetworkConfiguration(const QNetworkConfiguration &config)
{
    m_networkAccessManager->setConfiguration(config);
}

QNetworkConfiguration QThriftHttpClient::networkConfiguration() const
{
    return m_networkAccessManager->configuration();
}
