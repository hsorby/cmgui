#include "command_file_viewer_dialog.h"
#include "ui_command_file_viewer_dialog.h"

#include <QtWidgets>

CommandFileViewerDialog::CommandFileViewerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommandFileViewerDialog)
{
    ui->setupUi(this);
    makeConnections();
    setAttribute(Qt::WA_DeleteOnClose);
}

CommandFileViewerDialog::~CommandFileViewerDialog()
{
    delete ui;
}

void CommandFileViewerDialog::makeConnections()
{
    connect(ui->executeAll_pushButton, &QPushButton::clicked, this, &CommandFileViewerDialog::onExecuteAllPushButtonClicked);
    connect(ui->executeSelected_pushButton, &QPushButton::clicked, this, &CommandFileViewerDialog::onExecuteSelectedPushButtonClicked);
}

void CommandFileViewerDialog::readFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("SDI"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    QPlainTextEdit *ui_textEdit = findChild<QPlainTextEdit*>("contents_plainTextEdit");

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ui_textEdit->setPlainText(in.readAll());
    QApplication::restoreOverrideCursor();
}

QStringList CommandFileViewerDialog::getCommandStrings(CommandFileViewerCodes commandCode) const
{
    QStringList commands;
    if (commandCode == CommandCodeSelected) {
        QTextCursor cursor = ui->contents_plainTextEdit->textCursor();
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();
        cursor.setPosition(selectionStart);
        int blockSelectionStart = cursor.blockNumber();
        cursor.setPosition(selectionEnd);
        int blockSelectionEnd = cursor.blockNumber();
        for (int i=blockSelectionStart; i<=blockSelectionEnd; ++i) {
            QString line = ui->contents_plainTextEdit->document()->findBlockByNumber(i).text();
            if (!line.isEmpty()) {
                commands << line;
            }
        }
    } else if (commandCode == CommandCodeAll) {
        QString documentText = ui->contents_plainTextEdit->toPlainText();
        commands = documentText.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
    }

    return commands;
}

void CommandFileViewerDialog::onExecuteAllPushButtonClicked()
{
    emit executeCommands(CommandFileViewerCodes::CommandCodeAll);
}

void CommandFileViewerDialog::onExecuteSelectedPushButtonClicked()
{
    emit executeCommands(CommandFileViewerCodes::CommandCodeSelected);
}
