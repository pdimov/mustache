#ifndef BOOST_MUSTACHE_CONFIG_HPP_INCLUDED
#define BOOST_MUSTACHE_CONFIG_HPP_INCLUDED

// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>

// This header implements separate compilation features as described in
// http://www.boost.org/more/separate_compilation.html

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_MUSTACHE_DYN_LINK)
# if defined(BOOST_MUSTACHE_SOURCE)
#  define BOOST_MUSTACHE_DECL BOOST_SYMBOL_EXPORT
# else
#  define BOOST_MUSTACHE_DECL BOOST_SYMBOL_IMPORT
# endif
#else
# define BOOST_MUSTACHE_DECL
#endif

// Autolink

#if !defined(BOOST_MUSTACHE_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_MUSTACHE_NO_LIB)

#define BOOST_LIB_NAME boost_mustache

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_MUSTACHE_DYN_LINK)
# define BOOST_DYN_LINK
#endif

#include <boost/config/auto_link.hpp>

#endif

#endif // BOOST_MUSTACHE_CONFIG_HPP_INCLUDED
