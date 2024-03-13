#include "ishippropeller.h"
#include "ishipgearbox.h"
#include "ship.h"

IShipPropeller::IShipPropeller()
{
    mHost = nullptr;
    mGearBox = nullptr;
}

IShipPropeller::~IShipPropeller()
{
    mHost = nullptr;
    if (mGearBox != nullptr)
    {
        delete mGearBox;
        mGearBox = nullptr;
    }
}


void IShipPropeller::setHost(Ship *host)
{
    mHost = host;
}

void IShipPropeller::setGearBox(IShipGearBox *gearbox)
{
    mGearBox = gearbox;
}

const Ship *IShipPropeller::getHost() const
{
    return mHost;
}

const IShipGearBox *IShipPropeller::getGearBox() const
{
    return mGearBox;
}


units::length::meter_t IShipPropeller::getPropellerDiameter() const
{
    return mPropellerDiameter;
}

void IShipPropeller::setPropellerDiameter(
    const units::length::meter_t &newPropellerDiameter)
{
    mPropellerDiameter = newPropellerDiameter;
}

units::length::meter_t IShipPropeller::getPropellerPitch() const
{
    return mPropellerPitch;
}

void IShipPropeller::setPropellerPitch(
    const units::length::meter_t newPropellerPitch)
{
    mPropellerPitch = newPropellerPitch;
}

units::area::square_meter_t IShipPropeller::getPropellerExpandedBladeArea() const
{
    return mExpandedBladeArea;
}

void IShipPropeller::setPropellerExpandedBladeArea(
    const units::area::square_meter_t &newExpandedBladeArea)
{
    mExpandedBladeArea = newExpandedBladeArea;
}

units::area::square_meter_t IShipPropeller::getPropellerDiskArea() const
{
    return mPropellerDiskArea;
}

void IShipPropeller::setPropellerDiskArea(
    const units::area::square_meter_t &newPropellerDiskArea)
{
    mPropellerDiskArea = newPropellerDiskArea;
}

double IShipPropeller::getPropellerExpandedAreaRatio() const
{

    return mPropellerExpandedAreaRatio;
}

void IShipPropeller::setPropellerExpandedAreaRatio(
    double newPropellerExpandedAreaRatio)
{
    mPropellerExpandedAreaRatio = newPropellerExpandedAreaRatio;
}

int IShipPropeller::getPropellerBladesCount() const
{
    return mNumberOfblades;
}

void IShipPropeller::setPropellerBladesCount(int newPropellerBladesCount)
{
    mNumberOfblades = newPropellerBladesCount;
}
