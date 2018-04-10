#include "openglwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    OpenGLWindow w;
    w.resize(800, 600);
    w.show();
    w.setAnimating(true);

    return a.exec();
}
