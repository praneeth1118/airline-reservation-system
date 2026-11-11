#include "../include/AirlineManager.h"
#include "../include/ExcelExporter.h"
#include "../include/JsonUtils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <iostream>

AirlineManager::AirlineManager() : nextTicketSequence(1001) {
    initializeSampleData();
    loadFromFiles();
}

std::string AirlineManager::generateTicketId() {
    std::ostringstream ss;
    ss << "TKT-" << (nextTicketSequence++);
    return ss.str();
}

std::string AirlineManager::getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm* tmNow = std::localtime(&nowTime);
    char buf[64];
    if (tmNow) {
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tmNow);
        return std::string(buf);
    }
    return "2026-11-05 20:00";
}

void AirlineManager::initializeSampleData() {
    flights.clear();
    tickets.clear();
    nextTicketSequence = 1001;

    // 1. Flight SK101: Hyderabad to Delhi
    flights["SK101"] = Flight(
        "SK101", "SkyWings India",
        "Hyderabad (HYD)", "HYD",
        "Delhi (DEL)", "DEL",
        "09:30 AM (2026-12-10)", "11:45 AM (2026-12-10)",
        "2h 15m", "Airbus A320neo",
        480.0, 30, 10, 4
    );

    // 2. Flight SK202: Hyderabad to Mumbai
    flights["SK202"] = Flight(
        "SK202", "SkyWings India",
        "Hyderabad (HYD)", "HYD",
        "Mumbai (BOM)", "BOM",
        "14:15 PM (2026-12-11)", "15:45 PM (2026-12-11)",
        "1h 30m", "Boeing 737 MAX 8",
        350.0, 30, 10, 5
    );

    // 3. Flight SK303: Chennai to Bengaluru
    flights["SK303"] = Flight(
        "SK303", "SkyWings India",
        "Chennai (MAA)", "MAA",
        "Bengaluru (BLR)", "BLR",
        "18:45 PM (2026-12-12)", "19:45 PM (2026-12-12)",
        "1h 00m", "Airbus A321neo",
        180.0, 25, 8, 4
    );

    // 4. Flight SK404: Delhi to Kolkata
    flights["SK404"] = Flight(
        "SK404", "SkyWings India",
        "Delhi (DEL)", "DEL",
        "Kolkata (CCU)", "CCU",
        "07:00 AM (2026-12-13)", "09:15 AM (2026-12-13)",
        "2h 15m", "Airbus A320neo",
        290.0, 25, 8, 4
    );

    // Pre-populate with realistic Indian passengers
    auto bookSample = [this](const std::string& flNo, const std::string& name,
                             const std::string& email, const std::string& phone,
                             const std::string& passId, int age, const std::string& gender,
                             const std::string& seatClassType, const std::string& reqSeat,
                             bool isCancelled = false) {
        if (flights.find(flNo) == flights.end()) return;
        auto& fl = flights[flNo];
        auto seatObj = SeatClass::create(seatClassType);
        std::string seatNum = fl.allocateSeat(seatClassType, reqSeat);
        double fare = seatObj->calculateFare(fl.getBasePrice());
        std::string tId = "TKT-" + std::to_string(nextTicketSequence++);
        Passenger p(name, email, phone, passId, age, gender);
        Ticket t(tId, flNo, p, seatObj, seatNum, "2026-11-10 10:30", fare);
        if (isCancelled) {
            fl.releaseSeat(seatClassType, seatNum);
            t.cancelTicket();
        }
        tickets[tId] = t;
    };

    // Confirmed bookings matching exact available counts:
    // SK101: 1 Economy booked -> 29 Available (Economy: 30), 10 Business Avail, 4 First Class Avail
    bookSample("SK101", "Aarav Sharma", "aarav.sharma@gmail.com", "+91 98765 43210", "J8234109", 29, "Male", "Economy", "7A");

    // SK303: 1 Economy booked -> 24 Available (Economy: 25), 8 Business Avail, 4 First Class Avail
    bookSample("SK303", "Priya Patel", "priya.patel@gmail.com", "+91 94401 23456", "M4591023", 27, "Female", "Economy", "7A");

    // SK404: 1 Economy booked -> 24 Available (Economy: 25), 8 Business Avail, 4 First Class Avail
    bookSample("SK404", "Rohan Iyer", "rohan.iyer@gmail.com", "+91 81234 56789", "K7612349", 34, "Male", "Economy", "7A");

    // Cancelled demo bookings (seat released back to flight, demonstrating Business & First Class cancellation policies)
    bookSample("SK202", "Ananya Reddy", "ananya.reddy@gmail.com", "+91 98230 11223", "P3198421", 31, "Female", "Business", "3A", true);
    bookSample("SK101", "Vikram Verma", "vikram.verma@gmail.com", "+91 97112 34567", "R9081234", 42, "Male", "First Class", "1A", true);
}

