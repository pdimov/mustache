// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/renderer.hpp>
#include <boost/json/serializer.hpp>
#include <boost/assert.hpp>
#include <utility>
#include <algorithm>
#include <clocale>

boost::mustache::renderer::renderer( json::value&& data, json::object&& partials, json::storage_ptr sp ):
    context_stack_( sp ), partials_( std::move( partials ), sp ),
    whitespace_( sp ), start_delim_( "{{", sp ), end_delim_( "}}", sp ),
    tag_( sp ), section_stack_( sp ), section_text_( sp ), partial_lwsp_( sp )
{
    context_stack_.push_back( std::move( data ) );
}

boost::mustache::renderer::~renderer()
{
}

// utility functions

static boost::core::string_view trim_leading_whitespace( boost::core::string_view sv )
{
    char const* p = sv.data();
    char const* end = p + sv.size();

    while( p != end && ( *p == ' ' || *p == '\t' ) )
    {
        ++p;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

static boost::core::string_view trim_trailing_whitespace( boost::core::string_view sv )
{
    char const* p = sv.data();
    char const* end = p + sv.size();

    while( p != end && ( end[ -1 ] == ' ' || end[ -1 ] == '\t' ) )
    {
        --end;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

static boost::core::string_view trim_whitespace( boost::core::string_view sv )
{
    sv = trim_leading_whitespace( sv );
    sv = trim_trailing_whitespace( sv );

    return sv;
}

//

void boost::mustache::renderer::render_some( core::string_view in, output_ref out )
{
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

        case state_end_triple:

            in = handle_state_end_triple( in, out );
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

        default:

            BOOST_ASSERT( false );
            return;
        }
    }
}

// state_leading_wsp consumes initial whitespace at the beginning of the line,
// and accumulates it into whitespace_

boost::core::string_view boost::mustache::renderer::handle_state_leading_wsp( core::string_view in, output_ref out )
{
    if( in_section_ )
    {
        out = section_text_;
    }

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

// finish_state_leading_wsp is called when input ends while in state_whitespace

void boost::mustache::renderer::finish_state_leading_wsp( output_ref out )
{
    if( in_section_ )
    {
        in_section_ = false;
    }
    else if( whitespace_ != partial_lwsp_ )
    {
        // if all whitespace comes from the partial indentation,
        // we shouldn't be outputting it at partial end

        out.write( whitespace_ );

        whitespace_.clear();
        state_ = state_passthrough;
    }
}

// state_start_delim consumes the start delimiter ('{{' by default)

boost::core::string_view boost::mustache::renderer::handle_state_start_delim( core::string_view in, output_ref out )
{
    if( in_section_ )
    {
        out = section_text_;
    }

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
        // not a start delimiter, emit consumed input

        out.write( whitespace_ );
        out.write( { start_delim_.data(), delim_index_ } );

        whitespace_.clear();
        state_ = state_passthrough;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

// finish_state_start_delim is called when input ends while in state_start_delim
// e.g. "   {"

void boost::mustache::renderer::finish_state_start_delim( output_ref out )
{
    if( in_section_ )
    {
        in_section_ = false;
    }
    else
    {
        out.write( whitespace_ );
        out.write( { start_delim_.data(), delim_index_ } );
    }

    whitespace_.clear();
    state_ = state_passthrough;
}

// state_tag consumes the tag (the portion between the start delimiter and
//   the end delimiter) and accumulates it into tag_

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

// finish_state_tag is called when input ends while in state_tag
// e.g. "  {{ something"

void boost::mustache::renderer::finish_state_tag( output_ref out )
{
    if( in_section_ )
    {
        in_section_ = false;
    }
    else
    {
        out.write( whitespace_ );
    }

    whitespace_.clear();
    state_ = state_passthrough;
}

static bool is_tag_standalone( boost::core::string_view tag )
{
    tag = trim_leading_whitespace( tag );

    if( tag.empty() )
    {
        return false;
    }

    char ch = tag.front();

    return ch == '!' || ch == '=' || ch == '>' || ch == '#' || ch == '^' || ch == '/';
}

// state_end_delim consumes the end delimiter ('}}' by default)

boost::core::string_view boost::mustache::renderer::handle_state_end_delim( core::string_view in, output_ref out )
{
    output_ref out2( out );

    if( in_section_ )
    {
        out = section_text_;
    }

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

        if( standalone_ && !is_tag_standalone( tag_ ) )
        {
            standalone_ = false;
        }

        if( end_delim_ == "}}" && !tag_.empty() && tag_.front() == '{' )
        {
            state_ = state_end_triple;
        }
        else if( standalone_ )
        {
            state_ = state_standalone;
        }
        else
        {
            out.write( whitespace_ );

            whitespace_.clear();
            state_ = state_passthrough;

            handle_tag( tag_, out2, "" );
            tag_.clear();
        }
    }
    else if( p != end )
    {
        // not an end delimiter

        tag_.append( end_delim_.data(), end_delim_.data() + delim_index_ );

        state_ = state_tag;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

// finish_state_end_delim is called when input ends while in state_end_delim
// e.g. "  {{ something }"

void boost::mustache::renderer::finish_state_end_delim( output_ref out )
{
    if( in_section_ )
    {
        in_section_ = false;
    }
    else
    {
        out.write( whitespace_ );
    }

    whitespace_.clear();
    state_ = state_passthrough;
}

// state_end_triple occurs when the tag is "{{{ something }}"
// we need to check whether it's followed by another '}' in order
// to handle triple mustaches properly

boost::core::string_view boost::mustache::renderer::handle_state_end_triple( core::string_view in, output_ref out )
{
    output_ref out2( out );

    if( in_section_ )
    {
        out = section_text_;
    }

    char const* p = in.data();
    char const* end = p + in.size();

    BOOST_ASSERT( p != end );

    if( *p == '}' )
    {
        ++p;
        tag_.push_back( '}' );
    }

    if( standalone_ && is_tag_standalone( tag_ ) )
    {
        state_ = state_standalone;
    }
    else
    {
        out.write( whitespace_ );

        whitespace_.clear();
        state_ = state_passthrough;

        handle_tag( tag_, out2, "" );
        tag_.clear();
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

// finish_state_end_triple is called when input ends while in state_end_triple
// e.g. "   {{{ x }}"

void boost::mustache::renderer::finish_state_end_triple( output_ref out )
{
    if( in_section_ )
    {
        in_section_ = false;

        whitespace_.clear();
        state_ = state_passthrough;

        return;
    }

    if( standalone_ && is_tag_standalone( tag_ ) )
    {
        finish_state_standalone( out );
    }
    else
    {
        out.write( whitespace_ );

        whitespace_.clear();
        state_ = state_passthrough;

        handle_tag( tag_, out, "" );
        tag_.clear();
    }
}

// state_passthrough consumes literal input and emits it immediately,
// without buffering

boost::core::string_view boost::mustache::renderer::handle_state_passthrough( core::string_view in, output_ref out )
{
    if( in_section_ )
    {
        out = section_text_;
    }

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
            whitespace_ = partial_lwsp_;
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

// finish_state_passthrough is called when input ends while in state_passthrough
// almost nothing to do here, except reset in_section_

void boost::mustache::renderer::finish_state_passthrough( output_ref /*out*/ )
{
    in_section_ = false;
}

// state_standalone occurs at the end of a potentially standalone tag
// e.g. "   {{! standalone tag }}"
// we need to check whether the tag is followed by either \r\n or \n,
// and if so, discard the leading whitespace and the line ending

boost::core::string_view boost::mustache::renderer::handle_state_standalone( core::string_view in, output_ref out )
{
    output_ref out2( out );

    if( in_section_ )
    {
        out = section_text_;
    }

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

        json::string old_wsp( whitespace_, whitespace_.storage() );

        state_ = state_leading_wsp;
        whitespace_ = partial_lwsp_;

        handle_tag( tag_, out2, old_wsp );
        tag_.clear();
    }
    else
    {
        out.write( whitespace_ );

        whitespace_.clear();
        state_ = state_passthrough;

        handle_tag( tag_, out2, "" );
        tag_.clear();
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

// finish_state_standalone is called when input ends while in state_standalone

void boost::mustache::renderer::finish_state_standalone( output_ref out )
{
    json::string old_wsp( whitespace_, whitespace_.storage() );

    state_ = state_leading_wsp;
    whitespace_ = partial_lwsp_;

    handle_tag( tag_, out, old_wsp );
    tag_.clear();

    in_section_ = false;
}

// state_standalone_2 occurs at the end of a potentially standalone tag
// followed by \r

boost::core::string_view boost::mustache::renderer::handle_state_standalone_2( core::string_view in, output_ref out )
{
    output_ref out2( out );

    if( in_section_ )
    {
        out = section_text_;
    }

    char const* p = in.data();
    char const* end = p + in.size();

    BOOST_ASSERT( p != end );

    if( *p == '\n' )
    {
        ++p;

        json::string old_wsp( whitespace_, whitespace_.storage() );

        state_ = state_leading_wsp;
        whitespace_ = partial_lwsp_;

        handle_tag( tag_, out2, old_wsp );
        tag_.clear();
    }
    else
    {
        out.write( whitespace_ );

        whitespace_.clear();
        state_ = state_passthrough;

        handle_tag( tag_, out2, "" );
        tag_.clear();

        out.write( "\r" );
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

// finish_state_standalone_2 is called when input ends while in state_standalone_2

void boost::mustache::renderer::finish_state_standalone_2( output_ref out )
{
    if( in_section_ )
    {
        whitespace_.clear();
        state_ = state_passthrough;

        handle_tag( tag_, out, "" );
        tag_.clear();

        in_section_ = false;
    }
    else
    {
        out.write( whitespace_ );

        whitespace_.clear();
        state_ = state_passthrough;

        handle_tag( tag_, out, "" );
        tag_.clear();

        out.write( "\r" );
    }
}

//

void boost::mustache::renderer::handle_tag( core::string_view tag, output_ref out, core::string_view old_wsp )
{
    char ch = tag.empty()? '\0': tag.front();

    if( in_section_ )
    {
        bool ends = false;

        if( ch == '#' || ch == '^' )
        {
            auto sn = trim_whitespace( tag.substr( 1 ) );
            section_stack_.push_back( sn );
        }
        else if( ch == '/' )
        {
            auto sn = trim_whitespace( tag.substr( 1 ) );

            BOOST_ASSERT( !section_stack_.empty() );

            if( section_stack_.back() == sn )
            {
                section_stack_.pop_back();
            }

            if( section_stack_.empty() )
            {
                ends = true;
            }
        }

        if( !ends )
        {
            if( standalone_ )
            {
                section_text_ += whitespace_;
            }

            section_text_ += start_delim_;
            section_text_ += tag_;
            section_text_ += end_delim_;

            if( standalone_ )
            {
                section_text_ += "\n"; // \r?
            }
        }
        else
        {
            in_section_ = false;
            render_section( out );
        }
    }
    else
    {
        if( ch == '!' )
        {
            tag.remove_prefix( 1 );
            return handle_comment_tag( tag, out );
        }

        if( ch == '>' )
        {
            tag.remove_prefix( 1 );
            return handle_partial_tag( tag, out, old_wsp );
        }

        if( ch == '#' )
        {
            tag.remove_prefix( 1 );
            return handle_section_tag( tag, out, false );
        }

        if( ch == '^' )
        {
            tag.remove_prefix( 1 );
            return handle_section_tag( tag, out, true );
        }

        if( ch == '&' )
        {
            tag.remove_prefix( 1 );
            return handle_interpolation_tag( tag, out, false );
        }

        if( ch == '=' && tag.size() >= 2 && tag.back() == '=' )
        {
            tag.remove_prefix( 1 );
            tag.remove_suffix( 1 );

            return handle_delimiter_tag( tag, out );
        }

        if( ch == '{' && tag.size() >= 2 && tag.back() == '}' )
        {
            tag.remove_prefix( 1 );
            tag.remove_suffix( 1 );

            return handle_interpolation_tag( tag, out, false );
        }

        return handle_interpolation_tag( tag, out, true );
    }
}

void boost::mustache::renderer::handle_comment_tag( core::string_view /*tag*/, output_ref /*out*/ )
{
    // do nothing
}

static void quoted_write( boost::core::string_view sv, boost::mustache::output_ref out )
{
    char const* p = sv.data();
    char const* end = p + sv.size();

    while( p != end )
    {
        if( *p == '<' )
        {
            out.write( "&lt;" );
        }
        else if( *p == '>' )
        {
            out.write( "&gt;" );
        }
        else if( *p == '"' )
        {
            out.write( "&quot;" );
        }
        else if( *p == '&' )
        {
            out.write( "&amp;" );
        }
        else
        {
            out.write( { p, 1 } );
        }

        ++p;
    }
}

static void maybe_quoted_write( boost::core::string_view sv, boost::mustache::output_ref out, bool quoted )
{
    if( quoted )
    {
        quoted_write( sv, out );
    }
    else
    {
        out.write( sv );
    }
}

static void gcvt( char* buffer, std::size_t size, double v )
{
    std::size_t n = std::snprintf( buffer, size, "%.16g", v );

    char dp = std::localeconv()->decimal_point[0];

    if( dp != '.' )
    {
        std::replace( buffer, buffer + std::min( n, size ), dp, '.' );
    }
}

static void output_value( boost::json::value const& jv, boost::mustache::output_ref out, bool quoted )
{
    switch( jv.kind() )
    {
    case boost::json::kind::null:

        break;

    case boost::json::kind::bool_:

        maybe_quoted_write( jv.get_bool()? "true": "false", out, quoted );
        break;

    case boost::json::kind::int64:

        {
            char buffer[ 32 ];
            std::snprintf( buffer, sizeof( buffer ), "%lld", static_cast<long long>( jv.get_int64() ) );

            maybe_quoted_write( buffer, out, quoted );
        }

        break;

    case boost::json::kind::uint64:

        {
            char buffer[ 32 ];
            std::snprintf( buffer, sizeof( buffer ), "%llu", static_cast<unsigned long long>( jv.get_uint64() ) );

            maybe_quoted_write( buffer, out, quoted );
        }

        break;

    case boost::json::kind::double_:

        {
            char buffer[ 32 ] = {};
            gcvt( buffer, sizeof( buffer ), jv.get_double() );

            maybe_quoted_write( buffer, out, quoted );
        }

        break;

    case boost::json::kind::string:

        maybe_quoted_write( jv.get_string(), out, quoted );
        break;

    case boost::json::kind::array:
    case boost::json::kind::object:
    default:

        {
            boost::json::serializer sr; // sp?
            sr.reset( &jv );

            while( !sr.done() )
            {
                char buffer[ 32 ];
                maybe_quoted_write( sr.read( buffer ), out, quoted );
            }
        }

        break;
    }
}

void boost::mustache::renderer::handle_interpolation_tag( core::string_view tag, output_ref out, bool quoted )
{
    tag = trim_whitespace( tag );

    json::value const* p = lookup_value( tag );

    if( p == 0 )
    {
        return;
    }

    output_value( *p, out, quoted );
}

void boost::mustache::renderer::handle_section_tag( core::string_view tag, output_ref /*out*/, bool inverted )
{
    tag = trim_whitespace( tag );

    section_stack_.clear();
    section_stack_.push_back( tag );

    inverted_ = inverted;
    section_context_ = lookup_value( tag );

    section_text_.clear();

    in_section_ = true;
}

static boost::core::string_view get_leading_token( boost::core::string_view sv )
{
    char const* p = sv.data();
    char const* end = p + sv.size();

    while( p != end && ( *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' ) )
    {
        ++p;
    }

    return { sv.data(), static_cast<std::size_t>( p - sv.data() ) };
}

void boost::mustache::renderer::handle_delimiter_tag( core::string_view tag, output_ref /*out*/ )
{
    tag = trim_leading_whitespace( tag );

    auto d1 = get_leading_token( tag );
    tag.remove_prefix( d1.size() );

    tag = trim_leading_whitespace( tag );

    auto d2 = get_leading_token( tag );
    tag.remove_prefix( d2.size() );

    tag = trim_leading_whitespace( tag );

    if( d1.empty() || d2.empty() || !tag.empty() )
    {
        // the tag contents must be exactly two non-whitespace sequences
        return;
    }

    start_delim_ = d1;
    end_delim_ = d2;
}

void boost::mustache::renderer::handle_partial_tag( core::string_view tag, output_ref out, core::string_view old_wsp )
{
    tag = trim_whitespace( tag );

    if( json::value const* p1 = partials_.if_contains( tag ) )
    {
        if( json::string const* p2 = p1->if_string() )
        {
            json::string old_start_delim( start_delim_, start_delim_.storage() );
            json::string old_end_delim( end_delim_, end_delim_.storage() );

            start_delim_ = "{{";
            end_delim_ = "}}";

            json::string old_partial_lwsp( partial_lwsp_, partial_lwsp_.storage() );
            partial_lwsp_ = old_wsp;

            if( state_ == state_leading_wsp )
            {
                whitespace_ = partial_lwsp_;
            }

            render_some( *p2, out );
            finish( out );

            if( state_ == state_leading_wsp && whitespace_ == partial_lwsp_ )
            {
                whitespace_ = old_partial_lwsp;
            }

            start_delim_ = old_start_delim;
            end_delim_ = old_end_delim;
            partial_lwsp_ = old_partial_lwsp;
        }
    }
}

boost::json::value const* boost::mustache::renderer::lookup_value( core::string_view name ) const
{
    if( name == "." )
    {
        return &context_stack_.back();
    }

    std::size_t i = name.find( '.' );

    core::string_view n = name.substr( 0, i );

    boost::json::value const* r = 0;
    std::size_t j = context_stack_.size();

    while( j > 0 )
    {
        --j;

        if( auto const* p = context_stack_[ j ].if_object() )
        {
            if( p->contains( n ) )
            {
                r = &p->at( n );
                break;
            }
        }
    }

    if( r == 0 )
    {
        return r;
    }

    if( i == core::string_view::npos )
    {
        return r;
    }

    for( ;; )
    {
        name.remove_prefix( i + 1 );

        i = name.find( '.' );
        n = name.substr( 0, i );

        auto const* p = r->if_object();

        if( p && p->contains( n ) )
        {
            r = &p->at( n );
        }
        else
        {
            return 0;
        }

        if( i == core::string_view::npos )
        {
            return r;
        }
    }
}

//

static bool is_value_true( boost::json::value const & jv )
{
    switch( jv.kind() )
    {
    case boost::json::kind::null:

        return false;

    case boost::json::kind::bool_:

        return jv.get_bool();

    case boost::json::kind::int64:

        return jv.get_int64() != 0;

    case boost::json::kind::uint64:

        return jv.get_uint64() != 0;

    case boost::json::kind::double_:

        return jv.get_double() != 0;

    case boost::json::kind::string:

        return !jv.get_string().empty();

    case boost::json::kind::array:

        return !jv.get_array().empty();

    case boost::json::kind::object:

        return true;

    default:

        BOOST_ASSERT( false );
        return true;
    }
}

void boost::mustache::renderer::render_section( output_ref out )
{
    json::value const * p = section_context_;

    if( inverted_ )
    {
        if( p == 0 || !is_value_true( *p ) )
        {
            json::string tmp( section_text_, section_text_.storage() );

            render_some( tmp, out );
            finish( out );
        }
    }
    else
    {
        if( p != 0 && is_value_true( *p ) )
        {
            json::string tmp( section_text_, section_text_.storage() );
            json::value const jv( *p, p->storage() );

            if( jv.is_array() )
            {
                context_stack_.push_back( {} );

                auto st = state_;

                for( auto const& item: jv.get_array() )
                {
                    context_stack_.back() = item;

                    render_some( tmp, out );
                    finish( out );

                    state_ = st;
                }

                context_stack_.pop_back();
            }
            else
            {
                context_stack_.push_back( jv );

                render_some( tmp, out );
                finish( out );

                context_stack_.pop_back();
            }
        }
    }
}

//

void boost::mustache::renderer::finish( output_ref out )
{
    switch( state_ )
    {
    case state_leading_wsp:

        finish_state_leading_wsp( out );
        break;

    case state_start_delim:

        finish_state_start_delim( out );
        break;

    case state_tag:

        finish_state_tag( out );
        break;

    case state_end_delim:

        finish_state_end_delim( out );
        break;

    case state_end_triple:

        finish_state_end_triple( out );
        break;

    case state_passthrough:

        finish_state_passthrough( out );
        break;

    case state_standalone:

        finish_state_standalone( out );
        break;

    case state_standalone_2:

        finish_state_standalone_2( out );
        break;

    default:

        BOOST_ASSERT( false );
        return;
    }
}
