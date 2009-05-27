#ifndef __LIBF2F_MESSAGE_H__
#define __LIBF2F_MESSAGE_H__

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <iostream>

#ifndef GENUUID 
#define GENUUID "266695BF-AC15-4991-A01D-21DC180FD4B1"
#endif

#define PING             0
#define PONG             1
#define IDENT            2
#define QUERY            3
#define QUERYRESULT      4
#define QUERYCANCEL      5
#define SIDREQUEST       6
#define SIDDATA          7
#define SIDCANCEL        8
#define BYE              9

class Message;
typedef boost::shared_ptr<Message> message_ptr;

/*
        Bytes   Description
        -------------------
        0-35    Msg GUID
        36      Msg Type (see Message::Type enum)
        37      TTL
        38      Hops
        39-42   Payload Length
        43-     Payload

TTL             Time To Live. The number of times the message
                will be forwarded by servents before it is
                removed from the network. Each servent will decrement
                the TTL before passing it on to another servent. When
                the TTL reaches 0, the message will no longer be
                forwarded.

Hops            The number of times the message has been forwarded.
                As a message is passed from servent to servent, the
                TTL and Hops fields of the header must satisfy the
                following condition:
                TTL(0) = TTL(i) + Hops(i)
                Where TTL(i) and Hops(i) are the value of the TTL and
                Hops fields of the message, and TTL(0) is maximum
                number of hops a message will travel (7 in gnutella networks, less for us?).

*/

/// All messages start with this header:
struct message_header
{
    char    guid[36];
    char    type;
    char    ttl;
    char    hops;
    boost::uint32_t length;
};

class Message
{
public:

    Message()
        : m_payload(0)
    {}

    Message(const message_header& header)
    {
        m_header = header;
        m_guid = std::string(header.guid, 36);
        std::cout << "CTOR Msg(" << m_guid << ")" << std::endl;
    }
    
    virtual ~Message()
    {
        std::cout << "DTOR Msg(" << m_guid << ")" << std::endl;
        free(m_payload);
    }
    
    virtual const std::string str() const
    {
        std::ostringstream os;
        os  << "[Msg type:" << type() << " ttl:" << ttl() << " hops:" << hops()
            << " length:" << length() << " guid:" << guid() << "]";
        return os.str();
    }
    
    virtual message_header& header() { return m_header; }
    virtual const char type() const { return m_header.type; }
    virtual const short ttl() const { return m_header.ttl; }
    virtual const short hops() const { return m_header.hops; }
    virtual const boost::uint32_t length() const { return ntohl(m_header.length); }
    virtual const std::string& guid() const { return m_guid; }
    // payload
    virtual const char * payload() const { return m_payload; }
    virtual char * payload() { return m_payload; }
    
    virtual size_t malloc_payload()
    {
        if( length() == 0 ) return 0;
        free(m_payload);
        m_payload = (char*)malloc( length() );
        return length();
    }
    
    virtual const boost::asio::mutable_buffer payload_buffer() const
    {
        return boost::asio::buffer( m_payload, length() );
    }
    
    virtual std::vector<boost::asio::const_buffer> to_buffers() const
    {
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back( boost::asio::buffer( 
                            (char*)&m_header, sizeof(message_header) ) );
        if(length())
        {
            buffers.push_back( boost::asio::buffer( m_payload, length() ) );
        }
        
        return buffers;
    }
    
private:
    message_header m_header;
    std::string m_guid;
    char * m_payload;
};

class PongMessage : public Message
{
public:
    static boost::shared_ptr<Message> factory()
    {
        message_header h;
        memcpy( &h.guid, std::string(GENUUID).data(), 36 );
        //h.guid = GENUUID;
        h.type = PONG;
        h.ttl  = 1;
        h.hops = 0;
        h.length = 0;
        return boost::shared_ptr<Message>(new Message( h ));
    }
};

class PingMessage : public Message
{
public:
    static boost::shared_ptr<Message> factory()
    {
        message_header h;
        memcpy( &h.guid, std::string(GENUUID).data(), 36 );
        h.type = PING;
        h.ttl  = 1;
        h.hops = 0;
        h.length = 0;
        return boost::shared_ptr<Message>(new Message( h ));
    }
};




/// File transfer/streaming msgs are special in that they have an additional header describing the payload (just the sid)
class DataMessage : public Message
{
public:

    /// Grab the SID from the first 36 bytes of the payload:
    const std::string& sid() const
    {
        if( m_sid.empty() )
        {
            m_sid = "TODO"; //std::string( payload(), 36 );
        }
        return m_sid;
    }
    
    const boost::asio::const_buffer payload_data() const
    {
        return boost::asio::const_buffer( payload()+36, length()-36 );
    }
    
private:
    mutable std::string m_sid;
};


#endif
