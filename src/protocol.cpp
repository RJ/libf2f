#include "libf2f/protocol.h"

namespace libf2f {

using namespace std;

Protocol::Protocol()
{
}

bool
Protocol::new_incoming_connection( connection_ptr conn )
{
    cout << "Protocol::new_incoming_connection " << conn->str() << endl;
    return true;
}

void
Protocol::new_outgoing_connection( connection_ptr conn )
{
    cout << "Protocol::new_outgoing_connection " << conn->str() << endl;
}

void
Protocol::message_received( message_ptr msgp, connection_ptr conn )
{
    cout << "Protocol::message_received " << conn->str() << endl
         << msgp->str() << endl;
}

} //ns
