#include "CSVReader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

CSVReader::CSVReader()
    : m_separator(','),
      m_startLine(1),
      m_hasHeader(true),
      m_ignoreNonNumeric(false),
      m_fileSize(0)
{
}

void CSVReader::setFile(const QString& path)
{
    m_filePath = path;
}

void CSVReader::setSeparator(QChar sep)
{
    m_separator = sep;
}

void CSVReader::setStartLine(int line)
{
    m_startLine = line;
}

void CSVReader::setHasHeader(bool has)
{
    m_hasHeader = has;
}

void CSVReader::setIgnoreNonNumeric(bool ignore)
{
    m_ignoreNonNumeric = ignore;
}

QString CSVReader::getFilePath() const
{
    return m_filePath;
}

QChar CSVReader::getSeparator() const
{
    return m_separator;
}

int CSVReader::getStartLine() const
{
    return m_startLine;
}

bool CSVReader::getHasHeader() const
{
    return m_hasHeader;
}

bool CSVReader::getIgnoreNonNumeric() const
{
    return m_ignoreNonNumeric;
}

bool CSVReader::parse()
{
    m_data.clear();
    m_headers.clear();

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file:" << m_filePath;
        return false;
    }

    QTextStream in(&file);
    int currentLine = 0;

    while (!in.atEnd()) {
        currentLine++;
        QString line = in.readLine();

        if (currentLine < m_startLine) {
            continue;
        }

        if (m_hasHeader && currentLine == m_startLine) {
            m_headers = line.split(m_separator);
            continue;
        }

        QStringList fields = line.split(m_separator);
        QVector<double> row;
        bool isNumeric = false;

        for (const QString& field : fields) {
            bool ok;
            double value = field.toDouble(&ok);
            if (ok) {
                row.append(value);
                isNumeric = true;
            } else {
                row.append(0.0); // Or some other default value
            }
        }

        if (m_ignoreNonNumeric && !isNumeric) {
            continue;
        }

        m_data.append(row);
    }

    if (!m_hasHeader) {
        if (!m_data.isEmpty()) {
            for (int i = 0; i < m_data.first().size(); ++i) {
                m_headers.append(QString("Col%1").arg(i + 1));
            }
        }
    }

    m_fileSize = file.size();
    file.close();
    return true;
}

QStringList CSVReader::getHeaders() const
{
    return m_headers;
}

QJsonObject CSVReader::toJson() const
{
    QJsonObject obj;
    obj["filePath"] = m_filePath;
    obj["separator"] = QString(m_separator);
    obj["startLine"] = m_startLine;
    obj["hasHeader"] = m_hasHeader;
    obj["ignoreNonNumeric"] = m_ignoreNonNumeric;
    return obj;
}

void CSVReader::fromJson(const QJsonObject& obj)
{
    if (obj.contains("filePath")) m_filePath = obj["filePath"].toString();
    if (obj.contains("separator")) {
        QString s = obj["separator"].toString();
        if (!s.isEmpty()) m_separator = s.at(0);
    }
    if (obj.contains("startLine")) m_startLine = obj["startLine"].toInt();
    if (obj.contains("hasHeader")) m_hasHeader = obj["hasHeader"].toBool();
    if (obj.contains("ignoreNonNumeric")) m_ignoreNonNumeric = obj["ignoreNonNumeric"].toBool();
}

QVector<QVector<double>> CSVReader::getData() const
{
    return m_data;
}

bool CSVReader::readNewLines()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file:" << m_filePath;
        return false;
    }

    if (file.size() < m_fileSize) {
        // File has been truncated or replaced, re-parse from the beginning
        m_fileSize = 0;
        return parse();
    }

    if (file.size() == m_fileSize) {
        // No new data
        file.close();
        return true;
    }

    if (!file.seek(m_fileSize)) {
        qWarning() << "Could not seek to position" << m_fileSize << "in file" << m_filePath;
        file.close();
        return false;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty()) continue;

        QStringList fields = line.split(m_separator);
        QVector<double> row;
        bool isNumeric = false;

        for (const QString& field : fields) {
            bool ok;
            double value = field.toDouble(&ok);
            if (ok) {
                row.append(value);
                isNumeric = true;
            } else {
                row.append(0.0);
            }
        }

        if (m_ignoreNonNumeric && !isNumeric) {
            continue;
        }

        m_data.append(row);
    }

    m_fileSize = file.size();
    file.close();
    return true;
}
