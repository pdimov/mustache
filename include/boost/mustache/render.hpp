#ifndef BOOST_MUSTACHE_RENDER_HPP_INCLUDED
#define BOOST_MUSTACHE_RENDER_HPP_INCLUDED

// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/renderer.hpp>

namespace boost
{
namespace mustache
{

template<class T1 = json::value, class T2 = json::value> void render( core::string_view tmpl, output_ref out, T1 const& data, T2 const& partials, json::storage_ptr sp = {} )
{
    mustache::renderer rd( data, partials, sp );

    rd.render( tmpl, out );
    rd.finish( out );
}

} // namespace mustache
} // namespace boost

#endif // #ifndef BOOST_MUSTACHE_RENDER_HPP_INCLUDED
