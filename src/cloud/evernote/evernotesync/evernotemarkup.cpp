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

#include <QString>
#include <QVariant>
#include <QXmlStreamReader>
#include <QStringBuilder>
#include <QRegExp>
#include <QTextCodec>
#include <QScopedPointer>
#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QSize>
#include <QVariantList>
#include <QVariantMap>
#include <QTextDocument>
#include <QtDebug>
#include <QRegExp>
#include <QDir>
#include <QLocale>
#include "math.h"

#include "evernotemarkup.h"
#include "richtextnotecss.h"
#include "html_named_entities.h"

#define TRY_ALTERNATE_ENCODINGS

#define ENML_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
                    "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">" \
                    "<en-note>"
#define ENML_FOOTER "</en-note>"

class HtmlEntityResolver : public QXmlStreamEntityResolver {
    virtual QString resolveUndeclaredEntity(const QString &name) {
        return resolveHtmlNamedEntity(name);
    }
};

static QByteArray escapedUtf8Html(const QString &s)
{
    QByteArray result, escaped;
    int lastEscapedPos = -1;
    for (int i = 0; i < s.size(); ++i) {
        QChar c = s.at(i);
        bool isEscaped = false;
        if (c.unicode() == '<') {
            escaped = QByteArray("&lt;");
            isEscaped = true;
        } else if (c.unicode() == '>') {
            escaped = QByteArray("&gt;");
            isEscaped = true;
        } else if (c.unicode() == '&') {
            escaped = QByteArray("&amp;");
            isEscaped = true;
        } else if (c.unicode() == '\"') {
            escaped = QByteArray("&quot;");
            isEscaped = true;
        } else if (c.unicode() == '\n') {
            escaped = QByteArray("<br/>");
            isEscaped = true;
        } else if (c.unicode() == ' ') {
            escaped = QByteArray(" ");
            isEscaped = true;
        }
        if (isEscaped) {
            if (i > (lastEscapedPos + 1)) {
                QByteArray fragment = s.mid(lastEscapedPos + 1, i - (lastEscapedPos + 1)).toUtf8();
                result.append(fragment);
            }
            if (escaped == " ") {
                if (result.isEmpty() || result.at(result.length() - 1) == ' ' || i == (s.size() - 1)) {
                    escaped = "&nbsp;";
                }
            }
            result.append(escaped);
            lastEscapedPos = i;
        }
    }
    if (s.size() > (lastEscapedPos + 1)) {
        result.append(s.mid(lastEscapedPos + 1, s.size() - (lastEscapedPos + 1)).toUtf8());
    }
    return result;
}

static QByteArray constructEnml(const QString &plainText, const QVariantList &attachmentsData, int plainTextStartingIndex = 0)
{
    QByteArray contentEnml = QByteArray(ENML_HEADER);
    contentEnml.append(escapedUtf8Html(plainText.mid(plainTextStartingIndex)));
    foreach (const QVariant &attachment, attachmentsData) {
        QVariantMap map = attachment.toMap();
        QByteArray hash = map.value("Hash").toString().toLatin1();
        QByteArray type = map.value("MimeType").toString().toLatin1();
        QByteArray enMedia = QByteArray("<en-media hash=\"") + hash + QByteArray("\" type=\"") + type + QByteArray("\"/>");
        QString trailingText = map.value("TrailingText").toString();
        QByteArray afterEnMedia = escapedUtf8Html(trailingText);
        contentEnml.append(enMedia);
        contentEnml.append(afterEnMedia);
    }

    contentEnml.append(QByteArray(ENML_FOOTER));
    return contentEnml;
}

void EvernoteMarkup::enmlFromPlainText(const QString &titleAndContent, const QVariantList &attachmentsData, QString *title, QByteArray *enmlContent)
{
    int firstNL = titleAndContent.indexOf('\n');
    if (firstNL < 0) { // if there's no newline in the text
        (*title) = titleAndContent; // the whole thing is the title
        (*enmlContent) = QByteArray(ENML_HEADER) + QByteArray(ENML_FOOTER); // and the content is empty
        return;
    }
    (*title) = titleAndContent.left(firstNL);
    (*enmlContent) = constructEnml(titleAndContent, attachmentsData, firstNL + 1);
}

