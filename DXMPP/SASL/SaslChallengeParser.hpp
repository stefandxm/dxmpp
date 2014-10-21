//
//  SaslChallengeParser.hpp
//  DXMPP
//
//  Created by Stefan Marton on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//


#ifndef SASL_MANDLEBRASL_PARSER_HPP
#define SASL_MANDLEBRASL_PARSER_HPP

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/io.hpp>

#include <iostream>
#include <string>
#include <map>
#include <utility>

namespace DXMPP
{
    namespace SASL
    {

        namespace qi = boost::spirit::qi;
        namespace ascii = boost::spirit::ascii;

        template <typename Iterator>
            struct kvpair_parser : qi::grammar<Iterator, std::map<std::string, std::string>(), ascii::space_type>
        {
            kvpair_parser() : kvpair_parser::base_type(start)
            {
                using qi::int_;
                using qi::lit;
                using qi::lexeme;
                using ascii::char_;

                key_string %= lexeme[ qi::char_("a-zA-Z_") >> *qi::char_("a-zA-Z_0-9") ];
                value_string %= lexeme[ +qi::char_("/+=a-zA-Z_0-9\x2d") ];
                quoted_string %= lexeme[ '"' >> +(char_ - '"') >> '"' ];

                pair %= key_string >> qi::lit('=') >> ( value_string | quoted_string );

                start %= pair >> *(qi::lit(',') >> pair);
            }

            qi::rule<Iterator, std::pair<std::string,std::string>(), ascii::space_type> pair;
            qi::rule<Iterator, std::string(), ascii::space_type> key_string, value_string, quoted_string;
            qi::rule<Iterator, std::map<std::string, std::string>(), ascii::space_type> start;
        };

        extern bool ParseSASLChallenge(const std::string kv_string, std::map<std::string,std::string> & results);
    }
}

#endif
