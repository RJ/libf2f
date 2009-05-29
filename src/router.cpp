#include "libf2f/router.h"
#include "libf2f/connection.h"
#include "libf2f/protocol.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>

// How we typically prep a new connection object:
#define NEWCONN new Connection(m_acceptor->io_service(), \
                boost::bind( &Router::message_received, this, _1, _2 ), \
                boost::bind( &Router::connection_terminated, this, _1) )

namespace libf2f {

using namespace std;

Router::Router( boost::shared_ptr<boost::asio::ip::tcp::acceptor> accp,
                Protocol * p )
    : m_acceptor( accp ), m_protocol( p ), seen_connections(0)
{
    p->set_router( this );
    // Start an accept operation for a new connection.
    connection_ptr new_conn(NEWCONN);
    
    m_acceptor->async_accept(new_conn->socket(),
        boost::bind(&Router::handle_accept, this,
        boost::asio::placeholders::error, new_conn));

}

void
Router::stop()
{
    m_acceptor->get_io_service().stop();
}

void
Router::connection_terminated( connection_ptr conn )
{
    unregister_connection( conn );
    m_protocol->connection_terminated( conn );
}

/// Handle completion of a accept operation.
void 
Router::handle_accept(const boost::system::error_code& e, connection_ptr conn)
{
    if(e)
    {
        // Log it and return. Since we are not starting a new
        // accept operation the io_service will run out of work to do and the
        // Servent will exit.
        std::cerr << e.message() << std::endl;
        return;
    }
    if( !m_protocol->new_incoming_connection(conn) )
    {
        cout << "Rejecting connection " << conn->str() << endl;
        // don't register it (so it autodestructs)
    }
    else
    {
        register_connection( conn );
        conn->async_read();
    }
    
    // Start an accept operation for a new connection.
    connection_ptr new_conn(NEWCONN);
    
    m_acceptor->async_accept(new_conn->socket(),
        boost::bind(&Router::handle_accept, this,
            boost::asio::placeholders::error, new_conn));
}

void
Router::register_connection( connection_ptr conn )
{
    boost::mutex::scoped_lock lk(m_connections_mutex);
    vector<connection_ptr>::iterator it;
    for( it=m_connections.begin() ; it < m_connections.end() ; ++it )
    {
        if( *it == conn )
        {
            // already registered, wtf?
            cout << "ERROR connection already registered!" << endl;
            assert(false);
            return;
        }
    }
    m_connections.push_back( conn );
    cout << connections_str() << endl;
}

void
Router::unregister_connection( connection_ptr conn )
{
    boost::mutex::scoped_lock lk(m_connections_mutex);
    vector<connection_ptr>::iterator it;
    for( it=m_connections.begin() ; it < m_connections.end() ; ++it )
    {
        if( *it == conn )
        {
            m_connections.erase( it );
            cout << "Router::unregistered " << conn->str() << endl;
        }
    }
    cout << connections_str() << endl;
}

string
Router::connections_str()
{
    ostringstream os;
    os << "<connections>" << endl;
    BOOST_FOREACH( connection_ptr conn, m_connections )
    {
        os << conn->str() << endl;
    }
    os << "</connections>" << endl;
    return os.str();
}

/// this is the default msg recvd callback passed to new connections:
void
Router::message_received( message_ptr msgp, connection_ptr conn )
{
    cout << "router::message_received from " << conn->str() 
         << " " << msgp->str() << endl;
    if( msgp->hops() > 3 )
    {
        cout << "Dropping, hop count: " << msgp->hops() << endl;
        return;
    }
    if( msgp->length() > 10240 ) // 10k hard limit
    {
        cout << "Dropping, msg length: " << msgp->length() << endl;
        return;
    }
    
    // handle ping
    if( msgp->type() == PING )
    {
        cout << "got PING, responding.." << endl;
        conn->async_write( message_ptr(new PongMessage()) );
        return;
    }
    
    // handle pong
    if( msgp->type() == PONG )
    {
        cout << "got PONG" << endl;
        return;
    }
    
    m_protocol->message_received( msgp, conn );
}

/// Connect out to a remote Servent at endpoint
void 
Router::connect_to_remote(boost::asio::ip::tcp::endpoint &endpoint)
{
    cout << "connect_to_remote(" << endpoint.address().to_string()<<","
         << endpoint.port()<<")" << endl;
    connection_ptr new_conn(NEWCONN);
    // Start an asynchronous connect operation.
    new_conn->socket().async_connect(endpoint,
        boost::bind(&Router::handle_connect, this,
        boost::asio::placeholders::error, endpoint, new_conn));
}

/// Handle completion of a connect operation.
void 
Router::handle_connect( const boost::system::error_code& e,
                        boost::asio::ip::tcp::endpoint &endpoint,
                        connection_ptr conn )
{
    if (e)
    {
        std::cerr   << "Failed to connect out to remote Servent: " 
                    << e.message() << std::endl;
        return;
    }
    /// Successfully established connection. 
    m_protocol->new_outgoing_connection( conn );
    register_connection( conn );
    conn->async_read(); // start read loop for this connection
}

/// apply fun to all connections
void 
Router::foreach_conns( boost::function<void(connection_ptr)> fun )
{
    boost::mutex::scoped_lock lk(m_connections_mutex);
    BOOST_FOREACH( connection_ptr conn, m_connections )
    {
        fun( conn );
    }
}



void 
Router::foreach_conns_except( boost::function<void(connection_ptr)> fun, connection_ptr conn )
{
    boost::mutex::scoped_lock lk(m_connections_mutex);
    BOOST_FOREACH( connection_ptr c, m_connections )
    {
        if( c == conn ) continue;
        fun( c );
    }
}

void 
Router::send_all( message_ptr msgp )
{
    foreach_conns( boost::bind(&Connection::async_write, _1, msgp) );
}



} //ns
