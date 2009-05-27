#include "libf2f/protocol.h"

using namespace std;

bool
Protocol::new_incoming_connection( connection_ptr conn )
{
    cout << "Protocol::new_incoming_connection " << conn->str() << endl;
    conn->async_write( PingMessage::factory() );
    // first thing to expect is an ident msg, so set the msg handler to one 
    // that expects it, and kills the connection otherwise:
    
    return true;
}

void 
Protocol::new_outgoing_connection( connection_ptr conn )
{
    cout << "Protocol::new_outgoing_connection " << conn->str() << endl;
    conn->async_write( PingMessage::factory() );
}

void 
Protocol::message_received( message_ptr msgp, connection_ptr conn )
{
    cout << "Protocol::message_received " << conn->str() << endl 
         << msgp->str() << endl;
}