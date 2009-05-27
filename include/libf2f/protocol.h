#ifndef __F2F_PROTOCOL_H__
#define __F2F_PROTOCOL_H__

#include "libf2f/router.h"
#include "libf2f/connection.h"

class Protocol
{
public:
    /// called when a client connects to us
    virtual bool new_incoming_connection( connection_ptr conn );

    /// called when we opened a socket to a remote servent
    virtual void new_outgoing_connection( connection_ptr conn );

    /// we received a msg from this connection
    virtual void message_received( message_ptr msgp, connection_ptr conn );
    
    
};
#endif