static QByteArray coreEnml(const QByteArray &enmlData, const QVariantList &attachmentsData)
{
    // Used to get the ENML excluding the <en-note> part
    // Used in resolving conflicts between two versions of a note
    QSet<QByteArray> availableAttachments;
    foreach (const QVariant &v, attachmentsData) {
        QByteArray md5Hash = v.toMap().value("Hash").toByteArray();
        availableAttachments << md5Hash;
    }

    QXmlStreamReader xml(enmlData);
    HtmlEntityResolver entityResolver;
    xml.setEntityResolver(&entityResolver);
    QByteArray enmlOut;
    QXmlStreamWriter enmlWriter(&enmlOut);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (QLatin1String("en-media") == xml.name()) { // If an attachment
                // We should include only those attachments that are found in attachments.ini
                const QXmlStreamAttributes attributes = xml.attributes();
                if (attributes.hasAttribute("hash") && availableAttachments.contains(attributes.value("hash").toString().toLatin1())) {
                    enmlWriter.writeStartElement(xml.name().toString());
                    enmlWriter.writeAttributes(xml.attributes());
                }
            } else if (QLatin1String("en-note") != xml.name()) {
                enmlWriter.writeStartElement(xml.name().toString());
                enmlWriter.writeAttributes(xml.attributes());
            }
        } else if (xml.tokenType() == QXmlStreamReader::Characters) {
            enmlWriter.writeCharacters(xml.text().toString());
        } else if (xml.tokenType() == QXmlStreamReader::EndElement) {
            if (QLatin1String("en-note") != xml.name()) {
                enmlWriter.writeEndElement();
            }
        } else if (xml.tokenType() == QXmlStreamReader::EntityReference) {
            qDebug() << "Unknown entity reference" << xml.name();
        }
    }
    enmlWriter.writeEndDocument();
    return enmlOut;
}

static QString humanizedNumber(qint64 number, const QString &suffix)
{
    return QString("%1 %2").arg(number).arg(suffix);
}

static QString humanizedNumber(double number, int maxDecimalPoints, const QString &suffix)
{
    double integerPart;
    Q_UNUSED(integerPart);
    if (modf(number, &integerPart) == 0.0) {
        maxDecimalPoints = 0;
    }
    return QString("%1 %2").arg(QLocale::system().toString(number, 'f', maxDecimalPoints)).arg(suffix);
}

static QString humanizedFileSize(qint64 bytes)
{
    if (bytes >= 1073741824) {
        return humanizedNumber(bytes / 1073741824.0, 2, "GB");
    } else {
        if (bytes >= 1048576) {
            return humanizedNumber(bytes / 1048576.0, 2, "MB");
        } else {
            if (bytes >= 1024) {
                return humanizedNumber(bytes / 1024, "KB");
            } else {
                return humanizedNumber(bytes, "bytes");
            };
        };
    };
    return humanizedNumber(bytes, "bytes");
}

