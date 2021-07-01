#ifndef _LIBF2F_PEER_PeerConnection_H_
#define _LIBF2F_PEER_PeerConnection_H_

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <string>
#include <sstream>
#include <vector>

#include "libf2f/message.h"

namespace libf2f {

/// This class represents a Connection to one other libf2f user.
/// it knows how to marshal objects to and from the wire protocol
/// It keeps some state related to the Connection, eg are they authenticated.
class Connection
: public boost::enable_shared_from_this<Connection>
{
public:

    Connection( const boost::asio::any_io_executor& executor, Router * r );

    ~Connection();

    void close();

    void fin();

    /// Get the underlying socket.
    boost::asio::ip::tcp::socket& socket();

    /// Asynchronously write a data structure to the socket.
    /// This just enqueues the data and returns immediately
    void async_write(message_ptr msg);
    /// this is called internally to do actual sending:
    void do_async_write(const boost::system::error_code& e, message_ptr finished_msg);

    /// Setup a call to read the next msg_header
    void async_read();

    /// Handle a completed read of a message header
    void handle_read_header(const boost::system::error_code& e, message_ptr msgp);

    /// Handle a completed read of message, with payload attached
    void handle_read_data(const boost::system::error_code& e, message_ptr msgp);

    size_t writeq_size() const { return m_writeq.size(); }

    void push_message_received_cb( boost::function< void(message_ptr, connection_ptr) > cb );
    void pop_message_received_cb();

    std::string str() const;

    const bool ready() const { return m_ready; }
    void set_ready( bool b ) { m_ready = b; }
    const std::string& name() const { return m_name; }
    void set_name( const std::string& n ) { m_name = n; }

    /// get/set api for storing things associated with the connection
    /// bit of a hack, exposes weakness in api/design atm IMO.
    void set(const std::string&  key, const std::string& val)
    {
        boost::mutex::scoped_lock lk(m_props_mutex);
        m_props[key]=val;
    }
    std::string get(const std::string& key)
    {
        boost::mutex::scoped_lock lk(m_props_mutex);
        if( m_props.find(key) == m_props.end() ) return "";
        return m_props[key];
    }

private:

    boost::asio::ip::tcp::socket m_socket; // underlying socket

    boost::mutex m_mutex;               // protects outgoing message queue
    std::deque< message_ptr > m_writeq; // queue of outgoing messages
    size_t m_writeq_size;               // number of bytes in the writeq
    size_t max_writeq_size;             // max number of bytes in the queue

    /// Stateful stuff the protocol handler/servent will set:
    std::string m_name; // "name" of user at end of Connection
    std::map< std::string, std::string > m_props;
    boost::mutex m_props_mutex;

    bool m_ready; // ready for normal messages (ie, we authed etc)
    bool m_sending; // currently sending something?
    bool m_shuttingdown;

    std::vector< boost::function<void(message_ptr, connection_ptr)> > m_message_received_cbs;
    ///boost::mutex m_message_received_cb_mutex; // protects the above

    Router * m_router;

};

} //ns

#endif