void AirlineManager::resetData() {
    initializeSampleData();
    saveToFiles();
}

std::string AirlineManager::getAllFlightsJson() const {
    std::ostringstream ss;
    ss << "[\n";
    bool first = true;
    for (const auto& pair : flights) {
        if (!first) ss << ",\n";
        first = false;
        const auto& f = pair.second;
        ss << "  {\n"
           << "    \"flightNumber\": \"" << JsonUtils::escapeJson(f.getFlightNumber()) << "\",\n"
           << "    \"airlineName\": \"" << JsonUtils::escapeJson(f.getAirlineName()) << "\",\n"
           << "    \"origin\": \"" << JsonUtils::escapeJson(f.getOrigin()) << "\",\n"
           << "    \"originCode\": \"" << JsonUtils::escapeJson(f.getOriginCode()) << "\",\n"
           << "    \"destination\": \"" << JsonUtils::escapeJson(f.getDestination()) << "\",\n"
           << "    \"destinationCode\": \"" << JsonUtils::escapeJson(f.getDestinationCode()) << "\",\n"
           << "    \"departureTime\": \"" << JsonUtils::escapeJson(f.getDepartureTime()) << "\",\n"
           << "    \"arrivalTime\": \"" << JsonUtils::escapeJson(f.getArrivalTime()) << "\",\n"
           << "    \"duration\": \"" << JsonUtils::escapeJson(f.getDuration()) << "\",\n"
           << "    \"aircraft\": \"" << JsonUtils::escapeJson(f.getAircraft()) << "\",\n"
           << "    \"basePrice\": " << std::fixed << std::setprecision(2) << f.getBasePrice() << ",\n"
           << "    \"economy\": {\n"
           << "      \"price\": " << f.getPriceForClass("Economy") << ",\n"
           << "      \"totalSeats\": " << f.getCapacity("Economy") << ",\n"
           << "      \"availableSeats\": " << f.getAvailableSeats("Economy") << ",\n"
           << "      \"bookedSeats\": " << f.getBookedCount("Economy") << "\n"
           << "    },\n"
           << "    \"business\": {\n"
           << "      \"price\": " << f.getPriceForClass("Business") << ",\n"
           << "      \"totalSeats\": " << f.getCapacity("Business") << ",\n"
           << "      \"availableSeats\": " << f.getAvailableSeats("Business") << ",\n"
           << "      \"bookedSeats\": " << f.getBookedCount("Business") << "\n"
           << "    },\n"
           << "    \"firstClass\": {\n"
           << "      \"price\": " << f.getPriceForClass("First Class") << ",\n"
           << "      \"totalSeats\": " << f.getCapacity("First Class") << ",\n"
           << "      \"availableSeats\": " << f.getAvailableSeats("First Class") << ",\n"
           << "      \"bookedSeats\": " << f.getBookedCount("First Class") << "\n"
           << "    }\n"
           << "  }";
    }
    ss << "\n]";
    return ss.str();
}