static bool parseEnml(const QByteArray &enmlData, bool *isPlainTextContent, QString *content, bool shouldReturnHtml, const QVariantList &attachmentsData, const QString &attachmentUrlBase, const QString &qmlResourceBase, QVariantList *allAttachments, QVariantList *imageAttachments, QVariantList *checkboxStates, QString *version, QString *encoding, QVariantMap *errorInfo)
{
    Q_ASSERT((imageAttachments == 0) == (allAttachments == 0));
    QVariantMap attachmentHashMap;
    foreach (const QVariant &v, attachmentsData) {
        const QVariantMap map = v.toMap();
        attachmentHashMap[QLatin1String(map.value("Hash").toByteArray().constData())] = map;
    }

    if (allAttachments) {
        allAttachments->clear();
    }
    if (imageAttachments) {
        imageAttachments->clear();
    }
    if (checkboxStates) {
        checkboxStates->clear();
    }

    QString htmlString, plainTextHtmlString;
    bool isPlainTextNote = true;
    bool isEnMediaEncountered = false;
    int checkboxCount = 0;
    HtmlEntityResolver entityResolver;
    QXmlStreamReader xml(enmlData);
    QXmlStreamWriter htmlWriter(&htmlString);
    QXmlStreamWriter plaintextHtmlWriter(&plainTextHtmlString); // we write html and then convert to plaintext
    xml.setEntityResolver(&entityResolver);
    QStringRef currentElementName;
    QVariantMap currentAttachmentData;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartDocument) {
            (*version) = xml.documentVersion().toString();
            (*encoding) = xml.documentEncoding().toString();
        } else if (xml.tokenType() == QXmlStreamReader::DTD) {
            if (xml.dtdName() != "en-note") {
                xml.raiseError("Not an evernote note");
            }
        } else if (xml.tokenType() == QXmlStreamReader::StartElement) {
            QStringRef tokenNameStrRef = xml.name();
            currentElementName = tokenNameStrRef;
            if (isPlainTextNote) {
                if (QLatin1String("div") == tokenNameStrRef ||
                    QLatin1String("p") == tokenNameStrRef ||
                    QLatin1String("br") == tokenNameStrRef) {
                    if (isEnMediaEncountered) { // add the text to the attachment's data
                        QVariant &trailingText = currentAttachmentData["TrailingText"];
                        trailingText = trailingText.toString() + "\n";
                    }
                } else if (QLatin1String("span") == tokenNameStrRef ||
                           QLatin1String("en-note") == tokenNameStrRef) {
                    // valid. ignore maadi.
                } else if (QLatin1String("en-media") == tokenNameStrRef) {
                    isEnMediaEncountered = true;
                } else {
                    isPlainTextNote = false;
                }
            }
            if (QLatin1String("en-note") == tokenNameStrRef) {
                // ignore
            } else if (QLatin1String("en-crypt") == tokenNameStrRef) {
                if (shouldReturnHtml) {
                    htmlWriter.writeStartElement("span");
                    htmlWriter.writeAttribute("style", "color: gray;");
                }
            } else if (QLatin1String("en-todo") == tokenNameStrRef) {
                QXmlStreamAttributes attributes = xml.attributes();
                bool checked = (attributes.hasAttribute("checked") &&
                                attributes.value("checked").compare("true", Qt::CaseInsensitive) == 0);
                if (shouldReturnHtml) {
                    htmlWriter.writeStartElement("input");
                    htmlWriter.writeAttribute("type", "checkbox");
                    if (checked) {
                        htmlWriter.writeAttribute("checked", "checked");
                    }
                    htmlWriter.writeAttribute("class", "en_todo_checkbox");
                    htmlWriter.writeAttribute("onclick", "window.qml.checkboxClicked(" + QString::number(checkboxCount) + ");");
                }
                if (checkboxStates) {
                    checkboxStates->append(checked);
                }
                checkboxCount++;
            } else if (QLatin1String("en-media") == tokenNameStrRef) {
                if (allAttachments) {
                    if (!currentAttachmentData.value("Hash").toByteArray().isEmpty()) {
                        allAttachments->append(currentAttachmentData); // add previous attachment data to return data structure
                        currentAttachmentData.clear();
                    }
                }
                const QXmlStreamAttributes attributes = xml.attributes();
                if (attributes.hasAttribute("type") && attributes.hasAttribute("hash")) { // both are required attributes in ENML
                    QString mimeType = attributes.value("type").toString();
                    QString md5Sum = attributes.value("hash").toString();
                    QVariantMap attachmentDataMap = attachmentHashMap.value(md5Sum).toMap();
                    QString attachmentGuid = attachmentDataMap.value("guid").toString();
                    QString attachmentFilePath = attachmentDataMap.value("FilePath").toString();
                    QString attachmentUrl;
                    if (!attachmentFilePath.isEmpty()) {
                        attachmentUrl = QString("file:///" % attachmentFilePath);
                    } else {
                        attachmentUrl = attachmentUrlBase % "/" % attachmentGuid;
                    }
                    QString attachmentFileName = attachmentDataMap.value("FileName", QString("")).toString();
                    int attachmentFileSize = attachmentDataMap.value("Size").toInt();
                    QSize imageSize = attachmentDataMap.value("Dimensions").toSize();

                    // for images specifically
                    if (mimeType.startsWith("image/")) {
                        if (shouldReturnHtml && !md5Sum.isEmpty()) {
                            QSize htmlImageSize = imageSize;
                            if (htmlImageSize.width() > 360 || htmlImageSize.height() > 640) {
                                htmlImageSize.scale(360, 640, Qt::KeepAspectRatio);
                            }
                            htmlWriter.writeStartElement("img");
                            htmlWriter.writeAttribute("src", attachmentUrl);
                            htmlWriter.writeAttribute("width", QString::number(htmlImageSize.width()));
                            htmlWriter.writeAttribute("height", QString::number(htmlImageSize.height()));
                            if (imageAttachments) {
                                htmlWriter.writeAttribute("onclick", "window.qml.imageClicked(" + QString::number(imageAttachments->count()) + ");");
                            }
                            foreach (const QXmlStreamAttribute &attribute, attributes) {
                                if ("hash"   == attribute.qualifiedName() ||
                                        "type"   == attribute.qualifiedName() ||
                                        "src"    == attribute.qualifiedName() ||
                                        "width"  == attribute.qualifiedName() ||
                                        "height" == attribute.qualifiedName()) {
                                    continue;
                                }
                                htmlWriter.writeAttribute(attribute);
                            }
                        }
                        // for showing images at the end of a plaintext note
                        if (imageAttachments && !md5Sum.isEmpty()) {
                            QVariantMap imageDataToReturn;
                            imageDataToReturn["guid"] = attachmentGuid;
                            imageDataToReturn["Hash"] = md5Sum;
                            imageDataToReturn["Url"] = attachmentUrl;
                            imageDataToReturn["Width"] = imageSize.width();
                            imageDataToReturn["Height"] = imageSize.height();
                            imageDataToReturn["FileName"] = attachmentFileName;
                            imageDataToReturn["Size"] = attachmentFileSize;
                            imageDataToReturn["MimeType"] = mimeType;
                            imageDataToReturn["AttachmentIndex"] = allAttachments->count();
                            imageAttachments->append(imageDataToReturn);
                        }
                    } else {
                        if (shouldReturnHtml && !md5Sum.isEmpty()) {
                            QString imageUrl;
                            if (mimeType == "application/pdf") {
                                imageUrl = qmlResourceBase + "/notekeeper/images/filetypes/pdf.png";
                            } else if (mimeType == "audio/wav" || mimeType == "audio/mpeg") {
                                imageUrl = qmlResourceBase + "/notekeeper/images/filetypes/audio.png";
                            } else {
                                imageUrl = qmlResourceBase + "/notekeeper/images/filetypes/attachment.png";
                            }
                            if (imageUrl.startsWith(':')) {
                                imageUrl.replace(0, 1, "qrc://");
                            } else {
                                imageUrl.prepend("file://");
                            }
                            htmlWriter.writeStartElement("span");
                            htmlWriter.writeAttribute("style", "display: inline-block;");
                            htmlWriter.writeStartElement("div");
                            htmlWriter.writeAttribute("class", "attachment-box");
                            htmlWriter.writeAttribute("onclick", "window.qml.attachmentClicked(" + QString::number(allAttachments->count()) + ");");
                            htmlWriter.writeStartElement("div");
                            htmlWriter.writeAttribute("style", "margin: 10px 10px 10px 10px; background: url(" + imageUrl + ") no-repeat left center;");
                            htmlWriter.writeStartElement("div");
                            htmlWriter.writeAttribute("style", "margin-left: 40px;");
                            QString name = attachmentFileName;
                            if (name.isEmpty()) {
                                name = "Unnamed attachment";
                            }
                            htmlWriter.writeCharacters(name);
                            htmlWriter.writeStartElement("br");
                            htmlWriter.writeEndElement(); // </br>
                            htmlWriter.writeStartElement("small");
                            htmlWriter.writeCharacters(humanizedFileSize(attachmentFileSize));
                            htmlWriter.writeEndElement(); // </small>
                            htmlWriter.writeEndElement(); // </div>
                            htmlWriter.writeEndElement(); // </div>
                            htmlWriter.writeEndElement(); // </div>
                            // <span> will be closed when we see QXmlStreamReader::EndElement
                        }
                    }

                    // for all attachments (including images)
                    if (allAttachments) {
                        if (!md5Sum.isEmpty()) {
                            currentAttachmentData.clear();
                            currentAttachmentData["guid"] = attachmentGuid;
                            currentAttachmentData["Hash"] = md5Sum;
                            currentAttachmentData["Url"] = attachmentUrl;
                            currentAttachmentData["MimeType"] = mimeType;
                            currentAttachmentData["TrailingText"] = QString("");
                            currentAttachmentData["FileName"] = attachmentFileName;
                            currentAttachmentData["Size"] = attachmentFileSize;
                            currentAttachmentData["Width"] = imageSize.width();
                            currentAttachmentData["Height"] = imageSize.height();
                            currentAttachmentData["Duration"] = attachmentDataMap.value("Duration").toInt();
                        }
                    }
                }
            } else {
                if (shouldReturnHtml) {
                    htmlWriter.writeStartElement(tokenNameStrRef.toString());
                    htmlWriter.writeAttributes(xml.attributes());
                }
                if (isPlainTextNote && !isEnMediaEncountered) {
                    plaintextHtmlWriter.writeStartElement(tokenNameStrRef.toString());
                    plaintextHtmlWriter.writeAttributes(xml.attributes());
                }
            }
        } else if (xml.tokenType() == QXmlStreamReader::Characters) {
            QString text = xml.text().toString();
            if (isPlainTextNote) {
                QString trimmedText = text.trimmed();
                if (isEnMediaEncountered && !trimmedText.isEmpty()) {
                    // text after image/attachment => not plain text
                    isPlainTextNote = false;
                }
            }
            if (currentElementName == "en-crypt") {
                if (shouldReturnHtml) {
                    htmlWriter.writeCharacters("Encrypted text");
                }
            } else {
                if (shouldReturnHtml) {
                    htmlWriter.writeCharacters(text);
                }
                if (isPlainTextNote && !isEnMediaEncountered) {
                    plaintextHtmlWriter.writeCharacters(text);
                }
            }
            if (isEnMediaEncountered) {
                QVariant &trailingText = currentAttachmentData["TrailingText"];
                trailingText = trailingText.toString() + text;
            }
        } else if (xml.tokenType() == QXmlStreamReader::EndElement) {
            if (shouldReturnHtml) {
                htmlWriter.writeEndElement();
            }
            if (isPlainTextNote) {
                plaintextHtmlWriter.writeEndElement();
            }
            currentElementName = QStringRef();
            if (xml.name() == "en-note") {
                if (allAttachments) {
                    if (!currentAttachmentData.value("Hash").toString().isEmpty()) {
                        allAttachments->append(currentAttachmentData); // add previous attachment data to return data structure
                        currentAttachmentData.clear();
                    }
                }
            }
        } else if (xml.tokenType() == QXmlStreamReader::EntityReference) {
            qDebug() << "Unknown entity reference" << xml.name();
        }
    }

    if (shouldReturnHtml) {
        htmlWriter.writeEndDocument();
    }
    if (isPlainTextNote) {
        plaintextHtmlWriter.writeEndDocument();
    }

    (*isPlainTextContent) = isPlainTextNote;
    if (isPlainTextNote) {
        QTextDocument doc;
        if (plainTextHtmlString.endsWith(QChar('\n'))) { // remove trailing newline to prevent trailing space in plainTextString
            plainTextHtmlString.truncate(plainTextHtmlString.length() - 1);
        }
        doc.setHtml(plainTextHtmlString);
        QString plainTextString = doc.toPlainText();
        (*content) = plainTextString;
    } else {
        if (shouldReturnHtml) {
            htmlString.replace(QRegExp("href=\"([^\"]+)\""), "href=\"null\" onclick=\"window.qml.openUrlExternally('\\1')\"");
            if (checkboxCount > 0) {
                htmlString.prepend(QString::fromLatin1(RICH_TEXT_NOTE_CSS));
            }
            (*content) = htmlString;
        } else {
            (*content) = QString();
        }
    }

    if (xml.error() != QXmlStreamReader::NoError) {
        QVariantMap error;
        error["errorString"] = xml.errorString();
        error["errorCode"] = xml.error();
        error["lineNumber"] = xml.lineNumber();
        error["columnNumber"] = xml.columnNumber();
        error["characterOffet"] = xml.characterOffset();
        (*errorInfo) = error;
        return false;
    }

    return true;
}

