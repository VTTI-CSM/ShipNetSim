#include "QtWidgets/qapplication.h"
#include "hydrologytest.h"
#include "shiptest.h"
#include "holtropresistancemethodtest.h"

//#define RUN_HYDROLOGY_TESTS
//#define RUN_SHIP_TESTS
#define RUN_HOLTROP_RESISTANCE_METHOD_TEST

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    int status = 0;

#ifdef RUN_HYDROLOGY_TESTS
    {
        HydrologyTest ht;
        status |= QTest::qExec(&ht, argc, argv);
    }
#endif

#ifdef RUN_SHIP_TESTS
    {
        ShipTest ts;
        status |= QTest::qExec(&ts, argc, argv);
    }
#endif

#ifdef RUN_HOLTROP_RESISTANCE_METHOD_TEST
    {
        HoltropResistanceMethodTest hrt;
        status |= QTest::qExec(&hrt, argc, argv);
    }

#endif

    return status;
}
