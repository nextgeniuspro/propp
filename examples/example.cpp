#include "propp/Property.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace propp;

class Person {
public:    
   PropertyRO<std::string> Name;
   PropertyRWSMT<int> Age;
   PropertyROG<std::string, GetterTypeValue<std::string>> Address; // We declare getter that returns string by value
    
    Person(const std::string& name)
       : Name(name)
       , Age(0, std::bind(&Person::SetAge, this, std::placeholders::_1))
       , Address("123 Main St Mega City MS 12345", std::bind(&Person::GetAddress, this))
    {
    }
    
    // For properties with custom getter and setter, we need to define copy constructor
    Person(const Person& other)
        : Name(other.Name())
        , Age(other.Age(), std::bind(&Person::SetAge, this, std::placeholders::_1))
        , Address(other.Address.GetRaw(), std::bind(&Person::GetAddress, this)) // Because we declared getter that returns string by value, we need to use GetRaw() to get the initial value
    {
    }

private:    
    void SetAge(int value) {
        // Don't worry, inside setter you can assign value back with no recursion issue (if you do this in the same thread)
        Age = std::clamp(value, 0, 150);
    }

    // This getter returns initial address from property and appends predefined country to it
    std::string GetAddress() {
        static std::string predefinedCountry = " USA";

        // Don't worry that we call Address() inside getter, it won't cause recursion issue, next call will return the underlying value
        return Address() + predefinedCountry;
    }
};

class Office {
public:
    PropertyRW< std::vector<Person> > Persons;

    void Print() 
    {
        for (const Person& p : Persons()) {
            printf("Person: `%s`, `%d`, `%s`\n", p.Name().c_str(), p.Age(), p.Address().c_str());
        }
    }
};


int main()
{
    Office office;
    office.Persons().push_back(Person("Alice"));

    Person john("John");
    // obj.Name = "Alice"; // Won't compile because `Name` is read-only property
    john.Age = 200; // Will be clamped to 150 by setter
    office.Persons().push_back(john);

    // Multithreading test, 50/50 chance that Age will be 50 or 100
    Person john2(john); // Copy constructor will be called

    std::thread t1([&john2](){
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
        john2.Age = 50;
    });
    std::thread t2([&john2](){
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
        john2.Age = 100;
    });
    t1.join();
    t2.join();

    office.Persons().push_back(john2);
    
    office.Print();
    
	return 0;
}

