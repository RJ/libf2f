#ifndef __LIBF2F_MESSAGE_H__
#define __LIBF2F_MESSAGE_H__

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <iostream>

namespace libf2f {

class Message;
typedef boost::shared_ptr<Message> message_ptr;
class Connection;
class Router;
typedef boost::shared_ptr<Connection> connection_ptr;
typedef boost::weak_ptr<Connection> connection_ptr_weak;

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

(ttl/hops not used in darknet configuration)

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
        : m_payload(0)
    {
        m_header = header;
    }

    virtual ~Message()
    {
    }

    virtual const boost::uint32_t total_length() const
    {
        return sizeof(message_header) + length();
    }

    virtual const std::string str() const
    {
        std::ostringstream os;
        os  << "[Msg type:" << (int)type()
            << " ttl:" << (int)ttl()
            << " hops:" << (int)hops()
            << " length:" << (int)length()
            << " guid:" << guid() << "]";
        return os.str();
    }

    virtual message_header& header() { return m_header; }
    virtual const char type() const { return m_header.type; }
    virtual const short ttl() const { return m_header.ttl; }
    virtual const short hops() const { return m_header.hops; }
    virtual const boost::uint32_t length() const { return ntohl(m_header.length); }
    virtual const std::string& guid() const
    {
        if( m_guid.empty() )
        {
            m_guid = std::string(m_header.guid, 36);
        }
        return m_guid;
    }
    // payload
    virtual const char * payload() const { return m_payload.get(); }
    virtual char * payload() { return m_payload.get(); }
    /// sucks to have to do this really, jsonspirit needs a string or stream:
    virtual std::string payload_str() const
    {
        std::string s(m_payload.get(), length());
        return s;
    }

    virtual size_t malloc_payload()
    {
        if( length() == 0 ) return 0;
        m_payload = std::make_unique<char[]>(length());
        return length();
    }

    virtual const boost::asio::mutable_buffer payload_buffer() const
    {
        return boost::asio::buffer( m_payload.get(), length() );
    }

    virtual std::vector<boost::asio::const_buffer> to_buffers() const
    {
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back( boost::asio::buffer(
                            (char*)&m_header, sizeof(message_header) ) );
        if(length())
        {
            buffers.push_back( boost::asio::buffer( m_payload.get(), length() ) );
        }

        return buffers;
    }

protected:
    message_header m_header;
    mutable std::string m_guid;
    std::unique_ptr<char[]> m_payload;
};

class GeneralMessage : public Message
{
public:
    GeneralMessage(const char msgtype, const std::string& body, const std::string& uuid)
    {
        message_header h;
        memcpy( &h.guid, uuid.data(), 36 );
        h.type = msgtype;
        h.ttl  = 1;
        h.hops = 0;
        h.length = htonl( body.length() );
        m_header = h;
        malloc_payload();
        memcpy( m_payload.get(), body.data(), body.length() );
    }
};


}
#endif
