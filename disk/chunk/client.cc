#include "client.h"

using namespace muduo;
using namespace muduo::net;

extern int port;
extern std::set<std::string> content;

Client::Client(EventLoop* loop,const InetAddress& serverAddr,
            const string& nameArg):
    client_(loop, serverAddr, nameArg)
{
    client_.setConnectionCallback(boost::bind(&Client::onConnection, this, _1));
    client_.setMessageCallback(boost::bind(&Client::onMessage, this, _1, _2, _3));
}

void Client::start()
{
    client_.connect();
}

void sendMessage(const TcpConnectionPtr& conn)
{
    Json::Value   json;
    json["function"] = Json::Value("joinRequest");
    json["ip"] = "10.0.0.201";
    json["port"] = port;
    int size = content.size();
    json["size"] = size;

    std::string joinRequest = json.toStyledString();
    //std::cout<< joinRequest <<std::endl;
    conn->send(joinRequest);
}

void Client::onConnection(const TcpConnectionPtr& conn)
{
    if(conn->connected())
    {
        sendMessage(conn);
    }
    else
        std::cout<<"Connect to "<<conn->peerAddress().toIpPort()<<" failed"<<std::endl;
}
void Client::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    muduo::string msg(buf->retrieveAllAsString());
    std::string jsonString ="";
    for(int i=0;i<msg.length();i++)
    {
        jsonString+=msg[i];
    }

    Json::Reader *reader = new Json::Reader(Json::Features::strictMode());
    Json::Value   value;
    /*
    if(reader->parse(jsonString,value))//字符串转json
    {
        std::cout<<value.toStyledString();
    }
    */
    sleep(3);
    sendMessage(conn);
}
