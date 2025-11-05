#ifndef PARSERCONFIGDIALOG_H
#define PARSERCONFIGDIALOG_H

#include <QDialog>
#include "CSVReader.h"
#include <QJsonObject>

namespace Ui {
class ParserConfigDialog;
}

class ParserConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ParserConfigDialog(QWidget *parent = nullptr);
    ~ParserConfigDialog();

    CSVReader getReader() const;
    void setFilePath(const QString& path);
    void applySettings(const QJsonObject& settings);

private slots:
    void on_browseButton_clicked();
    void on_separatorComboBox_currentIndexChanged(int index);
    void on_filePathLineEdit_textChanged(const QString &arg1);
    void updatePreview();

private:
    Ui::ParserConfigDialog *ui;
    mutable CSVReader m_reader;

    void previewFile(const QString& filePath);
};

#endif // PARSERCONFIGDIALOG_H
