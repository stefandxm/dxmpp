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
        
        ~JID()
        {
        }
        
        JID()
        {
        }

        JID(std::string Username, 
            std::string Domain,
            std::string &Resource)
            :
              Username(Username),
              Domain(Domain),
              Resource(Resource)
        {
        }
        JID(const std::string &FullJID)
        {           
            LoadJID(FullJID);
        }

        JID(const std::string &Bare, std::string Resource)
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
        
        void SetResource(std::string Resource) 
        {
            this->Resource = Resource;
        }

        std::string GetFullJID()
        {
            if (Resource.empty() && Username.empty())
                return Domain;
            if (Resource.empty())
                return Username + "@" + Domain;
            if (Username.empty())
                return Domain + "/" + Resource;
            return Username + "@" + Domain + "/" + Resource;
        }
        
        std::string GetBareJID()
        {
            if (Username.empty())
                return Domain;
            return Username + "@" + Domain;
        }
        
        std::string GetUsername() 
        {
            return Username;
        }
        
        std::string GetDomain() 
        {
            return Domain;
        }

        std::string GetResource() 
        {
            return Resource;
        }
    };
}


#endif
