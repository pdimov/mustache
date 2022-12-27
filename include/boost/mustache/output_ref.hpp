#ifndef BOOST_MUSTACHE_OUTPUT_REF_HPP_INCLUDED
#define BOOST_MUSTACHE_OUTPUT_REF_HPP_INCLUDED

// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/core/detail/string_view.hpp>
#include <string>
#include <iosfwd>
#include <type_traits>

namespace boost
{
namespace mustache
{

class output_ref
{
private:

    using fptr = void (*)( void * out, core::string_view sv );

    void * out_;
    fptr fptr_;

private:

    template<class St> static void string_output( void * out, core::string_view sv )
    {
        St& st = *static_cast<St*>( out );
        st.append( sv.data(), sv.size() );
    }

    template<class Os> static void stream_output( void * out, core::string_view sv )
    {
        Os& os = *static_cast<Os*>( out );
        os.write( sv.data(), sv.size() );
    }

public:

    output_ref( std::string& st ): out_( &st ), fptr_( &string_output<std::string> )
    {
    }

    template<class Os, class En = typename std::enable_if<std::is_convertible<Os*, std::ostream*>::value>::type>
    output_ref( Os& os ): out_( &os ), fptr_( &stream_output<Os> )
    {
    }

    void write( core::string_view sv )
    {
        fptr_( out_, sv );
    }
};

} // namespace mustache
} // namespace boost

#endif // #ifndef BOOST_MUSTACHE_OUTPUT_REF_HPP_INCLUDED
