#ifndef __F2FDEMO_PROTOCOL_H__
#define __F2FDEMO_PROTOCOL_H__

#include "libf2f/protocol.h"
#include "demo_messages.h"

using namespace libf2f;

class DemoProtocol : public Protocol
{
public:
    DemoProtocol()
    {
    }
    
    virtual ~DemoProtocol()
    {
    }

    /// called when a client connects to us
    virtual bool new_incoming_connection( connection_ptr conn )
    {
        std::cout << "DemoProtocol::new_incoming_connection " 
                  << conn->str() << std::endl;
        return true; // returning false rejects the connection
    }

    /// called when we opened a socket to a remote servent
    virtual void new_outgoing_connection( connection_ptr conn )
    {
        std::cout << "DemoProtocol::new_outgoing_connection " 
                  << conn->str() << std::endl;
    }

    /// called on a disconnection, for whatever reason
    virtual void connection_terminated( connection_ptr conn )
    {
        std::cout << "Connection terminated!" << std::endl;
    }

    /// we received a msg from this connection
    virtual void message_received( message_ptr msgp, connection_ptr conn )
    {
        std::cout << "DemoProtocol::message_received " 
                  << conn->str() << std::endl 
                  << msgp->str() << std::endl;
        switch( msgp->type() )
        {
            case PING:
                std::cout << "Got a ping, replying with a pong." << std::endl;
                conn->async_write( message_ptr(new PongMessage( m_router->gen_uuid() )) );
                break;
            case PONG:
                std::cout << "Got a pong, yay!" << std::endl;
                break;
            default:
                std::cout << "Unhandled message type: " 
                          << msgp->type() << std::endl;
        }
    }

};

#endif

