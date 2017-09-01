#include <muduo/base/Logging.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoopThread.h>

#include <iostream>
#include <set>
#include <string>
#include <cstring>
#include <stdio.h>
#include <json/json.h>
#include "client.h"
#include <unistd.h>
#include <dirent.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;

int port = 1025;
typedef void (*OutputFunc) (const char* msg,int len);
typedef void (*FlushFunc) ();
FILE *logfp;
void defaultOutput(const char* msg,int len)
{
	logfp = fopen("../../log/log","at+");
	size_t n = fwrite(msg,1,len,logfp);
	(void)n;
	fclose(logfp);
}
void defaultFlush()
{
	fflush(logfp);
}


std::set<std::string> content;

void getFiles(std::string path)
{
    //文件句柄
    DIR *dir;
    struct dirent *ptr;
    char base[1000];

    if ((dir=opendir(path.c_str())) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8)    ///file
            content.insert(ptr->d_name);
    }
    closedir(dir);
}

std::string itoa(long num,int len)
{
    std::string s = "";
    while(num)
    {
        s += ('0'+num%10);
        num/=10;
    }
    reverse(s.begin(),s.end());
    while(s.length()<len)
    {
        s = "0"+s;
    }
    return s;
}

long getFileSize(char* filepath)
{
    FILE *fp = fopen(filepath,"ab+");
    fseek(fp, 0L, SEEK_END);
    int filesize = ftell(fp);
    fclose(fp);
    return filesize;
}

void handleMeg(const TcpConnectionPtr &conn,
    Buffer *buf,int jsonLen,int fileLen)
{
    muduo::string msg(buf->peek(),jsonLen+fileLen);
    buf->retrieve(jsonLen+fileLen);
    std::string jsonString ="";
    std::string fileString = "";
    for(int i=0;i<jsonLen;i++)
    {
        jsonString += msg[i];
    }
    for(int i=jsonLen;i<msg.length();i++)
    {
        fileString += msg[i];
    }
    Json::Reader *reader = new Json::Reader(Json::Features::strictMode());
    Json::Value   value;
    if(reader->parse(jsonString,value))//字符串转json
    {
        std::cout<<value.toStyledString();
        std::string function = value["function"].asString();
        std::cout<< function <<std::endl;
        if(function == "upload")
        {
            std::string md5 = value["md5"].asString();
            int pieceNumber = value["pieceNumber"].asInt();
            std::string fileName =  md5;
            fileName += ('0' + pieceNumber/100);
            fileName += ('0' + (pieceNumber%100)/10);
            fileName += ('0' + pieceNumber%10);
            content.insert(fileName);
            char name[1024];
            memset(name,0,sizeof(name));
            strcpy(name,fileName.c_str());
            FILE *fp = fopen(name,"at+");
            if(fp == NULL)
            {
                std::cout<<"fopen error\n";
            }
            int ret = fwrite(fileString.data(), 1,fileLen,fp);
            std::cout<<"ret = "<<ret<<std::endl;
            fclose(fp);
            /*
            Json::Value json;
            json["function"] = Json::Value("uploadResponse");
            json["write"] = Json::Value("OK");
            std::string uploadloadResponse = json.toStyledString();
            uploadloadResponse = itoa(uploadloadResponse.length(),5)+uploadloadResponse;
            conn->send(uploadloadResponse);
            */
        }
        if(function == "download")
        {
            std::string md5 = value["md5"].asString();
            int pieces = value["pieces"].asInt();
            for(int i=0;i<pieces;i++)
            {
                std::string fileName =  md5;
                fileName += itoa((i+1),3);
                //std::cout<<fileName<<"   length = "<<fileName.length()<<endl;
                //std::cout<<content.size()<<std::endl;
                set<std::string>::iterator it = content.find(fileName);    //查找元素
                if(it != content.end())
                {
                    Json::Value json;
                    json["function"] = Json::Value("downloadResponse");
                    json["fileName"] = Json::Value(fileName);
                    std::string downloadResponse = json.toStyledString();
                    char name[1024];
                    memset(name,0,sizeof(name));
                    strcpy(name,fileName.c_str());
                    long fileSize = getFileSize(name);
                    downloadResponse = itoa(downloadResponse.length(),5)+itoa(fileSize,10)+downloadResponse;
                    std::cout<<downloadResponse<<std::endl;
                    conn->send(downloadResponse);

                    FILE *fp = fopen(name,"rb+");
                    //std::cout<<"name = "<<name<<std::endl;
                    if(fp == NULL)
                    {
                        std::cout<<"fopen error\n";
                    }

                    char *filebuf = (char *) malloc(1024*1024+100);
                    while(!feof(fp))
                    {
                        memset(filebuf,0,sizeof filebuf);
                        int ret = fread(filebuf,1,1024*1024, fp);
                        //std::cout<<"ret = "<<ret<<std::endl;
                        conn->send(filebuf,ret);
                    }
                    free(filebuf);
                }
                sleep(1);
            }
        }
    }

}

//accept的回调函数
void onConnection(const TcpConnectionPtr &conn)
{
	conn->setTcpNoDelay(true);
    LOG_INFO << "chunk - " << conn->peerAddress().toIpPort() << " -> "
    << conn->localAddress().toIpPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");
}

//recv的回调函数
void onMessage(const TcpConnectionPtr &conn,
    Buffer *buf,
    Timestamp time)
{
	//std::cout<<buf->readableBytes()<<std::endl;
	while(buf->readableBytes()>=15)
	{
		int jsonLen=0,fileLen=0;
        std::string msg = buf->peek();
        for(int i=0;i<5;i++) jsonLen = jsonLen*10 + (msg[i] - '0');
        for(int i=5;i<15;i++) fileLen = fileLen*10 + (msg[i] - '0');
        //std::cout<<"jsonLen = "<<jsonLen<<"\tfileLen = "<<fileLen<<std::endl;
        if(buf->readableBytes()>= 15+jsonLen+fileLen)
        {
            buf->retrieve(15);
            handleMeg(conn,buf,jsonLen,fileLen);
        }
        else 
            break;
    }
}

int main(int argc, const char *argv[])
{
	content.clear();
	getFiles("./");

	Logger::setOutput(defaultOutput);
	Logger::setFlush(defaultFlush);
    
    EventLoopThread loopThread;
    InetAddress serverAddr("10.0.0.201",1024);
    Client client(loopThread.startLoop(), serverAddr, "chunkToMasterClient");
    client.start();
    
    EventLoop loop;
    InetAddress addr("10.0.0.201", port);//设置master服务端IP/端口
    TcpServer server(&loop, addr, "DiskChunk");
    server.setConnectionCallback(&onConnection);//注册回调
    server.setMessageCallback(&onMessage);//注册回调
    server.start();
    loop.loop();
    return 0;
}
