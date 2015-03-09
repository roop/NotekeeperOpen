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

#include "logger.h"
#include <QFile>
#include <QDir>
#include <QStringBuilder>
#include <QDesktopServices>
#include <QtDebug>
#include <QSystemInfo>
#include "qplatformdefs.h"

Logger::Logger(QObject *parent) : QObject(parent)
{
    m_mutex.lock();
    QString appExecName = "notekeeper-open";

#ifdef MEEGO_EDITION_HARMATTAN
    QString logFileLocation = (QDesktopServices::storageLocation(QDesktopServices::DataLocation) % "/" % appExecName);
#else
    QString logFileLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif

    m_logFile.setFileName(logFileLocation % "/Log/log.txt");
    if (m_logFile.open(QFile::ReadOnly)) {
        m_loggedText = m_logFile.readAll();
        m_logFile.close();
    } else {
        QDir(logFileLocation).mkpath("Log");
    }
    m_logFile.open(QFile::Append);
    m_mutex.unlock();
}

void Logger::addTextToLog(const QString &text)
{
    m_logFile.write(text.toUtf8());
    m_logFile.flush();
    m_loggedText.append(text);
    emit textAddedToLog(text);
}

static QString versionToString(qint32 version)
{
    if ((version & 0xff) == 0) {
        return QString("%1.%2").arg(version >> 16 & 0xff).arg(version >> 8 & 0xff);
    }
    return QString("%1.%2.%3").arg(version >> 16 & 0xff).arg(version >> 8 & 0xff).arg(version & 0xff);
}

void Logger::log(const QString &msg)
{
    m_mutex.lock();
    QDateTime now = QDateTime::currentDateTime();
    if (m_lastMessageDate.daysTo(now) > 0) {
        QtMobility::QSystemInfo sysInfo;
        QString infoMsg = QString("%1\nNotekeeper%2 %3 Qt %4 %5 %6 Firmware %7\n")
                .arg(now.toString("dd MMM yyyy"))
                .arg("Open")
                .arg(versionToString(APP_VERSION))
                .arg(qVersion())
        #if defined(Q_OS_SYMBIAN)
                .arg("Symbian")
        #elif defined(MEEGO_EDITION_HARMATTAN)
                .arg("Meego")
        #elif defined(QT_SIMULATOR)
                .arg("Simulator")
        #else
                .arg("Unknown_OS")
        #endif
                .arg(sysInfo.version(QtMobility::QSystemInfo::Os))
                .arg(sysInfo.version(QtMobility::QSystemInfo::Firmware));
        addTextToLog(infoMsg);
    }
    addTextToLog(now.toString("[hh:mm:ss] ") % msg % "\n");
    m_lastMessageDate = now;
    m_mutex.unlock();
}

QString Logger::loggedText()
{
    m_mutex.lock();
    QString text = m_loggedText;
    m_mutex.unlock();
    return text;
}

void Logger::clear()
{
    m_mutex.lock();
    m_logFile.remove();
    m_loggedText.clear();
    m_lastMessageDate = QDateTime();
    m_mutex.unlock();
    emit cleared();
}

void Logger::end()
{
    m_logFile.close();
}
