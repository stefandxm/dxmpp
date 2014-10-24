//
//  JID.hpp
//  DXMPP
//
//  Created by Stefan Karlsson on 31/05/14.
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

#ifndef DXMPP_JID_hpp
#define DXMPP_JID_hpp

#include <string>

namespace DXMPP {
    class JID
    {
        std::string Username;
        std::string Domain;
        std::string Resource;

        void LoadJID(const std::string &JID)
        {
            size_t IndexOfSlash = JID.find('/');
            if(IndexOfSlash != std::string::npos)
            {
                Resource = JID.substr(IndexOfSlash+1);
            }
            size_t IndexOfAt = JID.find('@');
            if(IndexOfAt != std::string::npos && IndexOfAt < IndexOfSlash)
            {
                Username = JID.substr(0, IndexOfAt);
                IndexOfAt++;
            }
            else
                IndexOfAt = 0;
            Domain = JID.substr(IndexOfAt, IndexOfSlash-IndexOfAt);
        }
        
    public:
        JID()
        {
        }

        JID(const std::string &Username, const std::string &Domain, const std::string &Resource)
        {
            this->Username = Username;
            this->Domain = Domain;
            this->Resource = Resource;
        }
        JID(const std::string &FullJID)
        {           
            LoadJID(FullJID);
        }

        JID(const std::string &Bare, const std::string &Resource)
        {           
            LoadJID(Bare);
            this->Resource = Resource;
        }
        
        JID(const JID &B)
        {
            this->Username = B.Username;
            this->Domain = B.Domain;
            this->Resource = B.Resource;
        }
        
        void SetResource(const std::string &Resource) 
        {
            this->Resource = Resource;
        }

        const std::string GetFullJID() const 
        {
            if (Resource.empty() && Username.empty())
                return Domain;
            if (Resource.empty())
                return Username + "@" + Domain;
            if (Username.empty())
                return Domain + "/" + Resource;
            return Username + "@" + Domain + "/" + Resource;
        }
        
        const std::string GetBareJID() const 
        {
            if (Username.empty())
                return Domain;
            return Username + "@" + Domain;
        }
        
        const std::string GetUsername() const 
        {
            return Username;
        }
        
        const std::string GetDomain() const 
        {
            return Domain;
        }

        const std::string GetResource() const 
        {
            return Resource;
        }
    };
}


#endif
