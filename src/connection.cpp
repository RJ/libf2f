#include "libf2f/connection.h"
#include <boost/foreach.hpp>

namespace libf2f {

using namespace std;

Connection::Connection( boost::asio::io_service& io_service, 
            boost::function< void(message_ptr, connection_ptr) > msg_cb,
            boost::function< void(connection_ptr) > fin_cb )
    : m_socket(io_service), m_authed(false), m_sending(false),
      m_message_received_cb(msg_cb),
      m_fin_cb(fin_cb)
{
    std::cout << "CTOR connection" << std::endl;
    max_writeq_size = 20*1024; // 20kb
}

Connection::~Connection()
{
    std::cout << "dtor Connection shutting down" << std::endl;
}

void 
Connection::close()
{
    socket().close();
}

void 
Connection::fin()
{
    std::cout << "FIN connection " << str() << std::endl;
    m_fin_cb( shared_from_this() );
    close();
}

/// Get the underlying socket.
boost::asio::ip::tcp::socket& 
Connection::socket()
{
    return m_socket;
}

void 
Connection::async_write(message_ptr msg)
{
    {
        boost::mutex::scoped_lock lk(m_mutex);
        m_writeq.push_back(msg);
        m_writeq_size += sizeof(message_header) + msg->length();
    }
    // make sure our sending loop is running:
    boost::system::error_code e;
    do_async_write( e, message_ptr() );
}

/// Reading incoming messages is a chain of 3 async methods:
/// async_read() -> handle_read_headers() -> handle_read_data()
void 
Connection::async_read()
{
    message_ptr msgp(new Message());
    // Read exactly the number of bytes in a header
    boost::asio::async_read(m_socket, 
                            boost::asio::buffer((char*)&msgp->header(), 
                                                sizeof(message_header)),
                            boost::bind(&Connection::handle_read_header,
                                        shared_from_this(), 
                                        boost::asio::placeholders::error, 
                                        msgp
                                        ));
}
/// called when we've read the header off the wire
void 
Connection::handle_read_header(const boost::system::error_code& e, message_ptr msgp)
{
    //cout << "handle_read_header" << endl;
    if (e)
    {
        std::cerr << "err" << std::endl;
        fin();
        return;
    }
    // Start an asynchronous call to receive the payload data
    assert( msgp->length() <= 10240 ); // 10K hard limit on payload size 
    // allocate space for payload, length taken from header:
    msgp->malloc_payload();
    // short-circuit if there is no payload
    if( msgp->length() == 0 )
    {
        handle_read_data( e, msgp );
        return;
    }
    // read the payload data:
    boost::asio::async_read(m_socket, 
                            boost::asio::buffer(msgp->payload(), 
                                                msgp->length()),
                            boost::bind(&Connection::handle_read_data,
                                        shared_from_this(), 
                                        boost::asio::placeholders::error,
                                        msgp
                                        ));
    
}
/// called when we've read header and payload off the wire
void 
Connection::handle_read_data(const boost::system::error_code& e, message_ptr msgp)
{
    if (e)
    {
        std::cerr << "errrrrrr" << std::endl;
        fin();
        return;
    }
    // inform router that we received a new message
    m_message_received_cb( msgp, shared_from_this() );
    // setup recv for next message:
    async_read();
}

void 
Connection::set_message_received_cb( boost::function< void(message_ptr, connection_ptr) > cb )
{
    m_message_received_cb = cb;
}
/// unused atm.. todo flow control?
size_t
Connection::drain_writeq( std::deque< message_ptr > & out )
{
    size_t ret;
    boost::mutex::scoped_lock lk(m_mutex);
    BOOST_FOREACH( message_ptr mp, m_writeq )
    {
        ret++;
        out.push_back( mp );
    }
    m_writeq.clear();
    return ret;
}

std::string 
Connection::str() const 
{
    std::ostringstream os;
    os   << "[Connection:"
            << m_socket.remote_endpoint().address().to_string()
            << "]";
    return os.str();
}

/// Calls to do_async_write are chained - it will call itself when a write
/// completes, to send the next msg in the queue. Bails when none left.
void 
Connection::do_async_write(const boost::system::error_code& e, message_ptr finished_msg)
{
    message_ptr msgp;
    { // mutex scope
        boost::mutex::scoped_lock lk(m_mutex);
        if(m_sending && !finished_msg)
        {
            // this call is telling us to send, but we already are.
            //cout << "bailing from do_async_write - already sending (sending=true)" << endl;
            return;
        }
        
        if(m_writeq.empty())
        {
            //cout << "bailing from do_async_write, q empty (sending=false)" << endl;
            m_sending = false;
            return;
        }
        
        msgp = m_writeq.front();
        m_writeq.pop_front();
        m_writeq_size -= (sizeof(message_header) + msgp->length());
        m_sending = true;
    } // mutex scope
    
    boost::asio::async_write( socket(), msgp->to_buffers(),
                              boost::bind( &Connection::do_async_write, this,
                                           boost::asio::placeholders::error,
                                           msgp ) );
}

} //ns

