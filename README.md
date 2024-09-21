# propp
Propp is a lightweight, header-only C++ library that brings an efficient and flexible property system to C++. Unlike traditional getter and setter methods commonly used in object-oriented programming, Propp enables C++ developers to work with properties in a more intuitive and Python-like manner, improving code readability and maintainability.

## Key Features:

- Header-Only Design: No need to compile a separate library or link against external dependencies. Propp is a single header file that can be included in your project, making it easy to integrate and use.

- Custom Getters and Setters: The library allows for custom logic to be executed whenever a property is accessed or modified

- Multithreading Support: Propp provides thread-safe access to properties, ensuring that concurrent reads and writes are handled correctly.

- Customizable types for getters and setters: Propp allows you to use returning data by value or reference for getters and setting by value or constant reference for setters.

- No recursion occurs when assigning or retrieving values within the setter or getter. For instance, if your getter modifies the underlying value (e.g., by multiplying it by 2), you can safely call the data access method inside the getter without triggering recursive calls.

```cpp
    class Person {
        PropertyRWG<int, GetterTypeValue<int>> Age; // <-- define getter returning data by value

        Person(const std::string& name)
            : Age(0, std::bind(&Person::GetAge, this)) { // <-- bind getter to the object}

        int GetAge() const { return Age() * 2; } // <-- custom getter that multiplies the value by 2
    };
```

### Known Limitations:

- Class that uses properties with getter or setter should implement a copy constructor to assign getter and setter functions to the new object.

- For properties with custome getter GetRaw() method should be used in copy constructor to copy proper underlying value.

```cpp
        // For properties with custom getter and setter, we need to define copy constructor
        Person(const Person& other)
            : Name(other.Name())
            , Age(other.Age(), std::bind(&Person::SetAge, this, std::placeholders::_1))
            , Address(other.Address.GetRaw(), std::bind(&Person::GetAddress, this)) // Because we declared getter that returns string by value, we need to use GetRaw() to get the initial value
        {
        }
```

### Examples:

- Declaration for read-only property
```cpp
    PropertyRO<int> Age;
```
- Declaration for read-write property
```cpp
    PropertyRW<int> Age;
```
- Declaration for read-write property with getter that returns data by value
```cpp
    PropertyRWG<int, GetterTypeValue<int>> Age;
```
- Declaration for read-only property with getter that returns data by reference (this behavior is default)
```cpp
    PropertyROG<int, GetterTypeRef<int>> Age;
```
- Declaration for read-write property with setter that sets data by value
```cpp
    PropertyRWS<int, SetterTypeValue<int>> Age;
```
- Declaration for read-write property with setter that sets data by constant reference (this behavior is default)
```cpp
    PropertyRWS<int, SetterTypeConstRef<int>> Age;
```
- Declaration for read-only property with multitheading support
```cpp
    PropertyROMT<int> Age;
```
- Declaration for read-write property with multitheading support
```cpp
    PropertyRWMT<int> Age;
```
- Declaration for read-write property with custom getter and setter and multitheading support
```cpp
    PropertyRWGSMT<int> Age;
```

- RO, RW - read-only and read-write properties
- G, S, GS - getter, setter or both. Empty means property doesn't have getter or setter support
- MT - if specified, property will be thread-safe

## How To Integrate with cmake

- Add propp as submodule to your project
```bash
git submodule add https://github.com/nextgeniuspro/propp.git vendor/propp
```
- Update submodules to download dependencies
```bash
git submodule update --init --recursive
```
- Add propp as subdirectory to your CMakeLists.txt
```cmake 
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vendor/propp propp_build)
```
- Add propp as dependency to your target and set C++17 standard
```cmake 
target_link_libraries(<your_project> PRIVATE propp)
target_compile_features(<your_project> PRIVATE cxx_std_17)
```

See examples/CMakeLists.txt for example of usage

## How To Build Example #

- Navigate to 'examples' directory
```bash
cd examples
```
- Run cmake to generate Visual Studio project
```bash
cmake -B ./build -G "Visual Studio 17 2022" .
```
- Run cmake to generate XCode project
```bash
cmake -B ./build -G "Xcode" .
```