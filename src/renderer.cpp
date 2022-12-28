// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/renderer.hpp>
#include <boost/json/serializer.hpp>
#include <boost/assert.hpp>
#include <utility>

boost::mustache::renderer::renderer( json::value&& data, json::object&& partials, json::storage_ptr sp ):
    context_stack_( sp ), partials_( std::move( partials ), sp ),
    whitespace_( sp ), start_delim_( "{{", sp ), end_delim_( "}}", sp ),
    tag_( sp ), section_stack_( sp ), section_text_( sp )
{
    context_stack_.push_back( std::move( data ) );
}

boost::mustache::renderer::~renderer()
{
}

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

        case state_section:

            in = handle_state_section( in, out );
            break;

        case state_section_start_delim:

            in = handle_state_section_start_delim( in, out );
            break;

        case state_section_tag:

            in = handle_state_section_tag( in, out );
            break;

        case state_section_end_delim:

            in = handle_state_section_end_delim( in, out );
            break;

        case state_section_end_triple:

            in = handle_state_section_end_triple( in, out );
            break;

        default:

            BOOST_ASSERT( false );
            return;
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

        out.write( whitespace_ );
        whitespace_.clear();

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

static bool is_tag_standalone( boost::core::string_view tag )
{
    if( tag.empty() )
    {
        return false;
    }

    char ch = tag.front();

    return ch == '!' || ch == '=' || ch == '>' || ch == '#' || ch == '^';
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

        if( end_delim_ == "}}" && !tag_.empty() && tag_.front() == '{' )
        {
            state_ = state_end_triple;
        }
        else if( standalone_ && is_tag_standalone( tag_ ) )
        {
            state_ = state_standalone;
        }
        else
        {
            out.write( whitespace_ );
            whitespace_.clear();

            state_ = state_passthrough;

            handle_tag( tag_, out );
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

boost::core::string_view boost::mustache::renderer::handle_state_end_triple( core::string_view in, output_ref out )
{
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

        handle_tag( tag_, out );
        tag_.clear();
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
            whitespace_.clear();
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

boost::core::string_view boost::mustache::renderer::handle_state_standalone( core::string_view in, output_ref out )
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
        whitespace_.clear();

        state_ = state_leading_wsp;

        handle_tag( tag_, out );
        tag_.clear();
    }
    else
    {
        out.write( whitespace_ );
        whitespace_.clear();

        state_ = state_passthrough;

        handle_tag( tag_, out );
        tag_.clear();
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
        whitespace_.clear();

        state_ = state_leading_wsp;

        handle_tag( tag_, out );
        tag_.clear();
    }
    else
    {
        out.write( whitespace_ );
        whitespace_.clear();

        state_ = state_passthrough;

        handle_tag( tag_, out );
        tag_.clear();

        out.write( "\r" );
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_section( core::string_view in, output_ref /*out*/ )
{
    char const* p = in.data();
    char const* end = p + in.size();

    while( p != end && *p != start_delim_[0] )
    {
        ++p;
    }

    section_text_.append( in.data(), p );

    if( p != end )
    {
        state_ = state_section_start_delim;
        delim_index_ = 0;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_section_start_delim( core::string_view in, output_ref /*out*/ )
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

        state_ = state_section_tag;
        tag_.clear();
    }
    else if( p != end )
    {
        // not a start delimiter

        section_text_.append( start_delim_.data(), start_delim_.data() + delim_index_ );

        state_ = state_section;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_section_tag( core::string_view in, output_ref /*out*/ )
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
        state_ = state_section_end_delim;
        delim_index_ = 0;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

static bool is_section_tag( boost::core::string_view tag )
{
    return !tag.empty() && ( tag.front() == '#' || tag.front() == '^' );
}

static bool is_end_section_tag( boost::core::string_view tag )
{
    return !tag.empty() && tag.front() == '/';
}

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

void boost::mustache::renderer::handle_state_section_end_delim_( output_ref out )
{
    bool ends = false;

    if( is_section_tag( tag_ ) )
    {
        auto sn = trim_whitespace( tag_.subview( 1 ) );

        section_stack_.push_back( sn );
    }
    else if( is_end_section_tag( tag_ ) )
    {
        auto sn = trim_whitespace( tag_.subview( 1 ) );

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
        section_text_.append( start_delim_ );
        section_text_.append( tag_ );
        section_text_.append( end_delim_ );

        state_ = state_section;
    }
    else
    {
        state_ = state_passthrough;

        render_section( out );

        state_ = state_passthrough;
    }
}

boost::core::string_view boost::mustache::renderer::handle_state_section_end_delim( core::string_view in, output_ref out )
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

        if( end_delim_ == "}}" && !tag_.empty() && tag_.front() == '{' )
        {
            state_ = state_section_end_triple;
        }
        else
        {
            handle_state_section_end_delim_( out );
        }
    }
    else if( p != end )
    {
        // not an end delimiter

        tag_.append( end_delim_.data(), end_delim_.data() + delim_index_ );

        state_ = state_section_tag;
    }

    return { p, static_cast<std::size_t>( end - p ) };
}

boost::core::string_view boost::mustache::renderer::handle_state_section_end_triple( core::string_view in, output_ref out )
{
    char const* p = in.data();
    char const* end = p + in.size();

    BOOST_ASSERT( p != end );

    if( *p == '}' )
    {
        ++p;
        tag_.push_back( '}' );
    }

    handle_state_section_end_delim_( out );

    return { p, static_cast<std::size_t>( end - p ) };
}

//

void boost::mustache::renderer::handle_tag( core::string_view tag, output_ref out )
{
    if( tag.empty() )
    {
        return handle_interpolation_tag( tag, out, true );
    }

    char ch = tag.front();

    if( ch == '!' )
    {
        tag.remove_prefix( 1 );
        return handle_comment_tag( tag, out );
    }

    if( ch == '>' )
    {
        tag.remove_prefix( 1 );
        return handle_partial_tag( tag, out );
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
            char buffer[ 32 ];
            std::snprintf( buffer, sizeof( buffer ), "%.16g", jv.get_double() );

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

    state_ = state_section;
}

void boost::mustache::renderer::handle_delimiter_tag( core::string_view /*tag*/, output_ref /*out*/ )
{
}

void boost::mustache::renderer::handle_partial_tag( core::string_view /*tag*/, output_ref /*out*/ )
{
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
        }
    }
    else if( p != 0 )
    {
        json::value const& jv = *p;

        if( is_value_true( jv ) )
        {
            json::string tmp( section_text_, section_text_.storage() );

            if( jv.is_array() )
            {
                context_stack_.push_back( {} );

                for( auto const& item: jv.get_array() )
                {
                    context_stack_.back() = item;
                    render_some( tmp, out );
                }

                context_stack_.pop_back();
            }
            else
            {
                context_stack_.push_back( jv );

                render_some( tmp, out );

                context_stack_.pop_back();
            }
        }
    }
}

//

void boost::mustache::renderer::finish( output_ref /*out*/ )
{
}
