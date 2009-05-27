#include "libf2f/router.h"
#include "libf2f/connection.h"
#include "libf2f/protocol.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>

using namespace std;

Router::Router( boost::asio::ip::tcp::acceptor& acc, Protocol * p )
    : m_acceptor( acc ), m_protocol( p ), seen_connections(0)
{
    // Start an accept operation for a new connection.
    connection_ptr new_conn(new Connection(m_acceptor.io_service(), boost::bind( &Router::message_received, this, _1, _2 )));
    
    m_acceptor.async_accept(new_conn->socket(),
        boost::bind(&Router::handle_accept, this,
        boost::asio::placeholders::error, new_conn));
    
    // start thread that dispatches outoing msgs:
    m_dispatch_thread = boost::thread(boost::bind(
                        &Router::message_dispatch_runner, this));
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
        m_connections.push_back( conn );
        conn->async_read();
    }
    
    // Start an accept operation for a new connection.
    connection_ptr new_conn(new Connection(m_acceptor.io_service(), boost::bind( &Router::message_received, this, _1, _2 )));
    
    m_acceptor.async_accept(new_conn->socket(),
        boost::bind(&Router::handle_accept, this,
            boost::asio::placeholders::error, new_conn));
}

/// this is the default msg recvd callback passed to new connections:
void
Router::message_received( message_ptr msgp, connection_ptr conn )
{
    cout << "router::message_received from " << conn->str() << endl;
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
        conn->async_write( PongMessage::factory() );
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
    connection_ptr new_conn(new Connection(m_acceptor.io_service(), boost::bind( &Router::message_received, this, _1, _2 )));
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
    conn->async_read(); // start read loop for this connection
}

/// runs in a thread- dispatches outbound messages
/// TODO flow control algorithm, stats collection.
void 
Router::message_dispatch_runner()
{
    boost::asio::deadline_timer timer( m_acceptor.get_io_service() );
    std::deque< message_ptr > msgs;
    size_t num;
    while(true)
    {
        BOOST_FOREACH( connection_ptr conn, m_connections )
        {
            if( (num = conn->drain_writeq( msgs )) )
            {
                cout << "Sending " << num << " msgs on " << conn->str() << endl;
                BOOST_FOREACH( message_ptr mp, msgs )
                {
                    cout << "Sending:" << mp->str() << endl;
                    //boost::asio::async_write(   conn->socket(),
                    //                            mp->to_buffers(),
                    //                            0 );
                }
                msgs.clear();
            }
        }
        
        // wait until next time slice:
        timer.expires_from_now( boost::posix_time::milliseconds(1000) );
        timer.wait();
    }
}

//void
//Router::write_complete( const boost::system::error_code& e, message_ptr msgp )
//{
//
//}
















