#include <muduo/base/Logging.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <pthread.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;


const int piecesize = 64*1000*1000;
const int KoutTimes = 3;
pthread_mutex_t mutex;

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

struct file
{
    int fileSize;
    int filepieces;
    std::vector<std::string> ip;
    std::vector<int> port;
};

struct chunkInfo
{
    std::string ip;
    int port;
    int size;
    chunkInfo(std::string _ip = "127.0.0.1",int _port = 8888,int _size = 0):ip(_ip),port(_port),size(_size){}
    bool operator<(const chunkInfo b) const
    {
        return this->port < b.port;
    }
};
bool cmp(chunkInfo a,chunkInfo b)
{
    return a.size<b.size;
}
struct chunkStatus
{
    int size;
    int outOfTime;
    chunkStatus(int _size = 0,int _outOfTime = 0):size(_size),outOfTime(_outOfTime){}
};
std::map<chunkInfo,chunkStatus> chunk;  //chunk信息保存，超时判断
std::vector<chunkInfo> chunkv;

std::map<std::string,file> contents;            //md5 find fileInfo
std::map<std::string,std::string> contentFind; //filename find md5

std::string itoa(int num,int len)
{
    std::string s = "";
    while(num)
    {
        s+=('0'+num%10);
        num/=10;
    }
    reverse(s.begin(),s.end());
    while(s.length()<len)
    {
        s = '0'+s;
    }
    return s;
}

//accept的回调函数
void onConnection(const TcpConnectionPtr &conn)
{
    LOG_INFO << "Server - " << conn->peerAddress().toIpPort() << " -> "
    << conn->localAddress().toIpPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");
}