std::string AirlineManager::getFlightJson(const std::string& flightNo) const {
    auto it = flights.find(flightNo);
    if (it == flights.end()) {
        return "{\"error\": \"Flight not found\"}";
    }
    const auto& f = it->second;
    std::ostringstream ss;
    ss << "{\n"
       << "  \"flightNumber\": \"" << JsonUtils::escapeJson(f.getFlightNumber()) << "\",\n"
       << "  \"airlineName\": \"" << JsonUtils::escapeJson(f.getAirlineName()) << "\",\n"
       << "  \"origin\": \"" << JsonUtils::escapeJson(f.getOrigin()) << "\",\n"
       << "  \"destination\": \"" << JsonUtils::escapeJson(f.getDestination()) << "\",\n"
       << "  \"departureTime\": \"" << JsonUtils::escapeJson(f.getDepartureTime()) << "\",\n"
       << "  \"arrivalTime\": \"" << JsonUtils::escapeJson(f.getArrivalTime()) << "\",\n"
       << "  \"duration\": \"" << JsonUtils::escapeJson(f.getDuration()) << "\",\n"
       << "  \"aircraft\": \"" << JsonUtils::escapeJson(f.getAircraft()) << "\",\n"
       << "  \"basePrice\": " << f.getBasePrice() << ",\n"
       << "  \"economyAvailable\": " << f.getAvailableSeats("Economy") << ",\n"
       << "  \"businessAvailable\": " << f.getAvailableSeats("Business") << ",\n"
       << "  \"firstClassAvailable\": " << f.getAvailableSeats("First Class") << "\n"
       << "}";
    return ss.str();
}

std::string AirlineManager::getClassPerksJson() const {
    std::vector<std::shared_ptr<SeatClass>> classes = {
        std::make_shared<EconomySeat>(),
        std::make_shared<BusinessSeat>(),
        std::make_shared<FirstClassSeat>()
    };

    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < classes.size(); ++i) {
        if (i > 0) ss << ",\n";
        const auto& c = classes[i];
        ss << "  {\n"
           << "    \"className\": \"" << JsonUtils::escapeJson(c->getClassName()) << "\",\n"
           << "    \"priceMultiplier\": " << c->getPriceMultiplier() << ",\n"
           << "    \"baggageKg\": " << c->getBaggageAllowanceKg() << ",\n"
           << "    \"refundPercentage\": " << (c->getRefundPercentage() * 100.0) << ",\n"
           << "    \"mealService\": \"" << JsonUtils::escapeJson(c->getMealService()) << "\",\n"
           << "    \"loungeAccess\": \"" << JsonUtils::escapeJson(c->getLoungeAccess()) << "\",\n"
           << "    \"seatType\": \"" << JsonUtils::escapeJson(c->getSeatType()) << "\",\n"
           << "    \"perks\": [";
        auto perks = c->getPerks();
        for (size_t p = 0; p < perks.size(); ++p) {
            if (p > 0) ss << ", ";
            ss << "\"" << JsonUtils::escapeJson(perks[p]) << "\"";
        }
        ss << "]\n  }";
    }
    ss << "\n]";
    return ss.str();
}

