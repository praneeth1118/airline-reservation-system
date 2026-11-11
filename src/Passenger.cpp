#include "../include/Passenger.h"

Passenger::Passenger()
    : fullName(""), email(""), phone(""), passportNumber(""), age(0), gender("Unspecified") {}

Passenger::Passenger(const std::string& name, const std::string& email, const std::string& phone,
                     const std::string& passport, int age, const std::string& gender)
    : fullName(name), email(email), phone(phone), passportNumber(passport), age(age), gender(gender) {}

