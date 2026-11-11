#ifndef AIRLINE_MANAGER_H
#define AIRLINE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Flight.h"
#include "Ticket.h"
#include "Passenger.h"
#include "SeatClass.h"

class AirlineManager {
private:
    std::map<std::string, Flight> flights;
    std::map<std::string, Ticket> tickets;
    int nextTicketSequence;

    std::string generateTicketId();
    std::string getCurrentDateTime();

public:
    AirlineManager();

    // Data initialization & persistence
    void initializeSampleData();
    void loadFromFiles();
    void saveToFiles();
    void resetData();

    // Flight management
    std::string getAllFlightsJson() const;
    std::string getFlightJson(const std::string& flightNo) const;
    std::string getClassPerksJson() const;

    // Booking management
    // Returns JSON response { "success": true/false, "message": "...", "ticket": { ... } }
    std::string bookTicket(const std::string& flightNo, const Passenger& passenger,
                           const std::string& seatClassType, const std::string& requestedSeat);

    // Cancel ticket using polymorphic refund policy
    std::string cancelTicket(const std::string& ticketId);

    // Queries
    std::string getPassengersForFlightJson(const std::string& flightNo) const;
    std::string getAllBookingsJson() const;
    std::string getBookingByTicketIdJson(const std::string& ticketId) const;
    std::string getSystemStatsJson() const;
};

#endif // AIRLINE_MANAGER_H
