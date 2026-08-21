#include "homecontrolcenter.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QDirIterator>
#include <QStyleFactory>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":/GlobalStyle.qss");
    qss.open(QFile::ReadOnly);
    QString style = qss.readAll();
    qApp->setStyleSheet(style);
    qss.close();

    HomeControlCenter center;
    center.show();
    return a.exec();
}
