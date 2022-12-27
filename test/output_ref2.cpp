// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/output_ref.hpp>
#include <boost/core/lightweight_test.hpp>
#include <sstream>

int main()
{
    std::ostringstream os( "foo", std::ios::app );

    boost::mustache::output_ref out( os );

    out.output( ".bar" );
    BOOST_TEST_EQ( os.str(), std::string("foo.bar") );

    out.output( ".baz" );
    BOOST_TEST_EQ( os.str(), std::string("foo.bar.baz") );

    return boost::report_errors();
}
