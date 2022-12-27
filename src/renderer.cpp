// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/renderer.hpp>
#include <utility>

boost::mustache::renderer::renderer( json::value&& data, json::object&& partials, json::storage_ptr sp ):
    context_stack_( sp ), partials_( std::move( partials ), sp )
{
    context_stack_.push_back( std::move( data ) );
}

boost::mustache::renderer::~renderer()
{
}

void boost::mustache::renderer::render( core::string_view tmpl, output_ref out )
{
    out.write( tmpl );
}

void boost::mustache::renderer::finish( output_ref /*out*/ )
{
}
