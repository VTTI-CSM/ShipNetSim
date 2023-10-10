#include <QCoreApplication>
#include "utils/logger.h"



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Logger::attach();

//    QMap<QString, std::any> params = createSampleParameters();
//    m_method =  new HoltropResistanceMethod();
//    Ship* m_ship = new Ship(params);




    Logger::detach();
    return a.exec();
}
