#ifndef __LIBF2F_DEMO_MESSAGE_H__
#define __LIBF2F_DEMO_MESSAGE_H__

#include "libf2f/message.h"

using namespace libf2f;

#define PING            0
#define PONG            1
#define QUERY           2

class PongMessage : public Message
{
public:
    PongMessage( std::string uid )
    {
        message_header h;
        memcpy( &h.guid, uid.c_str(), 36 );
        h.type = PONG;
        h.ttl  = 1;
        h.hops = 0;
        h.length = 0;
        m_header = h;
        m_payload = 0;
    }
};

class PingMessage : public Message
{
public:
    PingMessage( std::string uid )
    {
        message_header h;
        memcpy( &h.guid, uid.c_str(), 36 );
        h.type = PING;
        h.ttl  = 1;
        h.hops = 0;
        h.length = 0;
        m_header = h;
        m_payload = 0;
    }
};

#endif

