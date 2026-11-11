#ifndef PASSENGER_H
#define PASSENGER_H

#include <string>

/**
 * @brief Represents a Passenger booking a flight ticket.
 */
class Passenger {
private:
    std::string fullName;
    std::string email;
    std::string phone;
    std::string passportNumber;
    int age;
    std::string gender;

public:
    Passenger();
    Passenger(const std::string& name, const std::string& email, const std::string& phone,
              const std::string& passport, int age, const std::string& gender);

    // Getters
    std::string getFullName() const { return fullName; }
    std::string getEmail() const { return email; }
    std::string getPhone() const { return phone; }
    std::string getPassportNumber() const { return passportNumber; }
    int getAge() const { return age; }
    std::string getGender() const { return gender; }

    // Setters
    void setFullName(const std::string& name) { fullName = name; }
    void setEmail(const std::string& mail) { email = mail; }
    void setPhone(const std::string& ph) { phone = ph; }
    void setPassportNumber(const std::string& pass) { passportNumber = pass; }
    void setAge(int a) { age = a; }
    void setGender(const std::string& g) { gender = g; }
};

#endif // PASSENGER_H