std::string AirlineManager::bookTicket(const std::string& flightNo, const Passenger& passenger,
                                       const std::string& seatClassType, const std::string& requestedSeat) {
    if (flights.find(flightNo) == flights.end()) {
        return "{\"success\": false, \"message\": \"Error: Flight " + flightNo + " does not exist.\"}";
    }

    auto& flight = flights[flightNo];
    if (!flight.isSeatAvailable(seatClassType)) {
        return "{\"success\": false, \"message\": \"Error: No available seats left in " + seatClassType + " class for this flight.\"}";
    }

    std::string allocatedSeat = flight.allocateSeat(seatClassType, requestedSeat);
    if (allocatedSeat.empty()) {
        return "{\"success\": false, \"message\": \"Error: Unable to allocate seat. Requested seat may be occupied.\"}";
    }

    // Polymorphic Seat Class Creation & Fare Calculation
    auto seatObj = SeatClass::create(seatClassType);
    double fare = seatObj->calculateFare(flight.getBasePrice());
    std::string tId = generateTicketId();
    std::string bookingDate = getCurrentDateTime();

    Ticket newTicket(tId, flightNo, passenger, seatObj, allocatedSeat, bookingDate, fare);
    tickets[tId] = newTicket;

    saveToFiles();

    // Build success response with ticket details
    std::ostringstream ss;
    ss << "{\n"
       << "  \"success\": true,\n"
       << "  \"message\": \"Ticket booked successfully!\",\n"
       << "  \"ticket\": {\n"
       << "    \"ticketId\": \"" << JsonUtils::escapeJson(tId) << "\",\n"
       << "    \"flightNumber\": \"" << JsonUtils::escapeJson(flightNo) << "\",\n"
       << "    \"airlineName\": \"" << JsonUtils::escapeJson(flight.getAirlineName()) << "\",\n"
       << "    \"origin\": \"" << JsonUtils::escapeJson(flight.getOrigin()) << "\",\n"
       << "    \"destination\": \"" << JsonUtils::escapeJson(flight.getDestination()) << "\",\n"
       << "    \"departureTime\": \"" << JsonUtils::escapeJson(flight.getDepartureTime()) << "\",\n"
       << "    \"passengerName\": \"" << JsonUtils::escapeJson(passenger.getFullName()) << "\",\n"
       << "    \"passportNumber\": \"" << JsonUtils::escapeJson(passenger.getPassportNumber()) << "\",\n"
       << "    \"seatClass\": \"" << JsonUtils::escapeJson(seatObj->getClassName()) << "\",\n"
       << "    \"seatNumber\": \"" << JsonUtils::escapeJson(allocatedSeat) << "\",\n"
       << "    \"farePaid\": " << std::fixed << std::setprecision(2) << fare << ",\n"
       << "    \"baggageAllowanceKg\": " << seatObj->getBaggageAllowanceKg() << ",\n"
       << "    \"mealService\": \"" << JsonUtils::escapeJson(seatObj->getMealService()) << "\",\n"
       << "    \"loungeAccess\": \"" << JsonUtils::escapeJson(seatObj->getLoungeAccess()) << "\",\n"
       << "    \"bookingDate\": \"" << JsonUtils::escapeJson(bookingDate) << "\",\n"
       << "    \"status\": \"CONFIRMED\"\n"
       << "  }\n"
       << "}";
    return ss.str();
}

std::string AirlineManager::cancelTicket(const std::string& ticketId) {
    auto it = tickets.find(ticketId);
    if (it == tickets.end()) {
        return "{\"success\": false, \"message\": \"Error: Ticket ID not found.\"}";
    }

    Ticket& tkt = it->second;
    if (!tkt.isActive()) {
        return "{\"success\": false, \"message\": \"Error: Ticket is already cancelled.\"}";
    }

    // Free the seat from the flight
    std::string flNo = tkt.getFlightNumber();
    if (flights.find(flNo) != flights.end()) {
        flights[flNo].releaseSeat(tkt.getSeatClass()->getClassName(), tkt.getSeatNumber());
    }

    // Cancel ticket polymorphically calculating refund
    tkt.cancelTicket();
    saveToFiles();

    std::ostringstream ss;
    ss << "{\n"
       << "  \"success\": true,\n"
       << "  \"message\": \"Ticket successfully cancelled.\",\n"
       << "  \"ticketId\": \"" << JsonUtils::escapeJson(ticketId) << "\",\n"
       << "  \"farePaid\": " << std::fixed << std::setprecision(2) << tkt.getFarePaid() << ",\n"
       << "  \"refundRate\": " << (tkt.getSeatClass()->getRefundPercentage() * 100.0) << ",\n"
       << "  \"refundAmount\": " << std::fixed << std::setprecision(2) << tkt.getRefundAmount() << ",\n"
       << "  \"status\": \"CANCELLED\"\n"
       << "}";
    return ss.str();
}

