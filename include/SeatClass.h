#ifndef SEAT_CLASS_H
#define SEAT_CLASS_H

#include <string>
#include <vector>
#include <memory>

/**
 * @brief Base class for different seat classes in the Airline Reservation System.
 * Demonstrates Object-Oriented Inheritance and Runtime Polymorphism.
 */
class SeatClass {
protected:
    std::string className;
    double priceMultiplier;
    int baggageAllowanceKg;
    double refundPercentage;

public:
    SeatClass(const std::string& name, double multiplier, int baggage, double refundPct);
    virtual ~SeatClass() = default;

    // Pure virtual method for polymorphic fare calculation
    virtual double calculateFare(double baseFlightPrice) const = 0;

    // Virtual methods providing class-specific services & perks
    virtual std::string getClassName() const { return className; }
    virtual double getPriceMultiplier() const { return priceMultiplier; }
    virtual int getBaggageAllowanceKg() const { return baggageAllowanceKg; }
    virtual double getRefundPercentage() const { return refundPercentage; }

    virtual std::string getMealService() const = 0;
    virtual std::string getLoungeAccess() const = 0;
    virtual std::string getSeatType() const = 0;
    virtual std::vector<std::string> getPerks() const = 0;

    // Factory method for creating seat class instances
    static std::shared_ptr<SeatClass> create(const std::string& typeName);
};

/**
 * @brief Derived Class: Economy Class Seat
 */
class EconomySeat : public SeatClass {
public:
    EconomySeat();
    virtual ~EconomySeat() = default;

    double calculateFare(double baseFlightPrice) const override;
    std::string getMealService() const override;
    std::string getLoungeAccess() const override;
    std::string getSeatType() const override;
    std::vector<std::string> getPerks() const override;
};

/**
 * @brief Derived Class: Business Class Seat
 */
class BusinessSeat : public SeatClass {
public:
    BusinessSeat();
    virtual ~BusinessSeat() = default;

    double calculateFare(double baseFlightPrice) const override;
    std::string getMealService() const override;
    std::string getLoungeAccess() const override;
    std::string getSeatType() const override;
    std::vector<std::string> getPerks() const override;
};

/**
 * @brief Derived Class: First Class Seat
 */
class FirstClassSeat : public SeatClass {
public:
    FirstClassSeat();
    virtual ~FirstClassSeat() = default;

    double calculateFare(double baseFlightPrice) const override;
    std::string getMealService() const override;
    std::string getLoungeAccess() const override;
    std::string getSeatType() const override;
    std::vector<std::string> getPerks() const override;
};

#endif // SEAT_CLASS_H

