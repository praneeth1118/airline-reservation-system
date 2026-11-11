#include "../include/Ticket.h"

Ticket::Ticket()
    : ticketId(""), flightNumber(""), passenger(), seatClass(nullptr),
      seatNumber(""), bookingDate(""), status("CONFIRMED"), farePaid(0.0), refundAmount(0.0) {}

Ticket::Ticket(const std::string& id, const std::string& flightNo, const Passenger& pass,
               std::shared_ptr<SeatClass> seat, const std::string& seatNum,
               const std::string& date, double fare)
    : ticketId(id), flightNumber(flightNo), passenger(pass), seatClass(seat),
      seatNumber(seatNum), bookingDate(date), status("CONFIRMED"), farePaid(fare), refundAmount(0.0) {}

bool Ticket::cancelTicket() {
    if (status == "CANCELLED") {
        return false; // Already cancelled
    }
    status = "CANCELLED";
    if (seatClass) {
        // Polymorphic call: refund percentage varies by seat class!
        refundAmount = farePaid * seatClass->getRefundPercentage();
    } else {
        refundAmount = 0.0;
    }
    return true;
}

