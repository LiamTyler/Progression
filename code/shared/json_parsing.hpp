#pragma once

#include "glm/glm.hpp"
#include "rapidjson/document.h"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <functional>
#include <string>
#include <unordered_map>

bool ParseJSONFile( const std::string& filename, rapidjson::Document& document );

template < typename T >
T ParseNumber( const rapidjson::Value& v )
{
    PG_ASSERT( v.IsNumber() );
    return v.Get< T >();
}


template<typename ComponentType = float>
glm::vec<2, ComponentType> ParseVec2( const rapidjson::Value& v )
{
    PG_ASSERT( v.IsArray() && v.Size() == 2 );
    auto& GetF = ParseNumber< ComponentType >;
    return glm::vec<2, ComponentType>( GetF( v[0] ), GetF( v[1] ) );
}


template<typename ComponentType = float>
glm::vec<3, ComponentType> ParseVec3( const rapidjson::Value& v )
{
    PG_ASSERT( v.IsArray() && v.Size() == 3 );
    auto& GetF = ParseNumber< ComponentType >;
    return glm::vec<3, ComponentType>( GetF( v[0] ), GetF( v[1] ), GetF( v[2] ) );
}


template<typename ComponentType = float>
glm::vec<4, ComponentType> ParseVec4( const rapidjson::Value& v )
{
    PG_ASSERT( v.IsArray() && v.Size() == 4 );
    auto& GetF = ParseNumber< ComponentType >;
    return glm::vec<4, ComponentType>( GetF( v[0] ), GetF( v[1] ), GetF( v[2] ), GetF( v[3] ) );
}

template < typename ...Args >
class JSONFunctionMapper
{
public:
    using FunctionType = std::function< void( const rapidjson::Value&, Args... ) >;
    using StringToFuncMap = std::unordered_map< std::string, FunctionType >;

    JSONFunctionMapper( const StringToFuncMap& m ) : mapping( m ) {}

    FunctionType& operator[]( const std::string& name )
    {
        PG_ASSERT( mapping.find( name ) != mapping.end(), "Key '%s' not found in mapping", name.c_str() );
        return mapping[name];
    }

    void Evaluate( const std::string& name, const rapidjson::Value& v, Args&&... args )
    {
        if ( mapping.find( name ) != mapping.end() )
        {
            mapping[name]( v, std::forward<Args>( args )... );
        }
    }

    void ForEachMember( const rapidjson::Value& v, Args&... args )
    {
        for ( auto it = v.MemberBegin(); it != v.MemberEnd(); ++it )
        {
            std::string name = it->name.GetString();
            if ( mapping.find( name ) != mapping.end() )
            {
                mapping[name]( it->value, std::forward<Args>( args )... );
            }
#if USING( DEBUG_BUILD )
            else if ( name != "name" )
            {
                LOG_WARN( "JSON mapper does not contain key '%s'", name.c_str() );
            }
#endif // #if USING( DEBUG_BUILD )
        }
    }

    StringToFuncMap mapping;
};


template < typename ...Args >
class JSONFunctionMapperBoolCheck
{
    using function_type = std::function< bool( const rapidjson::Value&, Args... ) >;
    using map_type = std::unordered_map< std::string, function_type >;
public:
    JSONFunctionMapperBoolCheck( const map_type& m ) : mapping( m ) {}

    function_type& operator[]( const std::string& name )
    {
        PG_ASSERT( mapping.find( name ) != mapping.end(), "Key '%s' not found in mapping", name.c_str() );
        return mapping[name];
    }

    bool Evaluate( const std::string& name, const rapidjson::Value& v, Args&&... args )
    {
        if ( mapping.find( name ) != mapping.end() )
        {
            return mapping[name]( v, std::forward<Args>( args )... );
        }

        return true;
    }

    bool ForEachMember( const rapidjson::Value& v, Args&... args )
    {
        for ( auto it = v.MemberBegin(); it != v.MemberEnd(); ++it )
        {
            std::string name = it->name.GetString();
            if ( mapping.find( name ) != mapping.end() )
            {
                if ( !mapping[name]( it->value, std::forward<Args>( args )... ) )
                {
                    return false;
                }
            }
        }

        return true;
    }

    map_type mapping;
};


template< typename Func >
void ForEachJSONMember( const rapidjson::Value& v, const Func& func )
{
    for ( auto it = v.MemberBegin(); it != v.MemberEnd(); ++it )
    {
        func( it->name.GetString(), it->value );
    }
}