//recv的回调函数
void onMessage(const TcpConnectionPtr &conn,
    Buffer *buf,
    Timestamp time)
{
    muduo::string msg(buf->retrieveAllAsString());
    std::string s ="";
    for(int i=0;i<msg.length();i++)
    {
        s+=msg[i];
    }
    Json::Reader  reader;
    Json::Value   value;
    if(reader.parse(s,value))//字符串转json
    {
        std::string function = value["function"].asString();
        if(function == "uploadRequest")
        {
            std::string md5 = value["md5"].asString();
            std::string filePath = value["filePath"].asString();
            
            long long fileSize = value["fileSize"].asDouble();
            std::string fileName = "";
            for(int i=filePath.length()-1;i>=0;i--)
            {
                if(filePath[i]=='/')
                    break;
                fileName = filePath[i] + fileName;
            }
            Json::Value  json;
            json["function"] = Json::Value("uploadResponse");

            std::map<std::string,std::string>::iterator it = contentFind.find(fileName);
            if(it == contentFind.end())//文件名不存在
            {
                json["existFileName"] = Json::Value(0);
                std::map<std::string,file>::iterator iit = contents.find(md5);
                if(iit == contents.end()) //md5也不存在，直接上传
                {
                    json["existMd5"] = Json::Value(0);
                    json["md5"] = Json::Value(md5);
                    json["filePath"] = filePath;
                    int pieces = fileSize/piecesize + ((fileSize % piecesize) == 0 ? 0 : 1);
                    json["pieces"] = pieces;

                    file filetem;
                    filetem.fileSize = fileSize;
                    filetem.filepieces = pieces;

                    int chunksize = chunk.size();
                    pthread_mutex_lock(&mutex);
                    sort(chunkv.begin(),chunkv.end(),cmp);
                    for(int i=0;i<pieces;i++)
                    {
                        int step = i%chunksize;
                        Json::Value ipport;
                        filetem.ip.push_back(chunkv[step].ip);
                        ipport["ip"] = chunkv[step].ip;
                        filetem.port.push_back(chunkv[step].port);
                        ipport["port"] = chunkv[step].port;
                        json["ipport"].append(ipport);
                        chunkv[step].size++;
                    }
                    pthread_mutex_unlock(&mutex);
                    contents[md5] = filetem;
                    contentFind[fileName] = md5;
                    std::string uploadResponse = json.toStyledString();
                    int len = uploadResponse.length();
                    //std::cout<<"uploadResponse len  = " << len <<std::endl;
                    std::cout <<"uploadResponse = "<< uploadResponse << std::endl;
                    conn->send(itoa(len,5));
                    conn->send(uploadResponse);
                }
                else    //m5d存在，即文件已存在，但文件名不同，秒传并记录文件名
                {
                    contentFind[fileName] = md5;
                    json["existMd5"] = Json::Value(1);
                    std::string uploadResponse = json.toStyledString();
                    std::cout <<"uploadResponse = "<< uploadResponse <<endl;
                    int len = uploadResponse.length();
                    conn->send(itoa(len,5));
                    conn->send(uploadResponse);
                }
            }
            else
            {
                std::string exitFileMd5 = contentFind[fileName];
                if(md5 == exitFileMd5) //md5相同，秒传
                {
                    json["existMd5"] = Json::Value(1);
                    std::string uploadResponse = json.toStyledString();
                    std::cout <<"uploadResponse = "<< uploadResponse <<endl;
                    int len = uploadResponse.length();
                    conn->send(itoa(len,5));
                    conn->send(uploadResponse);
                }
                else    //md5不同，更新覆盖
                {
                    json["existMd5"] = Json::Value(0);
                    json["md5"] = Json::Value(md5);
                    json["filePath"] = filePath;
                    int pieces = fileSize/piecesize + ((fileSize % piecesize) == 0 ? 0 : 1);
                    json["pieces"] = pieces;

                    file filetem;
                    filetem.fileSize = fileSize;
                    filetem.filepieces = pieces;
                    int chunksize = chunkv.size();
                    pthread_mutex_lock(&mutex);
                    sort(chunkv.begin(),chunkv.end(),cmp);
                    for(int i=0;i<pieces;i++)
                    {
                        int step = i%chunksize;
                        
                        Json::Value ipport;
                        filetem.ip.push_back(chunkv[step].ip);
                        ipport["ip"] = chunkv[step].ip;
                        filetem.port.push_back(chunkv[step].port);
                        ipport["port"] = chunkv[step].port;
                        json["ipport"].append(ipport);
                        chunkv[step].size++;

                    }
                    pthread_mutex_unlock(&mutex);
                    contents[md5] = filetem;
                    contentFind[fileName] = md5;
                    std::string uploadResponse = json.toStyledString();
                    std::cout <<"uploadResponse = "<< uploadResponse <<endl;
                    int len = uploadResponse.length();
                    conn->send(itoa(len,5));
                    conn->send(uploadResponse);
                }
            }

            /*cout<<"function = "<<function<<endl;
            cout<<"md5 = "<< md5 <<endl;
            cout<<"filePath = "<< filePath <<endl;
            cout<<"fileName = "<< fileName <<endl;
            cout<<"fileSize = "<< fileSize <<endl;*/
        }
        else if(function == "existFilesRequest")
        {
            Json::Value  json;
            json["function"] = Json::Value("existFilesResponse");
            int size = contentFind.size();
            json["size"] = Json::Value(size);

            map<std::string,std::string>::iterator it;
            for(it = contentFind.begin();it!=contentFind.end();it++)
            {
                std::string filePath = it->first;
                std::string fileName = "";
                for(int i=filePath.length()-1;i>=0;i--)
                {
                    if(filePath[i] == '/')
                        break;
                    fileName = filePath[i] + fileName;
                }
                json["fileName"].append(fileName);
                
            }
            std::string existFilesResponse = json.toStyledString();
            std::cout <<"existFilesResponse = "<< existFilesResponse <<endl;
            int len = existFilesResponse.length();
            conn->send(itoa(len,5));
            conn->send(existFilesResponse);
        }
        else if(function == "downloadRequest")
        {
            Json::Value  json;
            json["function"] = Json::Value("downloadResponse");

            std::string fileName = value["fileName"].asString();
            json["fileName"] = Json::Value(fileName);

            std::map<std::string,std::string>::iterator it = contentFind.find(fileName);
            if(it == contentFind.end())//文件不存在
            {
                json["existFile"] = 0;
            }
            else
            {
                json["existFile"] = 1;
                std::string md5 = contentFind[fileName];
                file filetem = contents[md5];
                json["md5"] = Json::Value(md5);
                json["pieces"] = Json::Value(filetem.filepieces);
                for(int i=0;i<filetem.filepieces;i++)
                {
                    Json::Value ipport;
                    ipport["ip"] = Json::Value(filetem.ip[i]);
                    ipport["port"] = Json::Value(filetem.port[i]);
                    json["ipport"].append(ipport);
                }
            }
            std::string downloadResponse = json.toStyledString();
            std::cout <<"downloadResponse = "<< downloadResponse <<endl;
            int len = downloadResponse.length();
            conn->send(itoa(len,5));
            conn->send(downloadResponse);
        }
        else if(function == "joinRequest")
        {
            Json::Value  json;
            json["function"] = Json::Value("joinResponse");
            json["response"] = Json::Value("join OK!");
            std::string ip = value["ip"].asString();
            int port = value["port"].asInt();
            int size = value["size"].asInt();
            chunkInfo chunkinfo = chunkInfo(ip,port,size);
            chunkStatus chunkstatus = chunkStatus(size,0);
            std::map<chunkInfo, chunkStatus>::iterator it = chunk.find(chunkinfo);
            if(it == chunk.end())
            {
                pthread_mutex_lock(&mutex); //加锁
                chunkv.push_back(chunkinfo);
                pthread_mutex_unlock(&mutex); //去锁
            }
            pthread_mutex_lock(&mutex); //去锁
            chunk[chunkinfo] = chunkstatus;
            pthread_mutex_unlock(&mutex); //去锁
            std::string joinResponse = json.toStyledString();
            conn->send(joinResponse);
            //std::cout<<"chunk size = "<<chunk.size()<<std::endl;
            //std::cout<<"chunkv size = "<<chunkv.size()<<std::endl;
        }
        else
            cout<<"error"<<endl;
    }
    else
        cout<<"error"<<endl;
}

