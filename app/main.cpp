#include <iostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>


#include "libf2f/router.h"
#include "libf2f/protocol.h"

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
    
    Protocol p;
    short port = atoi(argv[1]);
    cout << "Listening on port " << port << endl;
    boost::asio::ip::tcp::acceptor acc(
        ios, 
        boost::asio::ip::tcp::endpoint(
            boost::asio::ip::tcp::v4(), 
            port) 
    );
    Router r(acc, &p);
    
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
            message_ptr ping = PingMessage::factory();
            r.foreach_conns( boost::bind(&Connection::async_write, _1, ping) );
        }
    }

    ios.stop();
    t.join();
    return 0;
}