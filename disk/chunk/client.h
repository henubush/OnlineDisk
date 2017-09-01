#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <set>
#include <json/json.h>

using namespace muduo;
using namespace muduo::net;

class Client
{
public:
	Client(EventLoop* loop,const InetAddress& serverAddr,const string& nameArg);
	void start();
private:
	void onConnection(const TcpConnectionPtr& conn);
	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
	TcpClient client_;
};

#endif