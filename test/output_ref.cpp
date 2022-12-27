// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/output_ref.hpp>
#include <boost/core/lightweight_test.hpp>
#include <string>

int main()
{
    std::string st( "foo" );

    boost::mustache::output_ref out( st );

    out.write( ".bar" );
    BOOST_TEST_EQ( st, std::string("foo.bar") );

    out.write( ".baz" );
    BOOST_TEST_EQ( st, std::string("foo.bar.baz") );

    return boost::report_errors();
}
