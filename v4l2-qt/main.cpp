#include "v4l2.h"
#include "showphoto.h"
#include <QApplication>
v4l2 *w = nullptr;
showphoto *s=nullptr;
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    w=new v4l2();
    s=new showphoto();
    w->show();
    return a.exec();
}
