// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/render.hpp>
#include <boost/core/lightweight_test.hpp>
#include <string>

int main()
{
    std::string st;

    boost::mustache::render( "prefix * {{! comment }} * suffix", st, {}, {} );

    BOOST_TEST_EQ( st, std::string( "prefix *  * suffix" ) );

    return boost::report_errors();
}
