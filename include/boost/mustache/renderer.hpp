#ifndef BOOST_MUSTACHE_RENDERER_HPP_INCLUDED
#define BOOST_MUSTACHE_RENDERER_HPP_INCLUDED

// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/output_ref.hpp>
#include <boost/mustache/config.hpp>
#include <boost/json/value.hpp>
#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value_from.hpp>
#include <boost/core/detail/string_view.hpp>

namespace boost
{
namespace mustache
{

class renderer
{
private:

    json::array context_stack_;
    json::object partials_;

public:

    BOOST_MUSTACHE_DECL renderer( json::value const& data, json::value const& partials );
    BOOST_MUSTACHE_DECL ~renderer();

    template<class T1, class T2> explicit renderer( T1 const& data, T2 const& partials, json::storage_ptr sp = {} ):
        renderer( json::value_from( data, sp ), json::value_from( partials, sp ) )
    {
    }

    BOOST_MUSTACHE_DECL void render( core::string_view tmpl, output_ref out );
    BOOST_MUSTACHE_DECL void finish( output_ref out );
};

} // namespace mustache
} // namespace boost

#endif // #ifndef BOOST_MUSTACHE_RENDERER_HPP_INCLUDED
