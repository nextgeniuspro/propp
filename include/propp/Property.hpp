#include <mutex>
#include <functional>
#include <type_traits>

namespace propp {
    
template <typename T>
using GetterTypeRef = std::function<T&()>;
template <typename T>
using GetterTypeValue = std::function<T()>;

template <typename T>
using SetterTypeCRef = std::function<void(const T&)>;
template <typename T>
using SetterTypeValue = std::function<void(T)>;

struct NoGetter {};
struct NoSetter {};

template <typename T, 
    bool ReadOnly, 
    bool ThreadSafe, 
    typename GetterType = GetterTypeRef<T>,
    typename SetterType = SetterTypeValue<T>
>
class Property {
public:
    using Getter = GetterType;
    using Setter = SetterType;

    static_assert(
        std::is_same_v<GetterType, GetterTypeValue<T>> || std::is_same_v<GetterType, GetterTypeRef<T>> || std::is_same_v<GetterType, NoGetter>,
        "GetterType must be either std::function<T>, std::function<T&> or NoGetter"
    );
    static_assert(
        std::is_same_v<SetterType, SetterTypeValue<T>> || std::is_same_v<SetterType, SetterTypeCRef<T>> || std::is_same_v<SetterType, NoSetter>,
        "SetterType must be std::function<void(T)>, std::function<void(const T&)> or NoSetter"
    );

    // No getter and setter constructor
    template <typename G = GetterType, typename S = SetterType, typename = std::enable_if_t<
        std::is_same_v<S, NoSetter> && std::is_same_v<G, NoGetter>
    >>
    Property(const T& value = T())
        : m_Value(value)
        , m_SetterActive(false)
        , m_GetterActive(false)
    {
    }
    
    // Getter only constructor
    template <typename G = GetterType, typename S = SetterType, typename = std::enable_if_t<
        std::is_same_v<S, NoSetter>
    >>
    Property(const T& value, const Getter& getter)
        : m_Value(value)
        , m_Getter(getter)
        , m_SetterActive(false)
        , m_GetterActive(false)
    {
    }
    
    // Setter only constructor
    template <typename G = GetterType, typename S = SetterType, typename = std::enable_if_t<
        std::is_same_v<G, NoGetter>
    >>
    Property(const T& value, const Setter& setter)
        : m_Value(value)
        , m_Setter(setter)
        , m_SetterActive(false)
        , m_GetterActive(false)
    {
    }
    
    // Getter and setter constructor
    Property(const T& value, const Getter& getter, const Setter& setter)
        : m_Value(value)
        , m_Getter(getter)
        , m_Setter(setter)
        , m_SetterActive(false)
        , m_GetterActive(false)
    {
    }

    // Non copyable
    Property() = default;
    Property(const Property&) = delete;
    Property& operator=(const Property&) = delete;

    // Type conversion operators
    operator T() { return Get(); }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    T operator*() {
        return Get();
    }

    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    const T operator*() const {
        return Get();
    }

    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    T operator()() {
        return Get();
    }

    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    const T operator()() const {
        return Get();
    }

    // Case 2: If GetterType is GetterTypeRef, return by reference
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    T& operator*() {
        return Get();
    }

    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    const T& operator*() const {
        return Get();
    }

    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    T& operator()() {
        return Get();
    }

    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    const T& operator()() const {
        return Get();
    }

