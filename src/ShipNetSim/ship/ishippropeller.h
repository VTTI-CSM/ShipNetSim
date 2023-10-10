#ifndef I_SHIPPROPELLER_H
#define I_SHIPPROPELLER_H

#include "../../third_party/units/units.h"
#include "ishipengine.h"
#include <QString>
#include <any>
#include "ishipgearbox.h"
class Ship;  // Forward declaration of the class ship

/**
 * @brief The IShipPropeller class
 * @class IShipPropeller
 *
 * Provides an interface to manage ships propellers
 */
class IShipPropeller
{
public:
    IShipPropeller();
    ~IShipPropeller();
    virtual void initialize(Ship *ship, IShipGearBox *gearbox,
                            QMap<QString, std::any>& parameters) = 0;

    void setHost(Ship *ship);
    void setGearBox(IShipGearBox* gearbox);
    const Ship *getHost() const;
    const IShipGearBox *getGearBox() const;

    virtual void setParameters(QMap<QString, std::any>& parameters) = 0;

    [[nodiscard]] units::length::meter_t getPropellerDiameter() const;
    void setPropellerDiameter(
        const units::length::meter_t &newPropellerDiameter);

    [[nodiscard]] units::area::square_meter_t
    getPropellerExpandedBladeArea() const;
    void setPropellerExpandedBladeArea(
        const units::area::square_meter_t &newExpandedBladeArea);

    [[nodiscard]] units::area::square_meter_t getPropellerDiskArea() const;
    void setPropellerDiskArea(
        const units::area::square_meter_t &newPropellerDiskArea);

    [[nodiscard]] double getPropellerExpandedAreaRatio() const;
    void setPropellerExpandedAreaRation(double newPropellerExpandedAreaRation);

//    virtual units::power::kilowatt_t getBreakPower() = 0;

    virtual double getShaftEfficiency() = 0;
    virtual void setShaftEfficiency(double newShaftEfficiency) = 0;
    virtual double getPropellerEfficiency() = 0;
    virtual void setPropellerOpenWaterEfficiencies(
        QMap<double, double> newPropellerOpenWaterEfficiencies) = 0;



//    virtual units::power::kilowatt_t getDeliveredPower() = 0;
//    virtual units::power::kilowatt_t getThrustPower() = 0;
    virtual units::power::kilowatt_t getEffectivePower() = 0;
    virtual units::power::kilowatt_t getPreviousEffectivePower() = 0;
    virtual units::torque::newton_meter_t getTorque() = 0;
    virtual units::force::newton_t getThrust() = 0;
    virtual units::angular_velocity::revolutions_per_minute_t getRPM() = 0;
    virtual double getThrustCoefficient() = 0;
    virtual double getTorqueCoefficient() = 0;
    virtual double getAdvancedRatio() = 0;

    virtual const QVector<IShipEngine*> getDrivingEngines() const = 0;


protected:
    Ship *mHost;
    IShipGearBox* mGearBox;


    units::length::meter_t mPropellerDiameter;
    units::area::square_meter_t mExpandedBladeArea;
    units::area::square_meter_t mPropellerDiskArea;
    double mPropellerExpandedAreaRatio;
};

#endif //I_SHIPPROPELLER_H
