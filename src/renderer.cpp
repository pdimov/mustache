// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache/renderer.hpp>

boost::mustache::renderer::renderer( json::value const& /*data*/, json::value const& /*partials*/ )
{
}

boost::mustache::renderer::~renderer()
{
}

void boost::mustache::renderer::render( core::string_view /*tmpl*/, output_ref /*out*/ )
{
}

void boost::mustache::renderer::finish( output_ref /*out*/ )
{
}
