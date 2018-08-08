
#include "utilities/qt_ui.h"

#include <QFile>
#include <QUiLoader>

QWidget *loadUiFile(QWidget *parent, const QString &resource)
{
    QFile file(resource);
    file.open(QIODevice::ReadOnly);


    QUiLoader loader;
    return loader.load(&file, parent);
}
