#ifndef GRAPHICSWINDOWDIALOG_H
#define GRAPHICSWINDOWDIALOG_H

#include <QDialog>

namespace Ui {
class GraphicsWindowDialog;
}

class GraphicsWindowDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GraphicsWindowDialog(QWidget *parent = 0);
    ~GraphicsWindowDialog();

private:
    void makeConnections();

    Ui::GraphicsWindowDialog *ui;
};

#endif // GRAPHICSWINDOWDIALOG_H
