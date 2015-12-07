/*
 * tcpClient.h
 *
 *  Created on: Dec 7, 2015
 *      Author: fcaldas
 */

#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <iostream>
#include <boost/asio.hpp>

namespace websocket {

class tcpClient {

private:
	boost::asio::io_service io_service;
	boost::asio::ip::tcp::socket *msocket;

public:
    tcpClient();
    bool conn(std::string, int);
    bool send_data(std::string data);
    void close();
    std::string receive();
    ~tcpClient();
};

} /* namespace websocket */

#endif /* TCPCLIENT_H_ */