static bool changeXmlHeader(const QByteArray &enmlData, const QString &version, const QString &encoding, QByteArray *enmlDataWithModifiedXmlHeader)
{
    // Replacing the <?xml version='1.0' encoding='utf-8'?> at the start of the ENML
    if (!enmlData.startsWith("<?")) {
        return false;
    }
    int endOfHeader = enmlData.indexOf("?>");
    if (endOfHeader < 0) {
        return false;
    }
    QString newXmlHeader = QString("<?xml version='%1' encoding='%2'?>").arg(version).arg(encoding);
    (*enmlDataWithModifiedXmlHeader) = (newXmlHeader.toUtf8() + enmlData.mid(endOfHeader + 2));
    return true;
}

static QByteArray convertXmlToUtf8(const QByteArray &xml, QVariantMap *errorInfo = 0)
{
    QByteArray copy;
    QXmlStreamReader xmlReader(xml);
    QXmlStreamWriter xmlWriter(&copy);
    while (!xmlReader.atEnd()) {
        xmlReader.readNext();
        if (xmlReader.tokenType() == QXmlStreamReader::StartDocument) {
            // even if it's already in utf-8, don't just return the original, let's verify if it's correct
            xmlWriter.writeStartDocument();
        } else if (xmlReader.tokenType() == QXmlStreamReader::DTD) {
            QString dtd = QString("<!DOCTYPE %1 SYSTEM \"%2\">").arg(xmlReader.dtdName().toString()).arg(xmlReader.dtdSystemId().toString());
            xmlWriter.writeDTD(dtd);
        } else if (xmlReader.tokenType() == QXmlStreamReader::StartElement) {
            xmlWriter.writeStartElement(xmlReader.name().toString());
            xmlWriter.writeAttributes(xmlReader.attributes());
        } else if (xmlReader.tokenType() == QXmlStreamReader::Characters) {
            xmlWriter.writeCharacters(xmlReader.text().toString());
        } else if (xmlReader.tokenType() == QXmlStreamReader::EndElement) {
            xmlWriter.writeEndElement();
        } else if (xmlReader.tokenType() == QXmlStreamReader::EntityReference) {
            xmlReader.raiseError("Unresolved entity reference: " + xmlReader.name().toString());
        }
    }
    if (xmlReader.error() != QXmlStreamReader::NoError) {
        if (errorInfo) {
            QVariantMap error;
            error["errorString"] = xmlReader.errorString();
            error["errorCode"] = xmlReader.error();
            error["lineNumber"] = xmlReader.lineNumber();
            error["columnNumber"] = xmlReader.columnNumber();
            error["characterOffet"] = xmlReader.characterOffset();
            (*errorInfo) = error;
        }
        return QByteArray();
    }

    if (errorInfo) {
        (*errorInfo) = QVariantMap();
    }

    xmlWriter.writeEndDocument();
    return copy;
}

