# ✈️ SkyWings Airline Reservation & Ticketing Engine
### C++ End-Semester Project with Pure C++ Web Backend & Modern Frontend

![C++](https://img.shields.io/badge/Language-C%2B%2B11%20%2F%20C%2B%2B14-00599C?logo=c%2B%2B)
![Backend](https://img.shields.io/badge/Backend-100%25%20Pure%20C%2B%2B%20Winsock-blue)
![Frontend](https://img.shields.io/badge/Frontend-HTML5%20%2F%20CSS3%20%2F%20Vanilla%20JS-orange)
![Platform](https://img.shields.io/badge/Platform-Windows%20(MinGW%20%2F%20GCC)-informational)

---

## 📌 Project Overview
The **SkyWings Airline Reservation System** is an end-to-end flight booking and operations management platform developed as a semester project. The backend is written **exclusively in standard C++** using low-level Windows Sockets (`winsock2.h`), serving both a responsive modern web application and full JSON REST API endpoints without any third-party framework dependencies.

The core of the system strongly demonstrates **Object-Oriented Programming (OOP) concepts in C++**, specifically **Inheritance, Dynamic Runtime Polymorphism, Abstract Base Classes, Encapsulation, and Smart Pointers**.

---

## 🏛️ Object-Oriented Design & C++ Principles

### 1. Abstract Base Class & Inheritance Hierarchy
Seat pricing, perks, baggage allowance, and cancellation refund percentages vary fundamentally by travel class. This is modeled using an abstract base class `SeatClass` and three derived classes:

```
                  ┌───────────────────────────────┐
                  │    SeatClass (Abstract Base)  │
                  ├───────────────────────────────┤
                  │ # className: string           │
                  │ # priceMultiplier: double     │
                  │ # baggageAllowanceKg: int     │
                  │ # refundPercentage: double    │
                  ├───────────────────────────────┤
                  │ + calculateFare()* = 0        │
                  │ + getMealService()* = 0       │
                  │ + getLoungeAccess()* = 0      │
                  │ + getRefundPercentage()       │
                  │ + getPerks()* = 0             │
                  └───────────────┬───────────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         ▼                        ▼                        ▼
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│   EconomySeat    │    │   BusinessSeat   │    │  FirstClassSeat  │
├──────────────────┤    ├──────────────────┤    ├──────────────────┤
│ Multiplier: 1.0x │    │ Multiplier: 2.2x │    │ Multiplier: 3.8x │
│ Baggage: 20 kg   │    │ Baggage: 40 kg   │    │ Baggage: 60 kg   │
│ Refund: 70%      │    │ Refund: 85%      │    │ Refund: 100%     │
│ Snack & Drink    │    │ 3-Course Dining  │    │ Michelin Dining  │
│ No Lounge        │    │ Business Lounge  │    │ Royal VIP Lounge │
└──────────────────┘    └──────────────────┘    └──────────────────┘
```

### 2. Runtime Polymorphism in Action
When a ticket is booked or cancelled, the system interacts exclusively with the abstract base class pointer `std::shared_ptr<SeatClass>`. The C++ compiler resolves the method call dynamically through the virtual table (**vtable**):

- **Polymorphic Fare Calculation:**
  ```cpp
  // Dynamic runtime resolution based on whether seatClass is Economy, Business, or First Class
  double fare = seatClass->calculateFare(flight.getBasePrice());
  ```
- **Polymorphic Refund Calculation on Cancellation:**
  ```cpp
  bool Ticket::cancelTicket() {
      status = "CANCELLED";
      // Each class yields its own refund rate (70%, 85%, or 100%) dynamically
      refundAmount = farePaid * seatClass->getRefundPercentage();
      return true;
  }
  ```

### 3. Factory Design Pattern
`SeatClass::create(typeName)` creates and returns the appropriate derived instance wrapped in a `std::shared_ptr<SeatClass>`, centralizing instantiation and decoupling business logic.

---

## 🚀 Key System Features

1. **✈️ Real-time Flight Search & Schedules:**
   - Domestic routes across India: SK101 (Hyderabad ⇄ Delhi), SK202 (Hyderabad ⇄ Mumbai), SK303 (Chennai ⇄ Bengaluru), SK404 (Delhi ⇄ Kolkata).
   - Real-time seat availability per class (Economy, Business, First Class).
   - Instant search filtering by destination, origin airport, or flight number.

2. **🎫 Multi-Class Booking Engine:**
   - Passenger details capture (Legal Name, Passport/Gov ID, Contact, Age, Gender).
   - Seat preference selection or automatic sequential row allocation:
     - First Class: Rows 1–2 (Seats A, B, E, F)
     - Business Class: Rows 3–6 (Seats A, B, C, D, E, F)
     - Economy Class: Rows 7–20 (Seats A, B, C, D, E, F)
   - Live interactive price breakdown updating dynamically as class or route changes.

3. **🖨️ Verified Boarding Pass Generation:**
   - Modal popup with barcode preview, route airports, flight number, seat number, and baggage allowance.
   - Built-in one-click printing support.

4. **👥 Passenger Manifest Inspector:**
   - Filterable by flight to inspect all active passengers onboard.
   - Real-time updates as soon as tickets are booked or cancelled.

5. **❌ Ticket Management & Cancellation Engine:**
   - Instant booking lookup via reference number (e.g. `TKT-1001`).
   - Displays estimated refund calculated via polymorphic class percentage.
   - Re-releases seat back into the flight inventory automatically.

6. **📊 Executive Dashboard & Stats:**
   - Live revenue tracking, total active bookings, cancellations, and class distribution percentages.

7. **💾 Persistent Storage & Excel Integration:**
   - JSON persistence across restarts in `data/bookings.json`.
   - **Automatic Excel Synchronizer (`data/airline_bookings.xlsx` and `.xml`):**
     - Automatically updates on every new booking or cancellation.
     - **Sheet 1: Booking Details** (Ticket ID, Flight ID, Airline, Origin, Destination, Passenger Name, Class, Seat, Fare Paid, Refund Amount, Booking Date, Status).
     - **Sheet 2: Passenger List** (Passenger ID / Ticket ID, Passenger Name, Age, Gender, Email, Phone, Passport Number, Flight ID, Seat Number, Seat Class, Booking Status).
     - Cancelled records are preserved with updated cancellation status and refund amounts.
     - 100% pure C++ implementation with zero external library dependencies.

---

## 📁 Project Structure

```
c:\Users\prane\Documents\programing\Cpp project\
├── include/
│   ├── SeatClass.h         # Base SeatClass & derived Economy, Business, First Class
│   ├── Passenger.h         # Passenger model
│   ├── Ticket.h            # Ticket model holding polymorphic SeatClass pointer
│   ├── Flight.h            # Flight inventory, seating capacities & allocations
│   ├── AirlineManager.h    # Core business engine & JSON file persistence
│   ├── ExcelExporter.h     # Native C++ multi-sheet Excel (.xlsx / .xml) exporter
│   ├── HttpServer.h        # Pure C++ Winsock2 HTTP server
│   └── JsonUtils.h         # Zero-dependency lightweight C++ JSON parser & serializer
├── src/
│   ├── SeatClass.cpp       # Class implementations & perks
│   ├── Passenger.cpp       # Passenger methods
│   ├── Ticket.cpp          # Polymorphic ticket cancellation & fare logic
│   ├── Flight.cpp          # Seat allocation & capacity checks
│   ├── AirlineManager.cpp  # Sample routes, booking transactions, stats
│   ├── ExcelExporter.cpp   # OpenXML ZIP builder & XMLSS spreadsheet generator
│   ├── HttpServer.cpp      # Sockets, static file server & REST routing
│   └── main.cpp            # Server initialization & entry point
├── public/                 # Static web files served directly by pure C++ backend
│   ├── index.html          # Responsive single-page application UI
│   ├── styles.css          # Modern dark/light airline theme styling
│   └── app.js              # Vanilla JavaScript connecting to C++ REST endpoints
├── data/
│   ├── bookings.json       # Persistent booking records
│   ├── airline_bookings.xlsx # Multi-sheet Excel workbook (Booking Details + Passenger List)
│   └── airline_bookings.xml  # Excel XML spreadsheet format
├── build.bat               # 1-Click build script
├── run.bat                 # 1-Click run script (builds, starts server & opens browser)
└── README.md               # Project documentation
```

---

## 🛠️ How to Build and Run

### Prerequisites
- Windows OS (10 or 11)
- MinGW / GCC C++ compiler (`g++`) with C++11 support

### Method 1: Easy One-Click Batch Script (Recommended)
Simply double-click or run:
```cmd
run.bat
```
This automatically compiles `airline_server.exe`, launches the pure C++ server, and opens `http://localhost:8080` in your default browser.

### Method 2: Manual Compilation via Command Prompt / PowerShell
To build:
```cmd
g++ -std=c++11 -O2 src/SeatClass.cpp src/Passenger.cpp src/Ticket.cpp src/Flight.cpp src/AirlineManager.cpp src/HttpServer.cpp src/main.cpp -o airline_server.exe -lws2_32
```

To run:
```cmd
.\airline_server.exe
```
Then visit **`http://localhost:8080`** in your browser.

---

## 📡 REST API Endpoints (Pure C++ Backend)

| HTTP Method | Endpoint | Description |
|:---|:---|:---|
| `GET` | `/` or `/index.html` | Serves the single-page frontend application |
| `GET` | `/api/flights` | Returns all available flight routes and seat availability |
| `GET` | `/api/flights/{id}` | Returns flight details for a specific flight number |
| `GET` | `/api/flights/{id}/passengers` | Returns passenger manifest for a specific flight |
| `GET` | `/api/perks` | Returns polymorphic perks and multipliers for all 3 classes |
| `GET` | `/api/bookings` | Returns all bookings in the system |
| `GET` | `/api/bookings/{ticketId}` | Looks up a specific booking by Ticket ID |
| `GET` | `/api/stats` | Returns real-time airline performance & revenue metrics |
| `POST` | `/api/book` | Books a ticket with passenger details & seat class |
| `POST` | `/api/cancel` | Cancels a ticket and executes polymorphic refund calculation |
| `POST` | `/api/reset` | Resets the system back to the initial sample state |

---

## 🎓 Viva / Presentation Defense Guide (For Evaluators)

When demonstrating this project to professors, emphasize the following points:

1. **Why Pure C++ in the Backend?**
   - The backend is implemented without Node.js, Python, or external frameworks like Crow or Boost. It demonstrates core systems programming, socket creation, TCP connection handling, and HTTP protocol parsing using native C++ and Winsock (`ws2_32`).

2. **Where is Inheritance used?**
   - In `include/SeatClass.h`: `EconomySeat`, `BusinessSeat`, and `FirstClassSeat` inherit publicly from `SeatClass`.

3. **Where is Polymorphism demonstrated?**
   - In `src/Ticket.cpp`: The `Ticket` class doesn't check `if (type == "Economy")`. Instead, it calls `seatClass->getRefundPercentage()` and `seatClass->calculateFare()`. The C++ runtime performs dynamic dispatch to execute the correct derived method.

4. **How are memory leaks avoided?**
   - Modern C++ smart pointers (`std::shared_ptr<SeatClass>`) are used, ensuring deterministic reference-counted memory cleanup.

