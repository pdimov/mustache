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
#include <boost/json/string.hpp>
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

    enum state
    {
        state_leading_wsp,
        state_start_delim,
        state_tag,
        state_end_delim,
        state_end_triple,
        state_passthrough,
        state_standalone,
        state_standalone_2,
        state_section,
        state_section_start_delim,
        state_section_tag,
        state_section_end_delim,
        state_section_end_triple,
    };

private:

    json::array context_stack_;
    json::object partials_;

    state state_ = state_leading_wsp;
    bool standalone_ = false;

    json::string whitespace_;

    json::string start_delim_;
    json::string end_delim_;

    std::size_t delim_index_ = 0;

    json::string tag_;

    json::array section_stack_;
    bool inverted_ = false;
    json::value const * section_context_ = nullptr;

    json::string section_text_;

private:

    BOOST_MUSTACHE_DECL core::string_view handle_state_leading_wsp( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_start_delim( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_tag( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_end_delim( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_end_triple( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_passthrough( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_standalone( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_standalone_2( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_section( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_section_start_delim( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_section_tag( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_section_end_delim( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_section_end_triple( core::string_view in, output_ref out );

    BOOST_MUSTACHE_DECL void handle_state_section_end_delim_( output_ref out );

    BOOST_MUSTACHE_DECL void handle_tag( core::string_view tag, output_ref out );

    BOOST_MUSTACHE_DECL void handle_comment_tag( core::string_view tag, output_ref out );
    BOOST_MUSTACHE_DECL void handle_interpolation_tag( core::string_view tag, output_ref out, bool quoted );
    BOOST_MUSTACHE_DECL void handle_section_tag( core::string_view tag, output_ref out, bool inverted );
    BOOST_MUSTACHE_DECL void handle_delimiter_tag( core::string_view tag, output_ref out );
    BOOST_MUSTACHE_DECL void handle_partial_tag( core::string_view tag, output_ref out );

    BOOST_MUSTACHE_DECL json::value const* lookup_value( core::string_view name ) const;

    BOOST_MUSTACHE_DECL void render_section( output_ref out );

private:

    BOOST_MUSTACHE_DECL renderer( json::value&& data, json::object&& partials, json::storage_ptr sp );

public:

    BOOST_MUSTACHE_DECL ~renderer();

    template<class T1, class T2> explicit renderer( T1 const& data, T2 const& partials, json::storage_ptr sp = {} ):
        renderer( json::value_from( data, sp ), json::value_from( partials, sp ).as_object(), sp )
    {
    }

    BOOST_MUSTACHE_DECL void render_some( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL void finish( output_ref out );
};

} // namespace mustache
} // namespace boost

#endif // #ifndef BOOST_MUSTACHE_RENDERER_HPP_INCLUDED