    // Get raw value
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    inline T GetRaw() {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return m_Value;
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    inline const T GetRaw() const {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return m_Value;
    }

    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    inline T& GetRaw() {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return m_Value;
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    inline const T& GetRaw() const {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return m_Value;
    }

    // Assignment operators
    template <typename F, bool RO = ReadOnly, typename std::enable_if<!RO, int>::type = 0>
    Property& operator=(const F& value) {
        Set(static_cast<T>(value));
        return *this;
    }

    // Getter and setter
    void SetGetter(const Getter& customGetter) {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        if constexpr (std::is_same_v<GetterType, NoGetter>) {
            m_Getter = customGetter; // TODO: Throw error
        }
    }

    template <bool RO = ReadOnly, typename std::enable_if<!RO, int>::type = 0>
    void SetSetter(const Setter& customSetter) {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        if constexpr (std::is_same_v<SetterType, NoSetter>) {
            m_Setter = customSetter; // TODO: Throw error
        }
    }

    // Arithmetic operators
    template <typename F>
    Property& operator+=(const F& value) { return ApplyOperation([&](T& v) { v += static_cast<T>(value); }); }
    template <typename F>
    Property& operator-=(const F& value) { return ApplyOperation([&](T& v) { v -= static_cast<T>(value); }); }
    template <typename F>
    Property& operator*=(const F& value) { return ApplyOperation([&](T& v) { v *= static_cast<T>(value); }); }
    template <typename F>
    Property& operator/=(const F& value) { return ApplyOperation([&](T& v) { v /= static_cast<T>(value); }); }
    template <typename F>
    Property& operator%=(const F& value) { return ApplyOperation([&](T& v) { v %= static_cast<T>(value); }); }
    
    // Bitwise operators
    template <typename F>
    Property& operator&=(const F& value) { return ApplyOperation([&](T& v) { v &= static_cast<T>(value); }); }
    template <typename F>
    Property& operator|=(const F& value) { return ApplyOperation([&](T& v) { v |= static_cast<T>(value); }); }
    template <typename F>
    Property& operator^=(const F& value) { return ApplyOperation([&](T& v) { v ^= static_cast<T>(value); }); }
    template <typename F>
    Property& operator<<=(const F& value) { return ApplyOperation([&](T& v) { v <<= static_cast<T>(value); }); }
    template <typename F>
    Property& operator>>=(const F& value) { return ApplyOperation([&](T& v) { v >>= static_cast<T>(value); }); }
    
    // Prefix increment and decrement
    Property& operator++() { return ApplyOperation([&](T& v) { ++v; }); }
    Property& operator--() { return ApplyOperation([&](T& v) { --v; }); }

    // Postfix increment and decrement
    Property& operator++(int) { return ApplyOperation([&](T& v) { T old = v; v++; return old; }); }
    Property& operator--(int) { return ApplyOperation([&](T& v) { T old = v; v--; return old; }); }

    // Arithmetic operator overloads (binary operators)
    template <typename F>
    auto operator+(const F& value) const { return Get() + static_cast<T>(value); }
    template <typename F>
    auto operator-(const F& value) const { return Get() - static_cast<T>(value); }
    template <typename F>
    auto operator*(const F& value) const { return Get() * static_cast<T>(value); }
    template <typename F>
    auto operator/(const F& value) const { return Get() / static_cast<T>(value); }
    template <typename F>
    auto operator%(const F& value) const { return Get() % static_cast<T>(value); }

    // Bitwise operator overloads (binary operators)
    template <typename F>
    auto operator&(const F& value) const { return Get() & static_cast<T>(value); }
    template <typename F>
    auto operator|(const F& value) const { return Get() | static_cast<T>(value); }
    template <typename F>
    auto operator^(const F& value) const { return Get() ^ static_cast<T>(value); }
    template <typename F>
    auto operator<<(const F& value) const { return Get() << static_cast<T>(value); }
    template <typename F>
    auto operator>>(const F& value) const { return Get() >> static_cast<T>(value); }

    // Comparison operators
    template <typename F>
    bool operator==(const F& value) const { return Get() == static_cast<T>(value); }
    template <typename F>
    bool operator!=(const F& value) const { return Get() != static_cast<T>(value); }
    template <typename F>
    bool operator<(const F& value) const { return Get() < static_cast<T>(value); }
    template <typename F>
    bool operator<=(const F& value) const { return Get() <= static_cast<T>(value); }
    template <typename F>
    bool operator>(const F& value) const { return Get() > static_cast<T>(value); }
    template <typename F>
    bool operator>=(const F& value) const { return Get() >= static_cast<T>(value); }

protected:
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    inline T Get() {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return GetST();
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    inline const T Get() const {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return GetST();
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    inline T& Get() {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return GetST();
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    inline const T& Get() const {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        return GetST();
    }

    inline void Set(const T& newValue) {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        SetST(newValue);
    }

    inline Property& ApplyOperation(const std::function<void(T&)>& operation) {
        if constexpr (ThreadSafe) {
            std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        }
        auto value = GetST();
        operation(value);
        SetST(value);
        
        return *this;
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    inline T GetST() {
        if constexpr (!std::is_same_v<GetterType, NoGetter>) {
            if (m_Getter && !m_GetterActive) {
                GetterGuard guard(*this);
                return m_Getter();
            }
        }
        return m_Value;
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeValue<T>>
    >>
    inline const T GetST() const {
        if constexpr (!std::is_same_v<GetterType, NoGetter>) {
            if (m_Getter && !m_GetterActive) {
                GetterGuard guard(*this);
                return m_Getter();
            }
        }
        return m_Value;
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    inline T& GetST() {
        if constexpr (!std::is_same_v<GetterType, NoGetter>) {
            if (m_Getter && !m_GetterActive) {
                GetterGuard guard(*this);
                return m_Getter();
            }
        }
        return m_Value;
    }
    
    template <typename G = GetterType, typename = std::enable_if_t<
        std::is_same_v<G, GetterTypeRef<T>> || std::is_same_v<G, NoGetter>
    >>
    inline const T& GetST() const {
        if constexpr (!std::is_same_v<GetterType, NoGetter>) {
            if (m_Getter && !m_GetterActive) {
                GetterGuard guard(*this);
                return m_Getter();
            }
        }
        return m_Value;
    }

    inline void SetST(const T& newValue) {
        if constexpr (!std::is_same_v<SetterType, NoSetter>) {
            if (m_Setter && !m_SetterActive) {
                SetterGuard guard(*this);
                m_Setter(newValue);
                return;
            }
        } 

        m_Value = newValue;
    }

private:
    // Helper classe to guard the setter
    class SetterGuard {
    public:
        SetterGuard(const Property& prop) : m_Property(prop) {
            m_Property.m_SetterActive = true;
        }
        ~SetterGuard() {
            m_Property.m_SetterActive = false;
        }
    private:
        const Property& m_Property;
    };
    
    // Helper classe to guard the getter
    class GetterGuard {
    public:
        GetterGuard(const Property& prop) : m_Property(prop) {
            m_Property.m_GetterActive = true;
        }
        ~GetterGuard() {
            m_Property.m_GetterActive = false;
        }
    private:
        const Property& m_Property;
    };

    T m_Value;
    mutable std::recursive_mutex m_Mutex;

    Getter m_Getter;
    Setter m_Setter;
    mutable bool m_SetterActive;
    mutable bool m_GetterActive;
};

// Read-write property, single-threaded, no getter or setter
template <typename T>
using PropertyRW = Property<T, false, false, NoGetter, NoSetter>;

// Read-write property, single-threaded, with getter
template <typename T, typename GetterType = GetterTypeValue<T>>
using PropertyRWG = Property<T, false, false, GetterType, NoSetter>;

// Read-write property, single-threaded, with setter
template <typename T, typename SetterType = SetterTypeValue<T>>
using PropertyRWS = Property<T, false, false, NoGetter, SetterType>;

// Read-write property, single-threaded, with getter and setter
template <typename T, typename GetterType = GetterTypeValue<T>, typename SetterType = SetterTypeValue<T>>
using PropertyRWGS = Property<T, false, false, GetterType, SetterType>;


// Read-write property, single-threaded, no getter or setter
template <typename T>
using PropertyRWMT = Property<T, false, true, NoGetter, NoSetter>;

// Read-write property, single-threaded, with getter
template <typename T, typename GetterType = GetterTypeValue<T>>
using PropertyRWGMT = Property<T, false, true, GetterType, NoSetter>;

// Read-write property, single-threaded, with setter
template <typename T, typename SetterType = SetterTypeValue<T>>
using PropertyRWSMT = Property<T, false, true, NoGetter, SetterType>;

// Read-write property, single-threaded, with getter and setter
template <typename T, typename GetterType = GetterTypeValue<T>, typename SetterType = SetterTypeValue<T>>
using PropertyRWGSMT = Property<T, false, true, GetterType, SetterType>;


// Read-only property, single-threaded, no getter or setter
template <typename T>
using PropertyRO = Property<T, true, false, NoGetter, NoSetter>;

// Read-only property, single-threaded, with getter
template <typename T, typename GetterType = GetterTypeValue<T>>
using PropertyROG = Property<T, true, false, GetterType, NoSetter>;

// Read-only property, single-threaded, with setter
template <typename T, typename SetterType = SetterTypeValue<T>>
using PropertyROS = Property<T, true, false, NoGetter, SetterType>;

// Read-only property, single-threaded, with getter and setter
template <typename T, typename GetterType = GetterTypeValue<T>, typename SetterType = SetterTypeValue<T>>
using PropertyROGS = Property<T, true, false, GetterType, SetterType>;


// Read-only property, multi-threaded, no getter or setter
template <typename T>
using PropertyROMT = Property<T, true, true, NoGetter, NoSetter>;

// Read-only property, multi-threaded, with getter
template <typename T, typename GetterType = GetterTypeValue<T>>
using PropertyROGMT = Property<T, true, true, GetterType, NoSetter>;

// Read-only property, multi-threaded, with setter
template <typename T, typename SetterType = SetterTypeValue<T>>
using PropertyROSMT = Property<T, true, true, NoGetter, SetterType>;

// Read-only property, multi-threaded, with getter and setter
template <typename T, typename GetterType = GetterTypeValue<T>, typename SetterType = SetterTypeValue<T>>
using PropertyROGSMT = Property<T, true, true, GetterType, SetterType>;

} // namespace propp