std::string AirlineManager::getPassengersForFlightJson(const std::string& flightNo) const {
    std::ostringstream ss;
    ss << "[\n";
    bool first = true;
    for (const auto& pair : tickets) {
        const auto& t = pair.second;
        if (t.getFlightNumber() == flightNo && t.isActive()) {
            if (!first) ss << ",\n";
            first = false;
            auto p = t.getPassenger();
            auto s = t.getSeatClass();
            ss << "  {\n"
               << "    \"ticketId\": \"" << JsonUtils::escapeJson(t.getTicketId()) << "\",\n"
               << "    \"passengerName\": \"" << JsonUtils::escapeJson(p.getFullName()) << "\",\n"
               << "    \"email\": \"" << JsonUtils::escapeJson(p.getEmail()) << "\",\n"
               << "    \"phone\": \"" << JsonUtils::escapeJson(p.getPhone()) << "\",\n"
               << "    \"passport\": \"" << JsonUtils::escapeJson(p.getPassportNumber()) << "\",\n"
               << "    \"age\": " << p.getAge() << ",\n"
               << "    \"gender\": \"" << JsonUtils::escapeJson(p.getGender()) << "\",\n"
               << "    \"seatClass\": \"" << JsonUtils::escapeJson(s ? s->getClassName() : "Unknown") << "\",\n"
               << "    \"seatNumber\": \"" << JsonUtils::escapeJson(t.getSeatNumber()) << "\",\n"
               << "    \"fare\": " << std::fixed << std::setprecision(2) << t.getFarePaid() << ",\n"
               << "    \"bookingDate\": \"" << JsonUtils::escapeJson(t.getBookingDate()) << "\"\n"
               << "  }";
        }
    }
    ss << "\n]";
    return ss.str();
}

std::string AirlineManager::getAllBookingsJson() const {
    std::ostringstream ss;
    ss << "[\n";
    bool first = true;
    for (const auto& pair : tickets) {
        const auto& t = pair.second;
        auto p = t.getPassenger();
        auto s = t.getSeatClass();
        std::string flOrigin = "", flDest = "", airline = "";
        auto flIt = flights.find(t.getFlightNumber());
        if (flIt != flights.end()) {
            flOrigin = flIt->second.getOrigin();
            flDest = flIt->second.getDestination();
            airline = flIt->second.getAirlineName();
        }

        ss << "  {\n"
           << "    \"ticketId\": \"" << JsonUtils::escapeJson(t.getTicketId()) << "\",\n"
           << "    \"flightNumber\": \"" << JsonUtils::escapeJson(t.getFlightNumber()) << "\",\n"
           << "    \"airlineName\": \"" << JsonUtils::escapeJson(airline) << "\",\n"
           << "    \"origin\": \"" << JsonUtils::escapeJson(flOrigin) << "\",\n"
           << "    \"destination\": \"" << JsonUtils::escapeJson(flDest) << "\",\n"
           << "    \"passengerName\": \"" << JsonUtils::escapeJson(p.getFullName()) << "\",\n"
           << "    \"email\": \"" << JsonUtils::escapeJson(p.getEmail()) << "\",\n"
           << "    \"phone\": \"" << JsonUtils::escapeJson(p.getPhone()) << "\",\n"
           << "    \"passport\": \"" << JsonUtils::escapeJson(p.getPassportNumber()) << "\",\n"
           << "    \"age\": " << p.getAge() << ",\n"
           << "    \"gender\": \"" << JsonUtils::escapeJson(p.getGender()) << "\",\n"
           << "    \"seatClass\": \"" << JsonUtils::escapeJson(s ? s->getClassName() : "Unknown") << "\",\n"
           << "    \"seatNumber\": \"" << JsonUtils::escapeJson(t.getSeatNumber()) << "\",\n"
           << "    \"farePaid\": " << std::fixed << std::setprecision(2) << t.getFarePaid() << ",\n"
           << "    \"refundAmount\": " << std::fixed << std::setprecision(2) << t.getRefundAmount() << ",\n"
           << "    \"bookingDate\": \"" << JsonUtils::escapeJson(t.getBookingDate()) << "\",\n"
           << "    \"status\": \"" << JsonUtils::escapeJson(t.getStatus()) << "\"\n"
           << "  }";
    }
    ss << "\n]";
    return ss.str();
}