bool EvernoteMarkup::parseEvernoteMarkup(const QByteArray &enmlData, bool *isPlainTextContent, QString *content, bool shouldReturnHtml, const QVariantList &attachmentsData, const QString &attachmentUrlBase, const QString &qmlResourceBase, QVariantList *allAttachments, QVariantList *imageAttachments, QVariantList *checkboxStates, QByteArray *convertedEnmlData)
{
    QVariantMap errorInfo;
    QString encodingAsInTheOriginalXml, versionAsInTheOriginalXml;
    if (parseEnml(enmlData, isPlainTextContent, content, shouldReturnHtml, attachmentsData, attachmentUrlBase, qmlResourceBase, allAttachments, imageAttachments, checkboxStates, &versionAsInTheOriginalXml, &encodingAsInTheOriginalXml, &errorInfo)) {
        if (convertedEnmlData) {
            if (encodingAsInTheOriginalXml.toLower() == "utf-8") {
                (*convertedEnmlData) = QByteArray();
            } else {
                (*convertedEnmlData) = convertXmlToUtf8(enmlData);
            }
        }
        return true;
    }
    QVariantMap originalErrorInfo = errorInfo;
    Q_ASSERT(errorInfo.value("errorCode").toInt() != QXmlStreamReader::NoError);

#ifdef TRY_ALTERNATE_ENCODINGS
    bool encodingIsIncorrect = false;
    if ((errorInfo.value("errorCode").toInt() == QXmlStreamReader::NotWellFormedError &&
         errorInfo.value("errorString").toString().indexOf("incorrectly encoded content", 0, Qt::CaseInsensitive) >= 0)) {
        encodingIsIncorrect = true;
    }

    QStringList alternateEncodings;
    alternateEncodings << "utf-8" << "iso-8859-1" << "iso-8859-2" << "iso-8859-3" << "iso-8859-4" << "utf-16";
    alternateEncodings.removeOne(encodingAsInTheOriginalXml.toLower());

    QByteArray convertedEnml;
    QString alternateEncoding;
    while (encodingIsIncorrect) {
        // The parser errored out because some characters in the document
        // used a different encoding from what's said in the xml header.
        // So let's change the encoding part of the xml header and try again.
        if (alternateEncodings.isEmpty()) {
            break;
        }
        alternateEncoding = alternateEncodings.takeFirst();
        QByteArray enmlDataWithModifiedXmlHeader;
        if (changeXmlHeader(enmlData, versionAsInTheOriginalXml, alternateEncoding, &enmlDataWithModifiedXmlHeader)) {
            // if we're able to modify the xml header correctly,
            // verify correctness of the modified xml and convert it to UTF-8 encoding
            convertedEnml = convertXmlToUtf8(enmlDataWithModifiedXmlHeader, &errorInfo);
        }
        encodingIsIncorrect = (errorInfo.value("errorCode").toInt() != QXmlStreamReader::NoError);
    }

    if (!convertedEnml.isEmpty()) {
        // We found an alternate encoding that parsed correctly
        qDebug() << "Assuming alternate encoding [" + alternateEncoding + "]";
        QString encodingTmp, versionTmp;
        if (parseEnml(convertedEnml, isPlainTextContent, content, shouldReturnHtml, attachmentsData, attachmentUrlBase, qmlResourceBase, allAttachments, imageAttachments, checkboxStates, &versionTmp, &encodingTmp, &errorInfo)) {
            if (convertedEnmlData) {
                (*convertedEnmlData) = convertedEnml;
            }
            return true;
        }
    }
#endif

    qDebug() << QString("Error parsing ENML: %1 (Code: %2 Line: %3 Column: %4 Char Offset: %5)")
                .arg(originalErrorInfo.value("errorString").toString())
                .arg(originalErrorInfo.value("errorCode").toInt())
                .arg(originalErrorInfo.value("lineNumber").toInt())
                .arg(originalErrorInfo.value("columnNumber").toInt())
                .arg(originalErrorInfo.value("characterOffet").toInt());
    return false;
}

