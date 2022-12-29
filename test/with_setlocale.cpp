// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/render.hpp>
#include <boost/core/detail/lwt_unattended.hpp>
#include <string>
#include <iostream>
#include <algorithm>
#include <clocale>

int errors = 0;

static void test( boost::core::string_view tmpl, boost::json::value const& data, boost::json::object const& partials, boost::core::string_view expected )
{
    std::string result;
    boost::mustache::render( tmpl, result, data, partials );

    if( result != expected )
    {
        std::cerr << "Test failed: result '" << result << "', expected '" << expected << "'" << std::endl;
        ++errors;
    }
}

int main()
{
    boost::core::detail::lwt_unattended();

    std::setlocale( LC_ALL, "de_DE" );

    test( "'{{value}}'", { { "value", -1048576LL } }, {}, "'-1048576'" );
    test( "'{{value}}'", { { "value", 1048576ULL } }, {}, "'1048576'" );
    test( "'{{value}}'", { { "value", -1023.14159 } }, {}, "'-1023.14159'" );
    test( "'{{value}}'", { { "value", -1.7e-38 } }, {}, "'-1.7e-38'" );

    return errors;
}
