#ifndef UNITTYPE_H
#define UNITTYPE_H

#include <functional>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include "../../third_party/units/units.h"

// Base class for type erasure
struct UnitBase {
    virtual ~UnitBase() = default;
    virtual std::unique_ptr<UnitBase> clone() const = 0;
    virtual std::type_index type() const = 0;
};

template <typename T>
struct UnitHolder : UnitBase {
    T value;

    UnitHolder(T v) : value(v) {}

    std::unique_ptr<UnitBase> clone() const override {
        return std::make_unique<UnitHolder<T>>(*this);
    }

    std::type_index type() const override {
        return typeid(T);
    }
};

// Wrapper class for type erasure with type restrictions
template <typename... AllowedTypes>
class Unit {
public:
    template <typename T>
    Unit(T value) : unit_holder_(std::make_unique<UnitHolder<T>>(value)) {
        static_assert(is_allowed<T>(),
                      "Type is not allowed in this Unit variant");
    }

    Unit(const Unit& other) : unit_holder_(other.unit_holder_->clone()) {}

    Unit& operator=(const Unit& other) {
        if (this != &other) {
            unit_holder_ = other.unit_holder_->clone();
        }
        return *this;
    }

    template <typename T>
    T get() const {
        if (auto holder = dynamic_cast<UnitHolder<T>*>(unit_holder_.get())) {
            return holder->value;
        } else {
            throw std::bad_cast();
        }
    }

    std::type_index type() const {
        return unit_holder_->type();
    }

    template <typename TargetUnit>
    TargetUnit convertTo() const {
        return convertTo<TargetUnit>(*this);
    }

private:
    std::unique_ptr<UnitBase> unit_holder_;

    template <typename T>
    static constexpr bool is_allowed() {
        return (std::is_same_v<T, AllowedTypes> || ...);
    }

    // General conversion function template
    template <typename TargetUnit, typename Variant>
    TargetUnit convertTo(const Variant& variant) {
        static const std::unordered_map<std::type_index, std::function<TargetUnit(const Variant&)>> converters = {
            { typeid(units::length::meter_t), [](const Variant& u) { return u.template get<units::length::meter_t>().template convert<TargetUnit>(); }},
            { typeid(units::length::foot_t), [](const Variant& u) { return u.template get<units::length::foot_t>().template convert<TargetUnit>(); }},
            { typeid(units::length::inch_t), [](const Variant& u) { return u.template get<units::length::inch_t>().template convert<TargetUnit>(); }},
            { typeid(units::length::kilometer_t), [](const Variant& u) { return u.template get<units::length::kilometer_t>().template convert<TargetUnit>(); }},
            { typeid(units::length::mile_t), [](const Variant& u) { return u.template get<units::length::mile_t>().template convert<TargetUnit>(); }},
            { typeid(units::area::square_meter_t), [](const Variant& u) { return u.template get<units::area::square_meter_t>().template convert<TargetUnit>(); }},
            { typeid(units::area::square_foot_t), [](const Variant& u) { return u.template get<units::area::square_foot_t>().template convert<TargetUnit>(); }},
            { typeid(units::area::square_inch_t), [](const Variant& u) { return u.template get<units::area::square_inch_t>().template convert<TargetUnit>(); }},
            { typeid(units::area::acre_t), [](const Variant& u) { return u.template get<units::area::acre_t>().template convert<TargetUnit>(); }},
            { typeid(units::area::hectare_t), [](const Variant& u) { return u.template get<units::area::hectare_t>().template convert<TargetUnit>(); }},
            { typeid(units::volume::cubic_meter_t), [](const Variant& u) { return u.template get<units::volume::cubic_meter_t>().template convert<TargetUnit>(); }},
            { typeid(units::volume::liter_t), [](const Variant& u) { return u.template get<units::volume::liter_t>().template convert<TargetUnit>(); }},
            { typeid(units::volume::gallon_t), [](const Variant& u) { return u.template get<units::volume::gallon_t>().template convert<TargetUnit>(); }},
            { typeid(units::volume::cubic_foot_t), [](const Variant& u) { return u.template get<units::volume::cubic_foot_t>().template convert<TargetUnit>(); }},
            { typeid(units::volume::cubic_inch_t), [](const Variant& u) { return u.template get<units::volume::cubic_inch_t>().template convert<TargetUnit>(); }},
            { typeid(units::mass::kilogram_t), [](const Variant& u) { return u.template get<units::mass::kilogram_t>().template convert<TargetUnit>(); }},
            { typeid(units::mass::gram_t), [](const Variant& u) { return u.template get<units::mass::gram_t>().template convert<TargetUnit>(); }},
            { typeid(units::mass::pound_t), [](const Variant& u) { return u.template get<units::mass::pound_t>().template convert<TargetUnit>(); }},
            { typeid(units::mass::ounce_t), [](const Variant& u) { return u.template get<units::mass::ounce_t>().template convert<TargetUnit>(); }},
            { typeid(units::mass::metric_ton_t), [](const Variant& u) { return u.template get<units::mass::metric_ton_t>().template convert<TargetUnit>(); }},
            { typeid(units::mass::long_ton_t), [](const Variant& u) { return u.template get<units::mass::long_ton_t>().template convert<TargetUnit>(); }},
            { typeid(units::force::newton_t), [](const Variant& u) { return u.template get<units::force::newton_t>().template convert<TargetUnit>(); }},
            { typeid(units::force::pound_t), [](const Variant& u) { return u.template get<units::force::pound_t>().template convert<TargetUnit>(); }},
            { typeid(units::force::kilonewton_t), [](const Variant& u) { return u.template get<units::force::kilonewton_t>().template convert<TargetUnit>(); }}
        };

        auto it = converters.find(variant.type());
        if (it != converters.end()) {
            return it->second(variant);
        } else {
            throw std::invalid_argument("Unsupported type for conversion");
        }
    }
};

// Define types with restrictions
using LengthVariant = Unit<units::length::meter_t, units::length::foot_t, units::length::inch_t, units::length::kilometer_t, units::length::mile_t>;
using AreaVariant = Unit<units::area::square_meter_t, units::area::square_foot_t, units::area::square_inch_t, units::area::acre_t, units::area::hectare_t>;
using VolumeVariant = Unit<units::volume::cubic_meter_t, units::volume::liter_t, units::volume::gallon_t, units::volume::cubic_foot_t, units::volume::cubic_inch_t>;
using WeightVariant = Unit<units::mass::kilogram_t, units::mass::gram_t, units::mass::pound_t, units::mass::ounce_t, units::mass::metric_ton_t, units::mass::long_ton_t>;
using ForceVariant = Unit<units::force::newton_t, units::force::pound_t, units::force::kilonewton_t>;


#endif // UNITTYPE_H