QByteArray EvernoteMarkup::resolveConflict(const QByteArray &newContentEnml, const QByteArray &oldContentEnml, qint64 oldTimestampUtc, const QVariantList &attachmentsData, bool *ok)
{
    QString newContentText, oldContentText;
    bool newContentIsPlainText, oldContentIsPlainText;
    bool newContentParsedSuccessfully = EvernoteMarkup::parseEvernoteMarkup(newContentEnml, &newContentIsPlainText, &newContentText);
    bool oldContentParsedSuccessfully = EvernoteMarkup::parseEvernoteMarkup(oldContentEnml, &oldContentIsPlainText, &oldContentText);

    if (ok) {
        (*ok) = (oldContentParsedSuccessfully && newContentParsedSuccessfully);
    }

    if (!newContentParsedSuccessfully && oldContentParsedSuccessfully) {
        return oldContentEnml;
    }
    if (!oldContentParsedSuccessfully && newContentParsedSuccessfully) {
        return newContentEnml;
    }
    if (!oldContentParsedSuccessfully && !newContentParsedSuccessfully) {
        return QByteArray();
    }

    Q_ASSERT(oldContentParsedSuccessfully);
    Q_ASSERT(newContentParsedSuccessfully);

    QString joiningText;
    if (oldTimestampUtc > 0) {
        joiningText = QString::fromLatin1("\n\n\nConflicts with note edited on %1:\n\n")
                .arg(QDateTime::fromTime_t(oldTimestampUtc / 1000).toString("ddd MMM d, yyyy, h:mm ap"));
    } else {
        joiningText = QString::fromLatin1("\n\n\nConflicts with note edited elsewhere:\n\n");
    }

    if (newContentIsPlainText && oldContentIsPlainText) {
        if ((newContentText.size() > oldContentText.size()) && (newContentText.indexOf(oldContentText) > 0)) {
            // oldContentText is a substring of newContentText
            return newContentEnml;
        }
        if ((oldContentText.size() > newContentText.size()) && (oldContentText.indexOf(newContentText) > 0)) {
            // newContentText is a substring of oldContentText
            return oldContentEnml;
        }
        return constructEnml(newContentText % joiningText % oldContentText, attachmentsData);
    } else {
        joiningText = "<div>" % joiningText.split('\n').join("</div><div>") % "</div>";
        return (QByteArray(ENML_HEADER) + coreEnml(newContentEnml, attachmentsData) + joiningText.toUtf8() + coreEnml(oldContentEnml, attachmentsData) + QByteArray(ENML_FOOTER));
    }
    return QByteArray(); // control should never reach here
}