void* outOfTimeCheck(void* args)
{  
    while(1)
    {
        sleep(3);
        std::map<chunkInfo, chunkStatus>::iterator it;
        for(it = chunk.begin();it!=chunk.end();it++)
        {
            chunkStatus chunk_status = it->second;
            chunkInfo chunk_info = it->first;
            chunk_status.outOfTime++;
            pthread_mutex_lock(&mutex); //加锁
            if(chunk_status.outOfTime < KoutTimes)
                it->second = chunk_status;
            else
            {
                std::vector<chunkInfo>::iterator iit;
                for(iit = chunkv.begin();iit!=chunkv.end();)
                {
                    if(iit->port == chunk_info.port)
                    {
                        iit = chunkv.erase(iit);
                        break;
                    }
                    else
                        iit++;
                }
                chunk.erase(it);
            }
            pthread_mutex_unlock(&mutex); //去锁
        }
    }
} 

int main(int argc, const char *argv[])
{
    Logger::setOutput(defaultOutput);
    Logger::setFlush(defaultFlush);

    pthread_t pid;
    pthread_mutex_init(&mutex,NULL); //初始化互斥锁
    int ret = pthread_create(&pid, NULL, outOfTimeCheck, NULL);

    contents.clear();
    contentFind.clear();
    int ipport = 1025;
    
    EventLoop loop;
    InetAddress addr("10.0.0.201", 1024);//设置master服务端IP/端口
    TcpServer server(&loop, addr, "DiskMaster");
    server.setConnectionCallback(&onConnection);//注册回调
    server.setMessageCallback(&onMessage);//注册回调
    server.start();
    loop.loop();
    return 0;
}