std::string AirlineManager::getBookingByTicketIdJson(const std::string& ticketId) const {
    auto it = tickets.find(ticketId);
    if (it == tickets.end()) {
        return "{\"found\": false, \"message\": \"Ticket not found.\"}";
    }

    const auto& t = it->second;
    auto p = t.getPassenger();
    auto s = t.getSeatClass();
    std::string origin = "", dest = "", airline = "", depTime = "";
    auto flIt = flights.find(t.getFlightNumber());
    if (flIt != flights.end()) {
        origin = flIt->second.getOrigin();
        dest = flIt->second.getDestination();
        airline = flIt->second.getAirlineName();
        depTime = flIt->second.getDepartureTime();
    }

    std::ostringstream ss;
    ss << "{\n"
       << "  \"found\": true,\n"
       << "  \"ticketId\": \"" << JsonUtils::escapeJson(t.getTicketId()) << "\",\n"
       << "  \"flightNumber\": \"" << JsonUtils::escapeJson(t.getFlightNumber()) << "\",\n"
       << "  \"airlineName\": \"" << JsonUtils::escapeJson(airline) << "\",\n"
       << "  \"origin\": \"" << JsonUtils::escapeJson(origin) << "\",\n"
       << "  \"destination\": \"" << JsonUtils::escapeJson(dest) << "\",\n"
       << "  \"departureTime\": \"" << JsonUtils::escapeJson(depTime) << "\",\n"
       << "  \"passengerName\": \"" << JsonUtils::escapeJson(p.getFullName()) << "\",\n"
       << "  \"email\": \"" << JsonUtils::escapeJson(p.getEmail()) << "\",\n"
       << "  \"phone\": \"" << JsonUtils::escapeJson(p.getPhone()) << "\",\n"
       << "  \"passport\": \"" << JsonUtils::escapeJson(p.getPassportNumber()) << "\",\n"
       << "  \"seatClass\": \"" << JsonUtils::escapeJson(s ? s->getClassName() : "Unknown") << "\",\n"
       << "  \"seatNumber\": \"" << JsonUtils::escapeJson(t.getSeatNumber()) << "\",\n"
       << "  \"farePaid\": " << std::fixed << std::setprecision(2) << t.getFarePaid() << ",\n"
       << "  \"refundAmount\": " << std::fixed << std::setprecision(2) << t.getRefundAmount() << ",\n"
       << "  \"baggageKg\": " << (s ? s->getBaggageAllowanceKg() : 20) << ",\n"
       << "  \"mealService\": \"" << (s ? JsonUtils::escapeJson(s->getMealService()) : "") << "\",\n"
       << "  \"bookingDate\": \"" << JsonUtils::escapeJson(t.getBookingDate()) << "\",\n"
       << "  \"status\": \"" << JsonUtils::escapeJson(t.getStatus()) << "\"\n"
       << "}";
    return ss.str();
}

std::string AirlineManager::getSystemStatsJson() const {
    int totalFlights = static_cast<int>(flights.size());
    int activeBookings = 0;
    int cancelledBookings = 0;
    double totalRevenue = 0.0;
    int econBooked = 0, bizBooked = 0, firstBooked = 0;

    for (const auto& pair : tickets) {
        const auto& t = pair.second;
        if (t.isActive()) {
            activeBookings++;
            totalRevenue += t.getFarePaid();
            std::string cName = t.getSeatClass() ? t.getSeatClass()->getClassName() : "";
            if (cName == "First Class") firstBooked++;
            else if (cName == "Business") bizBooked++;
            else econBooked++;
        } else {
            cancelledBookings++;
            totalRevenue += (t.getFarePaid() - t.getRefundAmount()); // airline retains cancellation fee
        }
    }

    std::ostringstream ss;
    ss << "{\n"
       << "  \"totalFlights\": " << totalFlights << ",\n"
       << "  \"activeBookings\": " << activeBookings << ",\n"
       << "  \"cancelledBookings\": " << cancelledBookings << ",\n"
       << "  \"totalRevenue\": " << std::fixed << std::setprecision(2) << totalRevenue << ",\n"
       << "  \"economyCount\": " << econBooked << ",\n"
       << "  \"businessCount\": " << bizBooked << ",\n"
       << "  \"firstClassCount\": " << firstBooked << "\n"
       << "}";
    return ss.str();
}

