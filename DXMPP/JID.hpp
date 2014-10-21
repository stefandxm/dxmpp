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
            size_t IndexOfAt = JID.find('@');
            size_t IndexOfSlash = JID.find('/');

            if(IndexOfAt == std::string::npos)
            {
                Domain = JID;
                return;
            }

            Username = JID.substr(0, IndexOfAt);

            if(IndexOfSlash == std::string::npos)
            {
                Domain = JID.substr(IndexOfAt+1, JID.length()-IndexOfAt-1);
                return;
            }
            Domain = JID.substr(IndexOfAt+1, IndexOfSlash-IndexOfAt-1);
            Resource = JID.substr(IndexOfSlash+1, JID.length()-IndexOfSlash-1);

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
            std::string rVal = Username + "@" + Domain + "/" + Resource;
            
            return rVal;
        }
        
        const std::string GetBareJID() const 
        {
            std::string rVal = Username + "@" + Domain;
            
            return rVal;
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
