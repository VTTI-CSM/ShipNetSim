#include <QCoreApplication>
#include "utils/logger.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Logger::attach();






    Logger::detach();
    return a.exec();
}
