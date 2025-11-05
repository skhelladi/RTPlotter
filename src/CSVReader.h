#ifndef CSVREADER_H
#define CSVREADER_H

#include <QString>
#include <QVector>
#include <QStringList>
#include <QJsonObject>

class CSVReader {
public:
    CSVReader();

    void setFile(const QString& path);
    void setSeparator(QChar sep);
    void setStartLine(int line);
    void setHasHeader(bool has);
    void setIgnoreNonNumeric(bool ignore);

    bool parse();
    QStringList getHeaders() const;
    QVector<QVector<double>> getData() const;
    bool readNewLines();

    QString getFilePath() const;
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
    QChar getSeparator() const;
    int getStartLine() const;
    bool getHasHeader() const;
    bool getIgnoreNonNumeric() const;

private:
    QString m_filePath;
    QChar m_separator;
    int m_startLine;
    bool m_hasHeader;
    bool m_ignoreNonNumeric;
    QStringList m_headers;
    QVector<QVector<double>> m_data;
    qint64 m_fileSize;
};

#endif // CSVREADER_H
