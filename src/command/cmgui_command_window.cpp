

#include "command/cmgui_command_window.h"

#include <QtWidgets>

#include "utilities/qt_ui.h"
#include "utilities/qt_metatypes.h"
#include "configure/cmgui_version.h"

#include "command/command_file_viewer_dialog.h"

static inline QString recentFilesKey() { return QStringLiteral("recentFileList"); }
static inline QString fileKey() { return QStringLiteral("file"); }

CmguiCommandWindow::CmguiCommandWindow()
    : commandFileViewerDialog(nullptr)
    , model(nullptr)
{
    setWindowTitle("Command Window");
    init();
}

void CmguiCommandWindow::setModel(QSharedPointer<CmguiCommandModel> model)
{
    this->model = model;
}

void CmguiCommandWindow::open()
{
    const QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
        openFile(fileName);
    }
}

void CmguiCommandWindow::openFile(const QString &fileName)
{
    CmguiCommandWindow::prependToRecentFiles(fileName);
    if (commandFileViewerDialog == nullptr) {
        commandFileViewerDialog = new CommandFileViewerDialog(this);
    }
    connect(commandFileViewerDialog, &CommandFileViewerDialog::executeCommands, this, &CmguiCommandWindow::executeCommandsFromCommandFileViewerDialog);
    connect(commandFileViewerDialog, &CommandFileViewerDialog::finished, this, &CmguiCommandWindow::deleteCommandFileViewerDialog);
    commandFileViewerDialog->readFile(fileName);
    commandFileViewerDialog->show();
}

void CmguiCommandWindow::executeCommandsFromCommandFileViewerDialog(CommandFileViewerCodes commandCode)
{
    CommandFileViewerDialog *dlg = static_cast<CommandFileViewerDialog*>(sender());
    QStringList commands = dlg->getCommandStrings(commandCode);
    qDebug() << commands;
}

void CmguiCommandWindow::deleteCommandFileViewerDialog()
{
    commandFileViewerDialog = nullptr;
}

void CmguiCommandWindow::commandTextEntered()
{
    QString command = commandLineEdit->text();
    historyListWidget->addItem(command);
    if (model->executeCommandFromString(command)) {
        commandLineEdit->clear();
    } else {
        qDebug() << "Command failed.";
    }
}

void CmguiCommandWindow::updateWindowsMenu()
{
    windowMenu->clear();
    QAction *minimizeAction = new QAction(tr("Minimize"), this);
    windowMenu->addAction(minimizeAction);
    QAction *zoomAction = new QAction(tr("Zoom"), this);
    windowMenu->addAction(zoomAction);
    windowMenu->addSeparator();
    QWidgetList widgetList = QApplication::topLevelWidgets();
    for (int i = 0; i < widgetList.count(); ++i) {
        QString windowTitle = widgetList.at(i)->windowTitle();
        if (windowTitle.length() > 0) {
            QAction *action = new QAction(widgetList.at(i)->windowTitle(), this);
            connect(action, &QAction::triggered, this, &CmguiCommandWindow::showWindow);
            QVariant v = qVariantFromValue(widgetList.at(i));
            action->setData(v);
            action->setCheckable(true);
            action->setChecked(widgetList.at(i) == QApplication::activeWindow());
            windowMenu->addAction(action);
        }
    }
}

void CmguiCommandWindow::showWindow()
{
    QAction *action = static_cast<QAction *>(sender());
    QVariant v = action->data();
    QWidget *widget = v.value<QWidget *>();
    widget->show();
    widget->raise();
    widget->activateWindow();
}

void CmguiCommandWindow::read(ReadFileTypes fileTypeEnum)
{
    qDebug() << fileTypeEnum;
}

void CmguiCommandWindow::write(WriteFileTypes fileTypeEnum)
{
    qDebug() << fileTypeEnum;
}

void CmguiCommandWindow::view3d()
{
    qDebug() << "View 3D window.";
}

void CmguiCommandWindow::viewNode()
{
    qDebug() << "View node window.";
}

void CmguiCommandWindow::viewElement()
{
    qDebug() << "View element window.";
}

void CmguiCommandWindow::viewData()
{
    qDebug() << "View data window.";
}

void CmguiCommandWindow::fontSettings()
{
    qDebug() << "View font settings.";
}

void CmguiCommandWindow::editorScene()
{
    qDebug() << "View scene editor.";
}

void CmguiCommandWindow::editorSpectrum()
{
    qDebug() << "View spectrum editor.";
}

void CmguiCommandWindow::editorMaterial()
{
    qDebug() << "View material editor.";
}

