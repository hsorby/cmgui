
#pragma once

#include <QMainWindow>

#include "command/qt/command_file_viewer_codes.h"

class QAction;
class QMenu;
class QLineEdit;
class QListWidget;
class CommandFileViewerDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum { MaxRecentFiles = 5 };

    enum ReadFileTypes {
        ReadNodeFile,
        ReadElementFile,
        ReadDataFile
    };
    enum WriteFileTypes {
        WriteAllFiles,
        WriteNodeFile,
        WriteElementFile,
        WriteDataFile
    };

private slots:
    void open();
    void read(ReadFileTypes fileTypeEnum);
    void write(WriteFileTypes fileTypeEnum);

    void updateRecentFileActions();
    void openRecentFile();

    void view3d();
    void viewNode();
    void viewElement();
    void viewData();

    void fontSettings();

    void editorScene();
    void editorSpectrum();
    void editorMaterial();

    void about();

    void executeCommandsFromCommandFileViewerDialog(CommandFileViewerCodes commandCode);
    void deleteCommandFileViewerDialog();

    void commandTextEntered();

    void showWindow();

private:
    void init();
    void createActions();
    void createStatusBar();
    void makeConnections();
    void readSettings();
    void writeSettings();
    bool maybeQuit();
    void openFile(const QString &fileName);

    void updateWindowsMenu();

    static QString strippedName(const QString &fullFileName);
    static bool hasRecentFiles();
    void prependToRecentFiles(const QString &fileName);
    void setRecentFilesVisible(bool visible);

    QAction *recentFileActs[MaxRecentFiles];
    QAction *recentFileSeparator;
    QAction *recentFileSubMenuAct;

    QLineEdit *commandLineEdit;
    QListWidget *historyListWidget;

    QMenu *windowMenu;

    CommandFileViewerDialog *commandFileViewerDialog;

};
