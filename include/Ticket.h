#ifndef TICKET_H
#define TICKET_H

#include <string>
#include <memory>
#include "Passenger.h"
#include "SeatClass.h"

/**
 * @brief Represents a Booked Ticket.
 * Holds a polymorphic pointer to SeatClass, demonstrating OOP Polymorphism.
 */
class Ticket {
private:
    std::string ticketId;
    std::string flightNumber;
    Passenger passenger;
    std::shared_ptr<SeatClass> seatClass;
    std::string seatNumber;
    std::string bookingDate;
    std::string status; // "CONFIRMED" or "CANCELLED"
    double farePaid;
    double refundAmount;

public:
    Ticket();
    Ticket(const std::string& id, const std::string& flightNo, const Passenger& pass,
           std::shared_ptr<SeatClass> seat, const std::string& seatNum,
           const std::string& date, double fare);

    // Getters
    std::string getTicketId() const { return ticketId; }
    std::string getFlightNumber() const { return flightNumber; }
    Passenger getPassenger() const { return passenger; }
    std::shared_ptr<SeatClass> getSeatClass() const { return seatClass; }
    std::string getSeatNumber() const { return seatNumber; }
    std::string getBookingDate() const { return bookingDate; }
    std::string getStatus() const { return status; }
    double getFarePaid() const { return farePaid; }
    double getRefundAmount() const { return refundAmount; }

    // Business Methods
    bool cancelTicket(); // Uses polymorphic refund rate to calculate refund
    bool isActive() const { return status == "CONFIRMED"; }
};

#endif // TICKET_H

