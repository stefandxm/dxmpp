//
//  CryptoPP_byte.cpp
//  DXMPP
//
//  Created by Stefan Marton 2020
//  Copyright (c) 2014 Deus ex Machinae. All rights reserved.
//

// Work around Crypto++ API incompatibility between 5.x and later versions

#include <cryptopp/config.h>

#if (CRYPTOPP_VERSION < 600)
namespace CryptoPP {
  typedef unsigned char byte;
}
#endif
