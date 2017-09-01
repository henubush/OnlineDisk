#include "upload.h"
#include "md5.h"
#include "mainwindow.h"
#include "upload_dialog.h"
using namespace std;

const int Kpiecesize = 64*1000*1000;

long long getFileSize(char* filepath)
{
    FILE *fp = fopen(filepath,"rb+");
    fseek(fp, 0L, SEEK_END);
    long long filesize = ftell(fp);
    fclose(fp);
    return filesize;
}

UploadRequestThread::UploadRequestThread(QObject *parent) :
    QThread(parent)
{
}

void UploadRequestThread::run()
{
    QByteArray ba = path.toLatin1();
    char *filepath = ba.data();
    ifstream in(filepath);
    QString md5 =QString::fromStdString(MD5(in).toString());
    QJsonObject json;
    long long filesize = getFileSize(filepath);
    json.insert("function", QString("uploadRequest"));
    json.insert("filePath",filepath);
    json.insert("md5",md5);
    json.insert("fileSize",(double)filesize);
    md5Send(json);
}
UploadThread::UploadThread(QObject *parent) :
    QThread(parent)
{
}

void UploadThread::run()
{
    uploadclient1 = new QTcpSocket();
    QString md5,ip;
    string filePath;
    int pieces,port;
    QJsonArray ipport_array;
    if(obj.contains("filePath"))
    {
        QJsonValue filePath_value = obj.take("filePath");
        filePath = filePath_value.toString().toStdString();
    }
    if(obj.contains("md5"))
    {
        QJsonValue md5_value = obj.take("md5");
        md5 = md5_value.toString();
    }
    if(obj.contains("pieces"))
    {
        QJsonValue pieces_value = obj.take("pieces");
        pieces = pieces_value.toInt();
    }
    if (obj.contains("ipport"))
    {
        QJsonValue ipport_value = obj.value("ipport");
        ipport_array = ipport_value.toArray();
    }
    char *filebuf;
    const char *file = filePath.c_str();
    cout<<"file = "<<file<<endl;
    FILE *fp = fopen(file, "rb+");
    if(fp==NULL)
    {
        cout<<"fopen error"<<endl;
    }
    filebuf = (char *) malloc(64*1024*1024+100);

    for(int pieceStep = 0;pieceStep < pieces;pieceStep++)
    {
        QJsonObject json;
        json.insert("function","upload");
        json.insert("md5",md5);
        json.insert("pieceNumber",pieceStep+1);

        QJsonObject value = ipport_array.at(pieceStep).toObject();
        if(value.contains("ip"))
        {
            QJsonValue ip_value = value.take("ip");
            ip = ip_value.toString();
        }
        if(value.contains("port"))
        {
            QJsonValue port_value = value.take("port");
            port = port_value.toInt();
        }
        QJsonDocument document;
        document.setObject(json);
        QByteArray byte_array = document.toJson(QJsonDocument::Compact);
        QString json_str(byte_array);
        int len = json_str.length();
        char msg[1024*1024+1000];
        memset(msg,0,sizeof msg);
        strcpy(msg,itoa(len,5).c_str());

        memset(filebuf,0,sizeof filebuf);
        int ret = fread(filebuf, 1, Kpiecesize,fp);
        //cout<<"ret = "<<ret<<endl;
        strcpy(msg+5,itoa(ret,10).c_str());
        strcpy(msg+5+10,json_str.toStdString().c_str());
        int totalLen = 5+10+len;

        uploadclient1->connectToHost(QHostAddress(ip), port);
        uploadclient1->waitForConnected();
        uploadclient1->write(msg,totalLen);
        uploadclient1->waitForBytesWritten();
        int send = uploadclient1->write(filebuf,ret);
        //cout<<"send = "<<send<<endl;
        uploadclient1->waitForBytesWritten();
        uploadclient1->disconnectFromHost();
        uploadclient1->waitForDisconnected();
        resultSend(1.0*(pieceStep+1)/pieces*100);
    }
    delete(uploadclient1);
    free(filebuf);
    fclose(fp);
}

