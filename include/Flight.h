#ifndef FLIGHT_H
#define FLIGHT_H

#include <string>
#include <vector>
#include <set>
#include <memory>
#include "SeatClass.h"

class Flight {
private:
    std::string flightNumber;
    std::string airlineName;
    std::string origin;
    std::string originCode;
    std::string destination;
    std::string destinationCode;
    std::string departureTime;
    std::string arrivalTime;
    std::string duration;
    std::string aircraft;
    double basePrice;

    // Capacities
    int economyCapacity;
    int businessCapacity;
    int firstCapacity;

    // Booked seats sets
    std::set<std::string> bookedEconomySeats;
    std::set<std::string> bookedBusinessSeats;
    std::set<std::string> bookedFirstSeats;

public:
    Flight();
    Flight(const std::string& flightNo, const std::string& airline,
           const std::string& orig, const std::string& origCode,
           const std::string& dest, const std::string& destCode,
           const std::string& depTime, const std::string& arrTime,
           const std::string& dur, const std::string& plane,
           double base, int econCap = 60, int bizCap = 18, int firstCap = 8);

    // Getters
    std::string getFlightNumber() const { return flightNumber; }
    std::string getAirlineName() const { return airlineName; }
    std::string getOrigin() const { return origin; }
    std::string getOriginCode() const { return originCode; }
    std::string getDestination() const { return destination; }
    std::string getDestinationCode() const { return destinationCode; }
    std::string getDepartureTime() const { return departureTime; }
    std::string getArrivalTime() const { return arrivalTime; }
    std::string getDuration() const { return duration; }
    std::string getAircraft() const { return aircraft; }
    double getBasePrice() const { return basePrice; }

    int getCapacity(const std::string& className) const;
    int getBookedCount(const std::string& className) const;
    int getAvailableSeats(const std::string& className) const;
    double getPriceForClass(const std::string& className) const;

    bool isSeatAvailable(const std::string& className) const;
    std::string allocateSeat(const std::string& className, const std::string& requestedSeat = "");
    bool releaseSeat(const std::string& className, const std::string& seatNum);
    bool isSeatOccupied(const std::string& seatNum) const;

    std::vector<std::string> getBookedSeatsList() const;
};

#endif // FLIGHT_H

