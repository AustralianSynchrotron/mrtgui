#include <QtGui/QApplication>
#include "mrtwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MRTwindow w;
    w.show();

    return a.exec();
}
