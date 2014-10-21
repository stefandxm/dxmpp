//
//  SaslChallengeParser.cpp
//  DXMPP
//
//  Created by Stefan Marton on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//


#include "SaslChallengeParser.hpp"

namespace DXMPP
{
    namespace SASL
    {
        bool ParseSASLChallenge(const std::string kv_string, std::map<std::string,std::string> & results)
        {
            using boost::spirit::ascii::space;
            typedef std::string::const_iterator iterator_type;
            typedef DXMPP::SASL::kvpair_parser<iterator_type> kvpair_parser;
            
            kvpair_parser p;
            std::string::const_iterator iter = kv_string.begin();
            std::string::const_iterator end = kv_string.end();
            bool r = phrase_parse(iter, end, p, space, results);
            r = r && (iter==end);
            return r;
        }
    }
}
