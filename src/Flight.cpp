#include "../include/Flight.h"
#include <sstream>
#include <algorithm>

Flight::Flight()
    : flightNumber(""), airlineName(""), origin(""), originCode(""),
      destination(""), destinationCode(""), departureTime(""), arrivalTime(""),
      duration(""), aircraft(""), basePrice(0.0),
      economyCapacity(60), businessCapacity(18), firstCapacity(8) {}

Flight::Flight(const std::string& flightNo, const std::string& airline,
               const std::string& orig, const std::string& origCode,
               const std::string& dest, const std::string& destCode,
               const std::string& depTime, const std::string& arrTime,
               const std::string& dur, const std::string& plane,
               double base, int econCap, int bizCap, int firstCap)
    : flightNumber(flightNo), airlineName(airline), origin(orig), originCode(origCode),
      destination(dest), destinationCode(destCode), departureTime(depTime),
      arrivalTime(arrTime), duration(dur), aircraft(plane), basePrice(base),
      economyCapacity(econCap), businessCapacity(bizCap), firstCapacity(firstCap) {}

int Flight::getCapacity(const std::string& className) const {
    std::string lower = className;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("first") != std::string::npos) return firstCapacity;
    if (lower.find("business") != std::string::npos) return businessCapacity;
    return economyCapacity;
}

int Flight::getBookedCount(const std::string& className) const {
    std::string lower = className;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find("first") != std::string::npos) return static_cast<int>(bookedFirstSeats.size());
    if (lower.find("business") != std::string::npos) return static_cast<int>(bookedBusinessSeats.size());
    return static_cast<int>(bookedEconomySeats.size());
}

int Flight::getAvailableSeats(const std::string& className) const {
    return getCapacity(className) - getBookedCount(className);
}

double Flight::getPriceForClass(const std::string& className) const {
    auto seatObj = SeatClass::create(className);
    return seatObj->calculateFare(basePrice);
}

bool Flight::isSeatAvailable(const std::string& className) const {
    return getAvailableSeats(className) > 0;
}

bool Flight::isSeatOccupied(const std::string& seatNum) const {
    if (bookedFirstSeats.count(seatNum)) return true;
    if (bookedBusinessSeats.count(seatNum)) return true;
    if (bookedEconomySeats.count(seatNum)) return true;
    return false;
}

std::string Flight::allocateSeat(const std::string& className, const std::string& requestedSeat) {
    if (!isSeatAvailable(className)) {
        return "";
    }

    std::string lower = className;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    int startRow = 7, endRow = 20;
    std::vector<std::string> cols = {"A", "B", "C", "D", "E", "F"};
    std::set<std::string>* targetSet = &bookedEconomySeats;

    if (lower.find("first") != std::string::npos) {
        startRow = 1;
        endRow = 2;
        cols = {"A", "B", "E", "F"};
        targetSet = &bookedFirstSeats;
    } else if (lower.find("business") != std::string::npos) {
        startRow = 3;
        endRow = 6;
        cols = {"A", "B", "C", "D", "E", "F"};
        targetSet = &bookedBusinessSeats;
    }

    // Check if user requested a specific seat and it's valid & available
    if (!requestedSeat.empty() && !isSeatOccupied(requestedSeat)) {
        targetSet->insert(requestedSeat);
        return requestedSeat;
    }

    // Auto-allocate first free seat in appropriate row range
    for (int r = startRow; r <= endRow; ++r) {
        for (const auto& c : cols) {
            std::string seat = std::to_string(r) + c;
            if (!isSeatOccupied(seat)) {
                targetSet->insert(seat);
                return seat;
            }
        }
    }

    return ""; // No seats available
}

bool Flight::releaseSeat(const std::string& className, const std::string& seatNum) {
    std::string lower = className;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("first") != std::string::npos) {
        return bookedFirstSeats.erase(seatNum) > 0;
    } else if (lower.find("business") != std::string::npos) {
        return bookedBusinessSeats.erase(seatNum) > 0;
    } else {
        return bookedEconomySeats.erase(seatNum) > 0;
    }
}

std::vector<std::string> Flight::getBookedSeatsList() const {
    std::vector<std::string> allSeats;
    allSeats.insert(allSeats.end(), bookedFirstSeats.begin(), bookedFirstSeats.end());
    allSeats.insert(allSeats.end(), bookedBusinessSeats.begin(), bookedBusinessSeats.end());
    allSeats.insert(allSeats.end(), bookedEconomySeats.begin(), bookedEconomySeats.end());
    return allSeats;
}

