
#pragma once

#include <QList>
#include <QSharedPointer>
#include <QString>

template<class T>
class List: public QList< T >
{

};

template<class Y>
class NamedList: public List< Y >
{

};