void CmguiCommandWindow::about()
{
    QString heading;
    QTextStream stream(&heading);
    stream << "<h1>Cmgui version " << CMGUI_VERSION_STRING << "</h1>" << endl;
    stream << tr("Cmgui is an application for 3D visualisation, field storage, and mathematical field abstraction.");

    QMessageBox::about(this, tr("About Cmgui"), heading);
}

void CmguiCommandWindow::init()
{
    //setAttribute(Qt::WA_DeleteOnClose);
    QWidget* centralWidget = loadUiFile(this, ":/mainwindowwidget");
    setCentralWidget(centralWidget);

    commandLineEdit = findChild<QLineEdit*>("command_lineEdit");
    historyListWidget = findChild<QListWidget*>("history_listWidget");

    createActions();

    createStatusBar();

    readSettings();

    makeConnections();

    setUnifiedTitleAndToolBarOnMac(true);
}

void CmguiCommandWindow::makeConnections()
{
    connect(commandLineEdit, &QLineEdit::returnPressed, this, &CmguiCommandWindow::commandTextEntered);

}

void CmguiCommandWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open a Cmgui command file"));
    connect(openAct, &QAction::triggered, this, &CmguiCommandWindow::open);
    fileMenu->addAction(openAct);

    QMenu *readFileMenu = fileMenu->addMenu(tr("Read..."));
    QAction *readNodeAct = new QAction(tr("Node file"), this);
    readNodeAct->setStatusTip(tr("Read a Cmgui node file"));
    connect(readNodeAct, &QAction::triggered, this, [this]{read(ReadNodeFile);});
    readFileMenu->addAction(readNodeAct);
    QAction *readElementAct = new QAction(tr("Elements file"), this);
    readElementAct->setStatusTip(tr("Read a Cmgui element file"));
    connect(readElementAct, &QAction::triggered, this, [this]{read(ReadElementFile);});
    readFileMenu->addAction(readElementAct);
    QAction *readDataAct = new QAction(tr("Data file"), this);
    readDataAct->setStatusTip(tr("Read a Cmgui data file"));
    connect(readDataAct, &QAction::triggered, this, [this]{read(ReadDataFile);});
    readFileMenu->addAction(readDataAct);

    QMenu *writeFileMenu = fileMenu->addMenu(tr("Write..."));
    QAction *writeAllAct = new QAction(tr("All files"), this);
    writeAllAct->setStatusTip(tr("Write all Cmgui files"));
    connect(writeAllAct, &QAction::triggered, this, [this]{write(WriteAllFiles);});
    writeFileMenu->addAction(writeAllAct);
    QAction *writeNodeAct = new QAction(tr("Node file"), this);
    writeNodeAct->setStatusTip(tr("Write a Cmgui node file"));
    connect(writeNodeAct, &QAction::triggered, this, [this]{write(WriteNodeFile);});
    writeFileMenu->addAction(writeNodeAct);
    QAction *writeElementAct = new QAction(tr("Elements file"), this);
    writeElementAct->setStatusTip(tr("Write a Cmgui element file"));
    connect(writeElementAct, &QAction::triggered, this, [this]{write(WriteElementFile);});
    writeFileMenu->addAction(writeElementAct);
    QAction *writeDataAct = new QAction(tr("Data file"), this);
    writeDataAct->setStatusTip(tr("Write a Cmgui data file"));
    connect(writeDataAct, &QAction::triggered, this, [this]{write(WriteDataFile);});
    writeFileMenu->addAction(writeDataAct);

    fileMenu->addSeparator();

    QMenu *recentMenu = fileMenu->addMenu(tr("Recent..."));
    connect(recentMenu, &QMenu::aboutToShow, this, &CmguiCommandWindow::updateRecentFileActions);
    recentFileSubMenuAct = recentMenu->menuAction();

    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = recentMenu->addAction(QString(), this, &CmguiCommandWindow::openRecentFile);
        recentFileActs[i]->setVisible(false);
    }

    recentFileSeparator = fileMenu->addSeparator();

    setRecentFilesVisible(CmguiCommandWindow::hasRecentFiles());

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), qApp, &QApplication::closeAllWindows);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));

    QMenu *modelMenu = menuBar()->addMenu(tr("&Model"));
    QAction *view3DAct = new QAction(tr("Create 3D Viewer"), this);
    view3DAct->setStatusTip(tr("Create 3D viewer window"));
    connect(view3DAct, &QAction::triggered, this,  &CmguiCommandWindow::view3d);
    modelMenu->addAction(view3DAct);
    QAction *nodeViewerAct = new QAction(tr("Show node viewer"), this);
    nodeViewerAct->setStatusTip(tr("Show node viewer"));
    connect(nodeViewerAct, &QAction::triggered, this,  &CmguiCommandWindow::viewNode);
    modelMenu->addAction(nodeViewerAct);
    QAction *elementViewerAct = new QAction(tr("Show element viewer"), this);
    elementViewerAct->setStatusTip(tr("Show element viewer"));
    connect(elementViewerAct, &QAction::triggered, this,  &CmguiCommandWindow::viewElement);
    modelMenu->addAction(elementViewerAct);
    QAction *dataViewerAct = new QAction(tr("show data viewer"), this);
    dataViewerAct->setStatusTip(tr("Show data viewer"));
    connect(dataViewerAct, &QAction::triggered, this,  &CmguiCommandWindow::viewData);
    modelMenu->addAction(dataViewerAct);

    QMenu *fontMenu = menuBar()->addMenu(tr("&Font"));
    QAction *fontSettingsAct = new QAction(tr("Font settings"), this);
    fontSettingsAct->setStatusTip(tr("Adjust the font settings"));
    connect(fontSettingsAct, &QAction::triggered, this, &CmguiCommandWindow::fontSettings);
    fontMenu->addAction(fontSettingsAct);

    QMenu *editorMenu = menuBar()->addMenu(tr("&Editors"));
    QAction *sceneEditorAct = new QAction(tr("Scene editor"), this);
    sceneEditorAct->setStatusTip(tr("View the scene editor"));
    connect(sceneEditorAct, &QAction::triggered, this, &CmguiCommandWindow::editorScene);
    editorMenu->addAction(sceneEditorAct);
    QAction *materialEditorAct = new QAction(tr("Material editor"), this);
    materialEditorAct->setStatusTip(tr("View the material editor"));
    connect(materialEditorAct, &QAction::triggered, this, &CmguiCommandWindow::editorMaterial);
    editorMenu->addAction(materialEditorAct);
    QAction *spectrumEditorAct = new QAction(tr("Spectrum editor"), this);
    spectrumEditorAct->setStatusTip(tr("View the spectrum editor"));
    connect(spectrumEditorAct, &QAction::triggered, this, &CmguiCommandWindow::editorSpectrum);
    editorMenu->addAction(spectrumEditorAct);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(windowMenu, &QMenu::aboutToShow, this, &CmguiCommandWindow::updateWindowsMenu);
    QAction *emptyAction = new QAction(tr("Empty"), this);
    windowMenu->addAction(emptyAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &CmguiCommandWindow::about);
    aboutAct->setStatusTip(tr("Show the application's About box"));
}

void CmguiCommandWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void CmguiCommandWindow::setRecentFilesVisible(bool visible)
{
    recentFileSubMenuAct->setVisible(visible);
    recentFileSeparator->setVisible(visible);
}

static QStringList readRecentFiles(QSettings &settings)
{
    QStringList result;
    const int count = settings.beginReadArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        result.append(settings.value(fileKey()).toString());
    }
    settings.endArray();
    return result;
}

static void writeRecentFiles(const QStringList &files, QSettings &settings)
{
    const int count = files.size();
    settings.beginWriteArray(recentFilesKey());
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        settings.setValue(fileKey(), files.at(i));
    }
    settings.endArray();
}

bool CmguiCommandWindow::hasRecentFiles()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const int count = settings.beginReadArray(recentFilesKey());
    settings.endArray();
    return count > 0;
}

void CmguiCommandWindow::prependToRecentFiles(const QString &fileName)
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    const QStringList oldRecentFiles = readRecentFiles(settings);
    QStringList recentFiles = oldRecentFiles;
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    if (oldRecentFiles != recentFiles)
        writeRecentFiles(recentFiles, settings);

    setRecentFilesVisible(!recentFiles.isEmpty());
}

void CmguiCommandWindow::updateRecentFileActions()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

    const QStringList recentFiles = readRecentFiles(settings);
    const int count = qMin(int(MaxRecentFiles), recentFiles.size());
    int i = 0;
    for ( ; i < count; ++i) {
        const QString fileName = CmguiCommandWindow::strippedName(recentFiles.at(i));
        recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
        recentFileActs[i]->setData(recentFiles.at(i));
        recentFileActs[i]->setVisible(true);
    }
    for ( ; i < MaxRecentFiles; ++i)
        recentFileActs[i]->setVisible(false);
}

void CmguiCommandWindow::openRecentFile()
{
    if (const QAction *action = qobject_cast<const QAction *>(sender()))
        openFile(action->data().toString());
}

void CmguiCommandWindow::readSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void CmguiCommandWindow::writeSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

void CmguiCommandWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    if (maybeQuit()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

bool CmguiCommandWindow::maybeQuit()
{
    return true;
    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("Cmgui"),
                               tr("Are you sure you want to quit?"),
                               QMessageBox::Yes | QMessageBox::No);
    switch (ret) {
    case QMessageBox::Yes:
        return true;
    case QMessageBox::No:
        return false;
    default:
        break;
    }
    return true;
}

QString CmguiCommandWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}
