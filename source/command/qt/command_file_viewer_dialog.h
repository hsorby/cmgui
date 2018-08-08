
#pragma once

#include <QDialog>

#include "command_file_viewer_codes.h"


namespace Ui {
class CommandFileViewerDialog;
}

class CommandFileViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommandFileViewerDialog(QWidget *parent = 0);
    ~CommandFileViewerDialog();

    void readFile(const QString &fileName);
    QStringList getCommandStrings(CommandFileViewerCodes commandCode) const;

signals:
    void executeCommands(CommandFileViewerCodes commandCode);

private slots:
    void onExecuteAllPushButtonClicked();
    void onExecuteSelectedPushButtonClicked();

private:
    void makeConnections();

    Ui::CommandFileViewerDialog *ui;
};
