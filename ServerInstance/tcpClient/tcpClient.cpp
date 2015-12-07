/*
 * tcpClient.cpp
 *
 *  Created on: Dec 7, 2015
 *      Author: fcaldas
 */

#include "tcpClient.h"

using namespace boost;
using namespace boost::asio;

namespace websocket {

/**
    Connect to a host on a certain port number
*/
bool tcpClient::conn(std::string address , int port)
{
    msocket->connect( ip::tcp::endpoint( ip::address::from_string(address), port ) );
    if(msocket->is_open())
    	return true;
    else
    	return false;
}

/**
    Send data to the connected host
*/
bool tcpClient::send_data(std::string data)
{
	boost::system::error_code error;
    asio::write( *msocket, asio::buffer(data), error );
    if(error){
    	throw std::runtime_error(error.message());
    }
    return true;
}

void tcpClient::close(){
	msocket->close();
}

/**
    Receive data from the connected host
*/
std::string tcpClient::receive()
{
	asio::streambuf receive_buffer;
	boost::system::error_code error;
	asio::read(*msocket, receive_buffer, asio::transfer_all(), error );
	boost::asio::streambuf::const_buffers_type bufs = receive_buffer.data();
	std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + receive_buffer.size());
	return str;
}


tcpClient::~tcpClient(){
	delete msocket;
}

tcpClient::tcpClient(){
	msocket = new asio::ip::tcp::socket(io_service);
}

} /* namespace websocket */