QString EvernoteMarkup::plainTextFromEnml(const QByteArray &enmlData, int maxLength)
{
    HtmlEntityResolver entityResolver;
    QXmlStreamReader xml(enmlData);
    xml.setEntityResolver(&entityResolver);
    QString plainText;
    int length = 0;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::DTD) {
            if (xml.dtdName() != "en-note") {
                return QString();
            }
        } else if (xml.tokenType() == QXmlStreamReader::Characters) {
            QString textlet = xml.text().toString().trimmed();
            if (!textlet.isEmpty()) {
                if (plainText.isEmpty()) {
                    plainText = textlet;
                    length = textlet.length();
                } else {
                    plainText = (plainText % " " % textlet);
                    length += textlet.length() + 1;
                }
                if (maxLength >= 0 && length > maxLength) {
                    break;
                }
            }
        }
    }
    return plainText;
}

static QStringList words(const QString &text)
{
    QStringList wordList;
    QString word;
    for (int i = 0; i < text.length(); i++) {
        const QChar c = text.at(i);
        bool isSeparatorChar = (c.isSpace() || c.isPunct() || c.isSymbol());
        if (isSeparatorChar) {
            if (!word.isEmpty()) {
                wordList << word;
            }
            word.clear();
        } else {
            word.append(c);
        }
    }
    if (!word.isEmpty()) {
        wordList << word;
    }
    return wordList;
}

QStringList EvernoteMarkup::plainTextWordsFromEnml(const QString &title, const QByteArray &enmlData)
{
    HtmlEntityResolver entityResolver;
    QXmlStreamReader xml(enmlData);
    xml.setEntityResolver(&entityResolver);
    QStringList wordList;
    wordList << words(title);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::DTD) {
            if (xml.dtdName() != "en-note") {
                return QStringList();
            }
        } else if (xml.tokenType() == QXmlStreamReader::Characters) {
            QString textlet = xml.text().toString();
            wordList << words(textlet);
        }
    }
    return wordList;
}

bool EvernoteMarkup::updateCheckboxStatesInEnml(const QByteArray &enmlData, const QVariantList &checkboxStates, QByteArray *_updatedEnml)
{
    QByteArray updatedEnml;
    HtmlEntityResolver entityResolver;
    QXmlStreamReader xml(enmlData);
    QXmlStreamWriter enmlWriter(&updatedEnml);
    xml.setEntityResolver(&entityResolver);
    int checkboxIndex = 0;
    bool checkboxStatesListUnderflow = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartDocument) {
            enmlWriter.writeStartDocument(xml.documentVersion().toString());
        } else if (xml.tokenType() == QXmlStreamReader::DTD) {
            enmlWriter.writeDTD(xml.text().toString());
            if (xml.dtdName() != "en-note") {
                return false;
            }
        } else if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (QLatin1String("en-todo") == xml.name()) {
                enmlWriter.writeStartElement("en-todo");
                if (checkboxIndex < checkboxStates.count()) {
                    if (checkboxStates.at(checkboxIndex).toBool()) {
                        enmlWriter.writeAttribute("checked", "true");
                    }
                } else {
                    checkboxStatesListUnderflow = true;
                }
                checkboxIndex++;
            } else {
                enmlWriter.writeStartElement(xml.name().toString());
                enmlWriter.writeAttributes(xml.attributes());
            }
        } else if (xml.tokenType() == QXmlStreamReader::Characters) {
            enmlWriter.writeCharacters(xml.text().toString());
        } else if (xml.tokenType() == QXmlStreamReader::EndElement) {
            enmlWriter.writeEndElement();
        } else if (xml.tokenType() == QXmlStreamReader::EntityReference) {
            qDebug() << "Unknown entity reference" << xml.name();
        }
    }
    enmlWriter.writeEndDocument();
    if (xml.error() != QXmlStreamReader::NoError) {
        qDebug() << "Error parsing ENML:" << xml.errorString();
        return false;
    }
    if (_updatedEnml) {
        (*_updatedEnml) = updatedEnml;
    }
    if (checkboxStatesListUnderflow) {
        return false;
    }
    return true;
}

