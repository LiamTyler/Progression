#pragma once

#include "shared/platform_defines.hpp"
#include "shared/logger.hpp"
#include <stdint.h>
#include <string>

#define DVAR_DEBUGGING USE_IF( USING( DEBUG_BUILD ) )

#if USING( DVAR_DEBUGGING )
#include "shared/assert.hpp"
#include <typeinfo>
#define DVAR_ASSERT( ... ) PG_ASSERT( __VA_ARGS__ )
#else // #if USING( DVAR_DEBUGGING )
#define DVAR_ASSERT( ... )  do {} while ( 0 )
#endif // #if USING( DVAR_DEBUGGING )

namespace PG
{

class Dvar
{
private:
    enum class DvarType : uint8_t
    {
        BOOL,
        INT,
        UINT,
        FLOAT,
        DOUBLE,

        COUNT
    };

    static const char* TypeToString( DvarType t );

    union DvarValue
    {
        bool bVal;
        int iVal;
        uint32_t uVal;
        float fVal;
        double dVal;
    };

public:

    explicit Dvar( const char* const inName, bool defaultVal, const char* const desc );
    explicit Dvar( const char* const inName, int defaultVal, int minVal, int maxVal, const char* const desc );
    explicit Dvar( const char* const inName, uint32_t defaultVal, uint32_t minVal, uint32_t maxVal, const char* const desc );
    explicit Dvar( const char* const inName, float defaultVal, float minVal, float maxVal, const char* const desc );
    explicit Dvar( const char* const inName, double defaultVal, double minVal, double maxVal, const char* const desc );

    const char* const GetName() const;
    const char* const GetDescription() const;
    bool GetBool() const;
    int GetInt() const;
    uint32_t GetUint() const;
    float GetFloat() const;
    double GetDouble() const;

    template <typename T>
    void Set( const T& newVal )
    {
        using UT = relaxed_underlying_type<T>::type;
        if constexpr ( std::is_same_v<UT, bool> )
        {
            DVAR_ASSERT( type == DvarType::BOOL, "Trying to a BOOL dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.bVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, short> || std::is_same_v<UT, int> )
        {
            DVAR_ASSERT( type == DvarType::INT, "Trying to an INT dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.iVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, uint8_t> || std::is_same_v<UT, uint16_t> || std::is_same_v<UT, uint32_t> )
        {
            DVAR_ASSERT( type == DvarType::UINT, "Trying to a UINT dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.uVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, float> )
        {
            DVAR_ASSERT( type == DvarType::FLOAT, "Trying to a FLOAT dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.fVal = newVal;
        }
        else if constexpr ( std::is_same_v<UT, double> )
        {
            DVAR_ASSERT( type == DvarType::DOUBLE, "Trying to a DOUBLE dvar %s with a variable of type %s", name, typeid( UT ).name() );
            value.dVal = newVal;
        }
        else
        {
            LOG_ERR( "Wtf type is this? %s vs %s", typeid( UT ).name(), typeid( T ).name() );
        }
        // TODO: ideally, there would be an else statement here with a static_assert inside of it if none of the IFs above this pass
        //  constexpr if's don't work, but should try again with consteval if from C++23 once compilers support it
    }

private:
    const DvarType type;
    DvarValue value;
    const DvarValue minValue = { 0 };
    const DvarValue maxValue = { 0 };
    const char* const name;
    const char* const description;
};

Dvar* GetDvar( std::string_view name );

} // namespace PG

#if USING( DVAR_DEBUGGING )
#undef DVAR_ASSERT
#endif // #if USING( DVAR_DEBUGGING )