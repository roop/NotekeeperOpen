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

#ifndef QTHRIFTHTTPCLIENT_H
#define QTHRIFTHTTPCLIENT_H

#include <QtNetwork/QNetworkAccessManager>
#include <QUrl>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslConfiguration>

// thrift
#include <transport/THttpTransport.h>

class EvernoteAccess;

class QThriftHttpClient : public apache::thrift::transport::TTransport
{
public:
    explicit QThriftHttpClient(const QUrl &url, EvernoteAccess *evernoteSync);
    virtual ~QThriftHttpClient();

    virtual bool isOpen();
    virtual bool peek();
    virtual void open();
    virtual void close();
    virtual uint32_t read_virt(uint8_t* buf, uint32_t len);
    virtual uint32_t readAll_virt(uint8_t* buf, uint32_t len);
    virtual uint32_t readEnd();
    virtual void write_virt(const uint8_t* buf, uint32_t len);
    virtual uint32_t writeEnd();
    virtual void flush();

    // Borrow returns data without consuming (a subsequent read() will return the same data).
    // consume() actually consumes().
    // Can borrrow(n bytes) followed by consume(m bytes), as long as m<=n.
    virtual const uint8_t* borrow_virt(uint8_t* buf, uint32_t* len);
    virtual void consume_virt(uint32_t len);

    QNetworkAccessManager *networkAccessManager() const;
    void setNetworkConfiguration(const QNetworkConfiguration &config);
    QNetworkConfiguration networkConfiguration() const;

    void setCancelled(bool cancelled) { m_cancelled = cancelled; }
    bool cancelled() const { return m_cancelled; }

private:
    EvernoteAccess *m_evernoteAccess;
    QUrl m_url;
    QNetworkAccessManager *m_networkAccessManager;
    QNetworkRequest m_request;
    QNetworkReply *m_reply;
    bool m_isOpened;
    QByteArray m_dataToPost;
    QByteArray m_debugBa;
    bool m_cancelled;
};

#endif // QTHRIFTHTTPCLIENT_H
