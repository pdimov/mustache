// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/renderer.hpp>
#include <boost/assert.hpp>
#include <utility>

boost::mustache::renderer::renderer( json::value&& data, json::object&& partials, json::storage_ptr sp ):
    context_stack_( sp ), partials_( std::move( partials ), sp ),
    whitespace_( sp ), start_delim_( "{{", sp ), end_delim_( "}}", sp ), tag_( sp )
{
    context_stack_.push_back( std::move( data ) );
}

boost::mustache::renderer::~renderer()
{
}

void boost::mustache::renderer::render_some( core::string_view in, output_ref out )
{
    // out.write( tmpl );

    while( !in.empty() )
    {
        switch( state_ )
        {
        case state_leading_wsp:

            in = handle_state_leading_wsp( in, out );
            break;

        case state_start_delim:

            in = handle_state_start_delim( in, out );
            break;

        case state_tag:

            in = handle_state_tag( in, out );
            break;

        case state_end_delim:

            in = handle_state_end_delim( in, out );
            break;

        case state_passthrough:

            in = handle_state_passthrough( in, out );
            break;

        case state_standalone:

            in = handle_state_standalone( in, out );
            break;

        case state_standalone_2:

            in = handle_state_standalone_2( in, out );
            break;
        }
    }
}

boost::core::string_view boost::mustache::renderer::handle_state_leading_wsp( core::string_view in, output_ref out )
{
    char const* p = in.data();
    char const* end = p + in.size();

    while( p != end && ( *p == ' ' || *p == '\t' ) )
    {
        ++p;
    }

    whitespace_.append( in.data(), p );

    if( p != end )
    {
        if( *p == start_delim_[0] )
        {
            standalone_ = true;

            state_ = state_start_delim;
            delim_index_ = 0;
        }
        else
        {
            out.write( whitespace_ );
            whitespace_.clear();

            state_ = state_passthrough;
        }
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_start_delim( core::string_view in, output_ref out )
{
    char const* p = in.data();
    char const* end = p + in.size();

    while( delim_index_ < start_delim_.size() && p != end && *p == start_delim_[ delim_index_ ] )
    {
        ++p;
        ++delim_index_;
    }

    if( delim_index_ == start_delim_.size() )
    {
        // start delimiter

        state_ = state_tag;
        tag_.clear();
    }
    else if( p != end )
    {
        // not a start delimiter

        out.write( { start_delim_.data(), delim_index_ } );

        state_ = state_passthrough;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_tag( core::string_view in, output_ref /*out*/ )
{
    char const* p = in.data();
    char const* end = p + in.size();

    while( p != end && *p != end_delim_[0] )
    {
        ++p;
    }

    tag_.append( in.data(), p );

    if( p != end )
    {
        state_ = state_end_delim;
        delim_index_ = 0;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_end_delim( core::string_view in, output_ref out )
{
    char const* p = in.data();
    char const* end = p + in.size();

    while( delim_index_ < end_delim_.size() && p != end && *p == end_delim_[ delim_index_ ] )
    {
        ++p;
        ++delim_index_;
    }

    if( delim_index_ == end_delim_.size() )
    {
        // end delimiter

        handle_tag( tag_, out );
        tag_.clear();

        state_ = standalone_? state_standalone: state_passthrough;
    }
    else if( p != end )
    {
        // not an end delimiter

        tag_.append( end_delim_.data(), end_delim_.data() + delim_index_ );

        state_ = state_tag;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_passthrough( core::string_view in, output_ref out )
{
    char const* p = in.data();
    char const* end = p + in.size();

    while( p != end && *p != '\n' && *p != start_delim_[0] )
    {
        ++p;
    }

    out.write( { in.data(), static_cast<std::size_t>( p - in.data() ) } );

    if( p != end )
    {
        if( *p == '\n' )
        {
            out.write( { p, 1 } );
            ++p;

            state_ = state_leading_wsp;
        }
        else
        {
            standalone_ = false;

            state_ = state_start_delim;
            delim_index_ = 0;
        }
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_standalone( core::string_view in, output_ref /*out*/ )
{
    char const* p = in.data();
    char const* end = p + in.size();

    BOOST_ASSERT( p != end );

    if( *p == '\r' )
    {
        ++p;
        state_ = state_standalone_2;
    }
    else if( *p == '\n' )
    {
        ++p;
        state_ = state_leading_wsp;
    }
    else
    {
        state_ = state_passthrough;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_standalone_2( core::string_view in, output_ref out )
{
    char const* p = in.data();
    char const* end = p + in.size();

    BOOST_ASSERT( p != end );

    if( *p == '\n' )
    {
        ++p;
        state_ = state_leading_wsp;
    }
    else
    {
        out.write( "\r" );
        state_ = state_passthrough;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

void boost::mustache::renderer::handle_tag( core::string_view /*tag*/, output_ref /*out*/ )
{
}

void boost::mustache::renderer::finish( output_ref /*out*/ )
{
}
