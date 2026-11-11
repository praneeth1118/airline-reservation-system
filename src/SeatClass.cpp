#include "../include/SeatClass.h"
#include <algorithm>

// Base Class Constructor
SeatClass::SeatClass(const std::string& name, double multiplier, int baggage, double refundPct)
    : className(name), priceMultiplier(multiplier), baggageAllowanceKg(baggage), refundPercentage(refundPct) {}

// Factory Method
std::shared_ptr<SeatClass> SeatClass::create(const std::string& typeName) {
    std::string lower = typeName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "first" || lower == "first class" || lower == "firstclass") {
        return std::make_shared<FirstClassSeat>();
    } else if (lower == "business" || lower == "business class") {
        return std::make_shared<BusinessSeat>();
    } else {
        return std::make_shared<EconomySeat>();
    }
}

// ----------------- Economy Class Implementation -----------------
EconomySeat::EconomySeat()
    : SeatClass("Economy", 1.0, 20, 0.70) {} // 1.0x price, 20kg baggage, 70% refund on cancellation

double EconomySeat::calculateFare(double baseFlightPrice) const {
    return baseFlightPrice * priceMultiplier;
}

std::string EconomySeat::getMealService() const {
    return "Complimentary Snack & Non-Alcoholic Beverage";
}

std::string EconomySeat::getLoungeAccess() const {
    return "Not Included";
}

std::string EconomySeat::getSeatType() const {
    return "Standard Ergonomic Reclining Seat (31\" Pitch)";
}

std::vector<std::string> EconomySeat::getPerks() const {
    return {
        "20 kg Checked Baggage + 7 kg Cabin Bag",
        "In-flight Entertainment Screen",
        "Complimentary Snack & Drink",
        "Standard Seat Selection",
        "70% Refundable Cancellation"
    };
}

// ----------------- Business Class Implementation -----------------
BusinessSeat::BusinessSeat()
    : SeatClass("Business", 2.2, 40, 0.85) {} // 2.2x price, 40kg baggage, 85% refund

double BusinessSeat::calculateFare(double baseFlightPrice) const {
    return baseFlightPrice * priceMultiplier;
}

std::string BusinessSeat::getMealService() const {
    return "Chef-curated 3-Course Gourmet Meal with Wine Pairing";
}

std::string BusinessSeat::getLoungeAccess() const {
    return "Priority Airport Business Lounge Access";
}

std::string BusinessSeat::getSeatType() const {
    return "Lie-Flat Bed Seat with Direct Aisle Access (78\" Length)";
}

std::vector<std::string> BusinessSeat::getPerks() const {
    return {
        "40 kg Checked Baggage (2 pieces) + 14 kg Cabin",
        "Priority Check-in & Fast-track Airport Security",
        "Access to Airport Business Lounges",
        "180-degree Full Flat-bed Seat",
        "Chef-curated 3-Course Hot Dining & Fine Wines",
        "High-speed In-flight Wi-Fi Voucher",
        "85% Refundable Cancellation"
    };
}

// ----------------- First Class Implementation -----------------
FirstClassSeat::FirstClassSeat()
    : SeatClass("First Class", 3.8, 60, 1.00) {} // 3.8x price, 60kg baggage, 100% full refund

double FirstClassSeat::calculateFare(double baseFlightPrice) const {
    return baseFlightPrice * priceMultiplier;
}

std::string FirstClassSeat::getMealService() const {
    return "Michelin-Star Inspired A La Carte Dining & Vintage Champagne";
}

std::string FirstClassSeat::getLoungeAccess() const {
    return "Exclusive VIP Royal First Lounge & Complimentary Spa Treatment";
}

std::string FirstClassSeat::getSeatType() const {
    return "Private Enclosed Suite with Sliding Doors & Personal Wardrobe";
}

std::vector<std::string> FirstClassSeat::getPerks() const {
    return {
        "60 kg Generous Baggage Allowance",
        "Private VIP Suite with Sliding Door Privacy",
        "Complimentary Luxury Chauffeur Airport Transfer",
        "Access to VIP First Lounge & Complimentary Spa",
        "Dedicated Personal Flight Attendant",
        "Michelin-Star In-flight Dining & Premium Champagne",
        "Designer Amenity Kit & Silk Pajamas",
        "100% Full Refund Guarantee on Cancellation"
    };
}

