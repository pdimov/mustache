// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/render.hpp>
#include <boost/core/detail/lwt_unattended.hpp>
#include <string>
#include <iostream>
#include <algorithm>

int errors = 0;

static void test( boost::core::string_view tmpl, boost::json::value const& data, boost::json::object const& partials, boost::core::string_view expected )
{
    // entire buffer

    {
        std::string result;
        boost::mustache::render( tmpl, result, data, partials );

        if( result != expected )
        {
            std::cerr << "Test failed: result '" << result << "', expected '" << expected << "'" << std::endl;
            ++errors;
        }
    }

    // chunked

    for( std::size_t i = 1; i <= 8; ++i )
    {
        boost::mustache::renderer rd( data, partials );

        boost::core::string_view in = tmpl;
        std::string result;

        while( !in.empty() )
        {
            std::size_t m = std::min( i, in.size() );

            rd.render_some( { in.data(), m }, result );
            in.remove_prefix( m );
        }

        rd.finish( result );

        if( result != expected )
        {
            std::cerr << "Test (i=" << i << ") failed: result '" << result << "', expected '" << expected << "'" << std::endl;
            ++errors;
        }
    }
}

int main()
{
    boost::core::detail::lwt_unattended();

    test(

        "null: '{{null}}'; false: '{{false}}'; true: '{{true}}'; int64: '{{int64}}'; uint64: '{{uint64}}'; double: '{{double}}'; double2: '{{double2}}'; array: '{{array}}'; object: '{{&object}}'",
        { { "null", nullptr }, { "false", false }, { "true", true }, { "int64", -1048576LL }, { "uint64", 1048576ULL }, { "double", 1023.14159 }, { "double2", 1.7e+38 }, { "array", { 1, 2, 3 } }, { "object", { { "x", 1 }, { "y", 2 } } } },
        {},
        "null: ''; false: 'false'; true: 'true'; int64: '-1048576'; uint64: '1048576'; double: '1023.14159'; double2: '1.7e+38'; array: '[1,2,3]'; object: '{\"x\":1,\"y\":2}'"
    );

    return errors;
}
