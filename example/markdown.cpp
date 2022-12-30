// Copyright 2022 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/mustache.hpp>
#include <boost/describe.hpp>
#include <vector>
#include <string>
#include <iostream>

struct item
{
    std::string title;
    std::string author;
    std::string link;
};

BOOST_DESCRIBE_STRUCT(item, (), (title, author, link))

struct reference
{
    std::string heading;
    std::vector<item> items;
};

BOOST_DESCRIBE_STRUCT(reference, (), (heading, items))

int main()
{
    reference ref =
    {
        "Reference",
        {
            {
                "Better Bit Mixing - Improving on MurmurHash3's 64-bit Finalizer",
                "David Stafford",
                "https://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html"
            },
            {
                "Stronger, better, morer, Moremur; a better Murmur3-type mixer",
                "Pelle Evensen",
                "https://mostlymangling.blogspot.com/2019/12/stronger-better-morer-moremur-better.html"
            },
            {
                "Improved mx3 and the RRC test",
                "Jon Maiga",
                "http://jonkagstrom.com/mx3/mx3_rev2.html"
            }
        }
    };

    std::string tmpl =
R"(# {{{heading}}}

{{#items}}
* __{{{title}}}__
_{{{author}}}_
[{{{link}}}]({{{link}}})

{{/items}})";

    boost::mustache::render( tmpl, std::cout, ref, {} );
}