void AirlineManager::saveToFiles() {
    std::ofstream bFile("data/bookings.json");
    if (bFile.is_open()) {
        bFile << getAllBookingsJson();
        bFile.close();
    }

    // Automatically synchronize Excel workbook (.xlsx) and XML spreadsheet (.xml)
    ExcelExporter::exportToXlsx("data/airline_bookings.xlsx", tickets, flights);
    ExcelExporter::exportToXmlSpreadsheet("data/airline_bookings.xml", tickets, flights);
}

void AirlineManager::loadFromFiles() {
    std::ifstream bFile("data/bookings.json");
    if (!bFile.is_open()) {
        saveToFiles();
        return;
    }

    std::ostringstream ss;
    ss << bFile.rdbuf();
    std::string content = ss.str();
    bFile.close();

    if (content.empty()) return;

    // Reset current bookings and re-populate from file
    tickets.clear();
    for (auto& pair : flights) {
        // Re-initialize capacities
        pair.second = Flight(
            pair.second.getFlightNumber(), pair.second.getAirlineName(),
            pair.second.getOrigin(), pair.second.getOriginCode(),
            pair.second.getDestination(), pair.second.getDestinationCode(),
            pair.second.getDepartureTime(), pair.second.getArrivalTime(),
            pair.second.getDuration(), pair.second.getAircraft(),
            pair.second.getBasePrice(),
            pair.second.getCapacity("Economy"),
            pair.second.getCapacity("Business"),
            pair.second.getCapacity("First Class")
        );
    }

    size_t pos = 0;
    while ((pos = content.find('{', pos)) != std::string::npos) {
        size_t endPos = content.find('}', pos);
        if (endPos == std::string::npos) break;

        std::string objStr = content.substr(pos, endPos - pos + 1);
        std::string tId = JsonUtils::getString(objStr, "ticketId");
        std::string flNo = JsonUtils::getString(objStr, "flightNumber");
        std::string pName = JsonUtils::getString(objStr, "passengerName");
        std::string email = JsonUtils::getString(objStr, "email");
        std::string phone = JsonUtils::getString(objStr, "phone");
        std::string passNo = JsonUtils::getString(objStr, "passport");
        int age = JsonUtils::getInt(objStr, "age", 30);
        std::string gender = JsonUtils::getString(objStr, "gender", "Other");
        std::string sClass = JsonUtils::getString(objStr, "seatClass", "Economy");
        std::string sNum = JsonUtils::getString(objStr, "seatNumber");
        double fare = JsonUtils::getDouble(objStr, "farePaid", 0.0);
        std::string bDate = JsonUtils::getString(objStr, "bookingDate");
        std::string status = JsonUtils::getString(objStr, "status", "CONFIRMED");

        if (!tId.empty() && !flNo.empty()) {
            auto seatObj = SeatClass::create(sClass);
            Passenger p(pName, email, phone, passNo, age, gender);
            Ticket t(tId, flNo, p, seatObj, sNum, bDate, fare);

            if (status == "CANCELLED") {
                t.cancelTicket();
            } else {
                if (flights.find(flNo) != flights.end()) {
                    flights[flNo].allocateSeat(sClass, sNum);
                }
            }

            tickets[tId] = t;

            // Track max ticket sequence
            if (tId.find("TKT-") == 0) {
                try {
                    int seq = std::stoi(tId.substr(4));
                    if (seq >= nextTicketSequence) {
                        nextTicketSequence = seq + 1;
                    }
                } catch (...) {}
            }
        }

        pos = endPos + 1;
    }

    saveToFiles();
}
