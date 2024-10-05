#include "antonovcnc.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    AntonovCNC w;

    a.setWindowIcon(QIcon("C:/Users/technobublik/TBK_FILES/Project/LearnDEV/c++/MAG_DEV_FILES/ICO.ico"));

    w.show();
    return a.exec();
}
