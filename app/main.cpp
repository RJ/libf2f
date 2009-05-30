#include <iostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>

#include "libf2f/router.h"
#include "libf2f/protocol.h"

#include "demo_messages.h"
#include "demo_protocol.h"

using namespace std;
using namespace libf2f;


void iorun( boost::asio::io_service * ios )
{
    ios->run();
    cout << "io ended" << endl;
}

int main(int argc, char **argv)
{
    if( argc != 2 )
    {
        cout <<"Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }
    
    boost::asio::io_service ios;
    
    DemoProtocol p;
    short port = atoi(argv[1]);
    cout << "Listening on port " << port << endl;
    boost::shared_ptr<boost::asio::ip::tcp::acceptor> accp( 
        new boost::asio::ip::tcp::acceptor
            (ios, boost::asio::ip::tcp::endpoint(
                            boost::asio::ip::tcp::v4(), 
                            port)
            ) 
    );
    Router r(accp, &p);
    
    boost::thread t( boost::bind(&iorun, &ios) );
    
    string line;
    char b[255];
    while(true)
    {
        cout << "> " << flush;
        cin.getline(b,255);
        line = string(b);
        
        if(line == "quit") break;
        
        vector<string> parts;
        boost::split(parts,line,boost::is_any_of(" "));
        
        if(parts[0] == "connect" && parts.size() == 3)
        {
            cout << "...." << endl;
            boost::asio::ip::address_v4 ipaddr  =
                boost::asio::ip::address_v4::from_string(parts[1]);
            short rp = atoi(parts[2].c_str());
            boost::asio::ip::tcp::endpoint ep(ipaddr, rp);
            r.connect_to_remote( ep );
        }
        
        if(parts[0] == "pingall")
        {
            message_ptr ping = message_ptr(new PingMessage());
            r.send_all(ping);
        }
        
        if(parts[0] == "query" && parts.size() == 2)
        {
            message_ptr search = message_ptr(new GeneralMessage(QUERY, parts[1]) );
            r.send_all(search);
        }
        
        if(parts[0] == "quit") break;
    }

    r.stop();
    t.join();
    return 0;
}
