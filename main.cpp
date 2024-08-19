#include "MyGIS.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyGIS w;
    w.show();
    int nResult = a.exec();
    // 应用程序退出时，关闭日志系统
    log4cpp::Category::shutdown();  // 确保所有日志写入并释放资源
    return nResult;
}
