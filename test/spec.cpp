// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/render.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <fstream>

int main( int ac, char const* av[] )
{
    if( ac < 2 )
    {
        std::cerr << "Usage: spec <spec.json>" << std::endl;
        return -1;
    }

    int errors = 0;

    try
    {
        std::ifstream is( av[1] );

        boost::json::value spec = boost::json::parse( is );

        auto tests = spec.at( "tests" ).as_array();

        for( auto const& test: tests )
        {
            auto name = test.at( "name" ).as_string();
            auto data = test.at( "data" );
            auto template_ = test.at( "template" ).as_string();
            auto expected = test.at( "expected" ).as_string();

            boost::json::object partials;

            if( test.as_object().contains( "partials" ) )
            {
                partials = test.at( "partials" ).as_object();
            }

            std::string result;
            boost::mustache::render( template_, result, data, partials );

            if( result != expected )
            {
                std::cerr << "Test '" << name.subview() << "' failed: result '" << result << "', expected '" << expected.subview() << "'" << std::endl;
                ++errors;
            }
        }

        return errors;
    }
    catch( std::exception const& x )
    {
        std::cerr << "Exception: " << x.what() << std::endl;
        return -1;
    }
}
