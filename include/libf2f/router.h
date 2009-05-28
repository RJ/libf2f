#ifndef __LIBF2F_ROUTER_H_
#define __LIBF2F_ROUTER_H_

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>

#include "libf2f/message.h"
#include "libf2f/connection.h"

namespace libf2f {

class Protocol;


/// aka servent - responsible for managing connections
/// it will authenticate connections before creating a Connection
/// object and starting to send/recv data using flow control mechanism.
class Router
{
public:
    /// Constructor opens the acceptor and starts waiting for the first
    /// incoming connection.
    /// client code should create acceptor like so:
    /// acceptor(io_service, 
    ///          boost::asio::ip::tcp::endpoint(
    ///             boost::asio::ip::tcp::v4(), port) )
    Router( boost::asio::ip::tcp::acceptor & acc, Protocol * p );
    
    
    /// Handle completion of a accept operation.
    void handle_accept(const boost::system::error_code& e, connection_ptr conn);
                   
    /// Connect out to a remote Servent:
    void connect_to_remote(boost::asio::ip::tcp::endpoint &endpoint);
    
    /// Handle completion of a connect operation.
    void handle_connect(const boost::system::error_code& e,
                        boost::asio::ip::tcp::endpoint &endpoint,
                        connection_ptr conn);

    /// Default message recvd callback
    void message_received( message_ptr msgp, connection_ptr conn );
    
    /// apply function to all registered connections
    void foreach_conns( boost::function<void(connection_ptr)> );
    
private:
    /// Router keeps track of connections:
    void register_connection( connection_ptr conn );
    void unregister_connection( connection_ptr conn );
    void connections_str();
    
    /// all connections:
    std::vector< connection_ptr > m_connections;
    boost::mutex m_connections_mutex; // protects connections
    
    /// The acceptor object used to accept incoming socket connections.
    boost::asio::ip::tcp::acceptor & m_acceptor;
    /// protocol implementation
    Protocol * m_protocol;
    /// thread that enforces flow-control and sends outgoing msgs
    boost::thread m_dispatch_thread;
    
    /// misc stats:
    unsigned int seen_connections; // num incoming connections accepted
};

} //ns

#endif
