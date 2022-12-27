// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/render.hpp>
#include <boost/core/lightweight_test.hpp>
#include <string>

int main()
{
    std::string st;

	boost::mustache::render( "no-tags", st, {}, {} );

    BOOST_TEST_EQ( st, std::string( "no-tags" ) );

    return boost::report_errors();
}