bool EvernoteMarkup::appendTextToEnml(const QByteArray &_enmlContent, const QString &text, QByteArray *updatedEnml, QByteArray *appendedHtml)
{
    if (text.isEmpty()) {
        return false;
    }
    QByteArray htmlToAppend("");
    QStringList lines = text.split('\n');
    foreach (const QString &line, lines) {
        htmlToAppend.append("<div>" + escapedUtf8Html(line) + "</div>");
        // FIXME: Pick encoding from the current enml content
    }
    int enNoteClosingTagPos = _enmlContent.indexOf("</en-note>");
    if (enNoteClosingTagPos < 0) {
        return false;
    }
    QByteArray enmlContent = _enmlContent;
    enmlContent.insert(enNoteClosingTagPos, htmlToAppend);
    if (updatedEnml) {
        (*updatedEnml) = enmlContent;
    }
    if (appendedHtml) {
        (*appendedHtml) = htmlToAppend;
    }
    return true;
}

bool EvernoteMarkup::appendAttachmentToEnml(const QByteArray &_enmlContent, const QString &fileUrl, const QVariantMap &properties, QByteArray *updatedEnml, QByteArray *appendedHtml)
{
    QString mimeType = properties.value("MimeType").toString();
    QByteArray hash = properties.value("Hash").toByteArray();

    if (fileUrl.isEmpty() || mimeType.isEmpty() || hash.isEmpty()) {
        return false;
    }

    // Get the index of the new image
    int imageCount = 0;
    QXmlStreamReader xml(_enmlContent);
    HtmlEntityResolver entityResolver;
    xml.setEntityResolver(&entityResolver);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (QLatin1String("en-media") == xml.name()) {
                const QXmlStreamAttributes attributes = xml.attributes();
                if (attributes.hasAttribute("type") && attributes.hasAttribute("hash")) {
                    QString mimeType = attributes.value("type").toString();
                    QString imageMd5Sum = attributes.value("hash").toString();
                    if (mimeType.startsWith("image/") && !imageMd5Sum.isEmpty()) {
                        imageCount++;
                    }
                }
            }
        }
    }

    // Append to enml and html
    QByteArray enmlToAppend("");
    enmlToAppend.append("<en-media");
    enmlToAppend.append(" type=\"" + mimeType.toUtf8() + "\"");
    enmlToAppend.append(" hash=\"" + hash + "\"");
    enmlToAppend.append(" />");

    QByteArray htmlToAppend("");
    htmlToAppend.append("<img");
    htmlToAppend.append(" src=\"" + fileUrl.toUtf8() + "\"");
    if (properties.contains("Dimensions")) {
        QSize imageSize = properties.value("Dimensions").toSize();
        QSize htmlImageSize = imageSize;
        if (htmlImageSize.width() > 360 || htmlImageSize.height() > 640) {
            htmlImageSize.scale(360, 640, Qt::KeepAspectRatio);
        }
        htmlToAppend.append(" width=\"" + QByteArray::number(htmlImageSize.width()) + "\"");
        htmlToAppend.append(" height=\"" + QByteArray::number(htmlImageSize.height()) + "\"");
    }
    htmlToAppend.append(" onclick=\"window.qml.imageClicked(" + QByteArray::number(imageCount) + ");\"");
    htmlToAppend.append(" />");

    int enNoteClosingTagPos = _enmlContent.indexOf("</en-note>");
    if (enNoteClosingTagPos < 0) {
        return false;
    }
    QByteArray enmlContent = _enmlContent;
    enmlContent.insert(enNoteClosingTagPos, enmlToAppend);
    if (updatedEnml) {
        (*updatedEnml) = enmlContent;
    }
    if (appendedHtml) {
        (*appendedHtml) = htmlToAppend;
    }
    return true;
}
