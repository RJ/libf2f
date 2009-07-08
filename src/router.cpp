#include "libf2f/router.h"
#include "libf2f/connection.h"
#include "libf2f/protocol.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>

namespace libf2f {

using namespace std;

Router::Router( boost::shared_ptr<boost::asio::ip::tcp::acceptor> accp,
                Protocol * p, boost::function<std::string()> uuidf )
    :   m_acceptor( accp ),
        m_protocol( p ),
        seen_connections(0),
        m_uuidgen( uuidf )
{
    cout << "Testing uuid generator... " << flush;
    string uuid = m_uuidgen();
    if( uuid.length() != 36 )
    {
        cout << "ERROR length must be 36." << endl;
        throw;
    }
    cout << "OK" << endl;
    p->set_router( this );
    // Start an accept operation for a new connection.
    connection_ptr new_conn = new_connection();
    
    m_acceptor->async_accept(new_conn->socket(),
        boost::bind(&Router::handle_accept, this,
        boost::asio::placeholders::error, new_conn));

}

connection_ptr
Router::new_connection()
{
    return connection_ptr( new Connection( m_acceptor->io_service(), this ) );
}
                
std::string 
Router::gen_uuid()
{
    return m_uuidgen();
}

void
Router::stop()
{
    while( m_connections.size() )
    {
        m_connections.front()->fin();
    }
    //m_acceptor->get_io_service().stop();
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
    connection_ptr new_conn = new_connection();
    
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

connection_ptr 
Router::get_connection_by_name( const std::string &name )
{
    boost::mutex::scoped_lock lk(m_connections_mutex);
    vector<connection_ptr>::iterator it;
    for( it=m_connections.begin() ; it < m_connections.end() ; ++it )
    {
        if( (*it)->name() == name )
        {
            return *it;
        }
    }
    return connection_ptr();
}

/// debug usage - get list of connections
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

vector<string>
Router::get_connected_names()
{
    vector<string> v;
    BOOST_FOREACH( connection_ptr conn, m_connections )
    {
        v.push_back( conn->name() );
    }
    return v;
}

/// this is the default msg recvd callback passed to new connections
/// it does some basic sanity checks, then fires the callback
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
    if( msgp->length() > 16384 ) // hard limit
    {
        cout << "Dropping, msg length: " << msgp->length() << endl;
        return;
    }
    /* 
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
    */
    m_protocol->message_received( msgp, conn );
}

/// Connect out to a remote Servent at endpoint
void 
Router::connect_to_remote(boost::asio::ip::tcp::endpoint &endpoint)
{
    map<string,string> props;
    connect_to_remote( endpoint, props );
}

void 
Router::connect_to_remote(boost::asio::ip::tcp::endpoint &endpoint, const map<string,string>& props)
{
    cout << "connect_to_remote(" << endpoint.address().to_string()<<":"
         << endpoint.port()<<")" << endl;
    connection_ptr new_conn = new_connection();
    typedef pair<string,string> pair_t;
    BOOST_FOREACH( pair_t p, props )
    {
        new_conn->set( p.first, p.second );
    }
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
    cout << "foreach_conns" << endl;
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
    //foreach_conns( boost::bind(&Connection::async_write, _1, msgp) );
    boost::mutex::scoped_lock lk(m_connections_mutex);
    BOOST_FOREACH( connection_ptr conn, m_connections )
    {
        cout << "Sending " << msgp->str() << " to " << conn->str() << endl;
        conn->async_write( msgp );
    }
}



} //ns
