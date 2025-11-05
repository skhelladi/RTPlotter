#include "ParserConfigDialog.h"
#include "ui_ParserConfigDialog.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

ParserConfigDialog::ParserConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ParserConfigDialog)
{
    ui->setupUi(this);

    connect(ui->customSeparatorLineEdit, &QLineEdit::textChanged, this, &ParserConfigDialog::updatePreview);
    connect(ui->startLineSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ParserConfigDialog::updatePreview);
    connect(ui->headerCheckBox, &QCheckBox::checkStateChanged, this, &ParserConfigDialog::updatePreview);
    connect(ui->ignoreNonNumericCheckBox, &QCheckBox::checkStateChanged, this, &ParserConfigDialog::updatePreview);
}

ParserConfigDialog::~ParserConfigDialog()
{
    delete ui;
}

CSVReader ParserConfigDialog::getReader() const
{
    m_reader.setFile(ui->filePathLineEdit->text());
    QChar separator;
    switch (ui->separatorComboBox->currentIndex()) {
        case 0: separator = ','; break;
        case 1: separator = ';'; break;
        case 2: separator = '\t'; break;
        case 3: separator = ' '; break;
        case 4: separator = ui->customSeparatorLineEdit->text().at(0); break;
    }
    m_reader.setSeparator(separator);
    m_reader.setStartLine(ui->startLineSpinBox->value());
    m_reader.setHasHeader(ui->headerCheckBox->isChecked());
    m_reader.setIgnoreNonNumeric(ui->ignoreNonNumericCheckBox->isChecked());
    return m_reader;
}

void ParserConfigDialog::setFilePath(const QString& path)
{
    ui->filePathLineEdit->setText(path);
    previewFile(path);
}

void ParserConfigDialog::applySettings(const QJsonObject& settings)
{
    if (settings.contains("filePath")) {
        QString fp = settings["filePath"].toString();
        ui->filePathLineEdit->setText(fp);
        previewFile(fp);
    }
    if (settings.contains("separator")) {
        QString sep = settings["separator"].toString();
        int idx = 0; // default comma
        if (sep == ";") idx = 1;
        else if (sep == "\t") idx = 2;
        else if (sep == " ") idx = 3;
        else if (!sep.isEmpty()) {
            idx = 4;
            ui->customSeparatorLineEdit->setText(sep);
        }
        ui->separatorComboBox->setCurrentIndex(idx);
    }
    if (settings.contains("startLine")) ui->startLineSpinBox->setValue(settings["startLine"].toInt());
    if (settings.contains("hasHeader")) ui->headerCheckBox->setChecked(settings["hasHeader"].toBool());
    if (settings.contains("ignoreNonNumeric")) ui->ignoreNonNumericCheckBox->setChecked(settings["ignoreNonNumeric"].toBool());
    updatePreview();
}

void ParserConfigDialog::on_browseButton_clicked()
{
    qDebug() << "ParserConfigDialog: browse button clicked";
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open CSV File"), "", tr("CSV Files (*.csv);;Text Files (*.txt);;Data Files (*.dat);;All Files (*)"), nullptr, QFileDialog::DontUseNativeDialog);
    if (!filePath.isEmpty()) {
        ui->filePathLineEdit->setText(filePath);
        previewFile(filePath);
    }
}

void ParserConfigDialog::on_separatorComboBox_currentIndexChanged(int index)
{
    ui->customSeparatorLineEdit->setEnabled(index == 4);
    updatePreview();
}

void ParserConfigDialog::on_filePathLineEdit_textChanged(const QString &arg1)
{
    previewFile(arg1);
}

void ParserConfigDialog::updatePreview()
{
    previewFile(ui->filePathLineEdit->text());
}

void ParserConfigDialog::previewFile(const QString& filePath)
{
    ui->previewTableWidget->clear();
    ui->previewTableWidget->setRowCount(0);
    ui->previewTableWidget->setColumnCount(0);

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    int rowCount = 0;

    QChar separator;
    if (ui->separatorComboBox->currentIndex() == 4) {
        separator = ui->customSeparatorLineEdit->text().at(0);
    } else {
        separator = QString(",;\t ").at(ui->separatorComboBox->currentIndex());
    }

    while (!in.atEnd() && rowCount < 10) {
        QString line = in.readLine();
        ui->previewTableWidget->insertRow(rowCount);
        QStringList fields = line.split(separator);
        ui->previewTableWidget->setColumnCount(qMax(ui->previewTableWidget->columnCount(), fields.size()));
        for (int i = 0; i < fields.size(); ++i) {
            ui->previewTableWidget->setItem(rowCount, i, new QTableWidgetItem(fields.at(i)));
        }
        rowCount++;
    }

    file.close();
}
