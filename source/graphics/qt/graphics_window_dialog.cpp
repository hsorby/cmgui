#include "graphics_window_dialog.h"
#include "ui_graphics_window_dialog.h"

GraphicsWindowDialog::GraphicsWindowDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GraphicsWindowDialog)
{
    ui->setupUi(this);

    makeConnections();
}

GraphicsWindowDialog::~GraphicsWindowDialog()
{
    delete ui;
}

void GraphicsWindowDialog::makeConnections()
{

}
