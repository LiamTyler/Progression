#pragma once

#include "rapidjson/document.h"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "glm/glm.hpp"
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

glm::vec3 ParseVec3( const rapidjson::Value& v );
glm::vec4 ParseVec4( const rapidjson::Value& v );

template < typename ...Args >
class FunctionMapper
{
    using function_type = std::function< void( const rapidjson::Value&, Args... ) >;
    using map_type = std::unordered_map< std::string, function_type >;
public:
    FunctionMapper( const map_type& m ) : mapping( m ) {}

    function_type& operator[]( const std::string& name )
    {
        PG_ASSERT( mapping.find( name ) != mapping.end(), name + " not found in mapping" );
        return mapping[name];
    }

    void Evaluate( const std::string& name, const rapidjson::Value& v, Args&&... args )
    {
        if ( mapping.find( name ) == mapping.end() )
        {
            LOG_WARN( "'%s' not found in mapping\n", name.c_str() );
        }
        else
        {
            mapping[name]( v, std::forward<Args>( args )... );
        }
    }

    void ForEachMember( const rapidjson::Value& v, Args&&... args )
    {
        for ( auto it = v.MemberBegin(); it != v.MemberEnd(); ++it )
        {
            std::string name = it->name.GetString();
            if ( mapping.find( name ) == mapping.end() )
            {
                LOG_WARN( "'%s' not found in mapping\n", name.c_str() );
            }
            else
            {
                mapping[name]( it->value, std::forward<Args>( args )... );
            }
        }
    }

    map_type mapping;
};  
