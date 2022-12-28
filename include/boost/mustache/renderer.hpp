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
    };

private:

    json::array context_stack_;
    json::object partials_;

    state state_ = state_leading_wsp;

    // true when parsing the contents of a section tag
    bool in_section_ = false;

    // whitespace buffered by state_leading_wsp
    json::string whitespace_;

    // whether the tag has been preceded by only whitespace;
    // set on entering state_start_delim
    bool standalone_ = false;

    // start delimiter, default '{{'
    json::string start_delim_;

    // end delimiter, default '}}'
    json::string end_delim_;

    // current delimiter position in state_start_delim and
    // state_end_delim
    std::size_t delim_index_ = 0;

    // tag contents accumulated by state_tag
    json::string tag_;

    // section state, only valid when in_section_

    // stack of currently encountered open section tags
    json::array section_stack_;

    // whether the current section is inverted
    bool inverted_ = false;

    // the current context (e.g. for "{{#x.y}}",
    // a pointer to the lookup result of "x.y"
    json::value const * section_context_ = nullptr;

    // buffered section contents until its closing tag
    json::string section_text_;

    // partial state

    // leading whitespace before the standalone partial tag
    json::string partial_lwsp_;

private:

    BOOST_MUSTACHE_DECL core::string_view handle_state_leading_wsp( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_start_delim( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_tag( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_end_delim( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_end_triple( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_passthrough( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_standalone( core::string_view in, output_ref out );
    BOOST_MUSTACHE_DECL core::string_view handle_state_standalone_2( core::string_view in, output_ref out );

    BOOST_MUSTACHE_DECL void finish_state_leading_wsp( output_ref out );
    BOOST_MUSTACHE_DECL void finish_state_start_delim( output_ref out );
    BOOST_MUSTACHE_DECL void finish_state_tag( output_ref out );
    BOOST_MUSTACHE_DECL void finish_state_end_delim( output_ref out );
    BOOST_MUSTACHE_DECL void finish_state_end_triple( output_ref out );
    BOOST_MUSTACHE_DECL void finish_state_passthrough( output_ref out );
    BOOST_MUSTACHE_DECL void finish_state_standalone( output_ref out );
    BOOST_MUSTACHE_DECL void finish_state_standalone_2( output_ref out );

    BOOST_MUSTACHE_DECL void handle_tag( core::string_view tag, output_ref out, core::string_view old_wsp );

    BOOST_MUSTACHE_DECL void handle_comment_tag( core::string_view tag, output_ref out );
    BOOST_MUSTACHE_DECL void handle_interpolation_tag( core::string_view tag, output_ref out, bool quoted );
    BOOST_MUSTACHE_DECL void handle_section_tag( core::string_view tag, output_ref out, bool inverted );
    BOOST_MUSTACHE_DECL void handle_delimiter_tag( core::string_view tag, output_ref out );
    BOOST_MUSTACHE_DECL void handle_partial_tag( core::string_view tag, output_ref out, core::string_view old_wsp );

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
