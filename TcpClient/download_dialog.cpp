#include "download_dialog.h"
#include "ui_download_dialog.h"
#include "download.h"
#include <iostream>

using namespace std;

const int chunkSize = 2;
int downlaodPieces,downloadSuccess,downloadFailed;
string downloadFileName;
bool Received[chunkSize];  //初始化位false
int jsonLength[chunkSize],fileLength[chunkSize];   //待接收数据的长度
QTcpSocket *downloadclient1,*downloadclient2;
DownloadMergeThread *down;

void downloadFile(QJsonObject obj,QJsonObject downloadObj)
{
    downloadObj.insert("function",QString("download"));
    downloadFailed = 0;
    downloadSuccess = 0;
    string fileName;
    int pieces;
    if(obj.contains("fileName"))
    {
        QJsonValue fileName_value = obj.take("fileName");
        fileName = fileName_value.toString().toStdString();
    }
    if(obj.contains("pieces"))
    {
        QJsonValue pieces_value = obj.take("pieces");
        pieces = pieces_value.toInt();
    }
    downloadFileName = fileName;
    downlaodPieces = pieces;

    QString ip;
    int port;
    QJsonArray ipport_array;
    if (obj.contains("ipport"))
    {
        QJsonValue ipport_value = obj.value("ipport");
        ipport_array = ipport_value.toArray();
    }

    QJsonDocument document;
    document.setObject(downloadObj);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);
    char msgbuf[102400];
    int len = json_str.length();
    strcpy(msgbuf,itoa(len,5).c_str());
    strcpy(msgbuf+5,itoa(0,10).c_str());
    strcpy(msgbuf+15,json_str.toStdString().c_str());

    set<string> ipAndPort;
    ipAndPort.clear();
    for(int i=0;i<pieces;i++)
    {
        QJsonObject value = ipport_array.at(i).toObject();
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
        string s = ip.toStdString()+":"+itoa(port,0);
        if(ipAndPort.insert(s).second)
        {
            if(port == 1025)
            {
                downloadclient2->connectToHost(QHostAddress(ip), port);
                downloadclient2->write(msgbuf,15+len);
                downloadclient2->waitForBytesWritten();
            }
            else if(port == 1026)
            {
                downloadclient1->connectToHost(QHostAddress(ip), port);
                downloadclient1->write(msgbuf,15+len);
                downloadclient1->waitForBytesWritten();
            }
        }
    }
}

void DownloadDialog::readDownloadMessage1()
{
    while(1)
    {
        if(downloadclient1->bytesAvailable() >= 15 && !Received[0])
        {
            Received[0] = true;
            //只接收前15位，数据的大小
            jsonLength[0] = QString(downloadclient1->read(5)).toInt();
            fileLength[0] = QString(downloadclient1->read(10)).toInt();
        }
        if(downloadclient1->bytesAvailable() >= jsonLength[0]+fileLength[0] )
        {
            string md5;int pieceNum;
            QByteArray jsonMessage,fileMessage; //暂存接受到的数据
            jsonMessage = downloadclient1->read(jsonLength[0]);
            fileMessage = downloadclient1->read(fileLength[0]);
            QJsonParseError json_error;
            QJsonDocument parse_doucment = QJsonDocument::fromJson(jsonMessage, &json_error);
            if(json_error.error == QJsonParseError::NoError)
            {
                if(parse_doucment.isObject())
                {
                    string function;
                    QJsonObject obj = parse_doucment.object();
                    if(obj.contains("function"))
                    {
                        QJsonValue function_value = obj.take("function");
                        function = function_value.toString().toStdString();
                    }
                    if(function == "downloadResponse")
                    {
                        string fileName;
                        if(obj.contains("fileName"))
                        {
                            QJsonValue fileName_value = obj.take("fileName");
                            fileName = fileName_value.toString().toStdString();
                            md5 = fileName.substr(0,32);
                            pieceNum = QString::fromStdString(fileName.substr(32,3)).toInt();
                        }
                        char name[1024];
                        memset(name,0,sizeof(name));
                        strcpy(name,fileName.c_str());
                        FILE *fp = fopen(name,"at+");
                        std::cout<<"name = "<<name<<std::endl;
                        if(fp == NULL)
                        {
                            std::cout<<"fopen error\n";
                        }
                        int ret = fwrite(fileMessage.data(), 1,fileLength[0],fp);
                        //std::cout<<"ret = "<<ret<<std::endl;
                        fclose(fp);
                    }
                }
            }
            //jsonMessage.clear();fileMessage.clear();
            jsonMessage.resize(0);fileMessage.resize(0);
            Received[0] = false;
            downloadSuccess++;
            ui->RoundBar->setValue((1.0*downloadSuccess)/(1.0*downlaodPieces)*100);
            //cout<<"download = "<<downloadSuccess<<endl;
            if(downloadSuccess == downlaodPieces)
            {
                downloadclient1->disconnectFromHost();
                downloadclient1->waitForDisconnected();
                downloadclient2->disconnectFromHost();
                downloadclient2->waitForDisconnected();
                mergePiece(md5,ui);
            }
        }
        else
            break;
    }
}

void DownloadDialog::readDownloadMessage2()
{
    while(1)
    {
        if(downloadclient2->bytesAvailable() >= 15 && !Received[1])
        {
            Received[1] = true;
            //只接收前15位，数据的大小
            jsonLength[1] = QString(downloadclient2->read(5)).toInt();
            fileLength[1] = QString(downloadclient2->read(10)).toInt();
        }
        if(downloadclient2->bytesAvailable() >= jsonLength[1]+fileLength[1])
        {
            string md5;int pieceNum;
            QByteArray jsonMessage,fileMessage; //暂存接受到的数据
            jsonMessage = downloadclient2->read(jsonLength[1]);
            fileMessage = downloadclient2->read(fileLength[1]);
            QJsonParseError json_error;
            QJsonDocument parse_doucment = QJsonDocument::fromJson(jsonMessage, &json_error);
            if(json_error.error == QJsonParseError::NoError)
            {
                if(parse_doucment.isObject())
                {
                    string function;
                    QJsonObject obj = parse_doucment.object();
                    if(obj.contains("function"))
                    {
                        QJsonValue function_value = obj.take("function");
                        function = function_value.toString().toStdString();
                    }
                    if(function == "downloadResponse")
                    {
                        string fileName;
                        if(obj.contains("fileName"))
                        {
                            QJsonValue fileName_value = obj.take("fileName");
                            fileName = fileName_value.toString().toStdString();
                            md5 = fileName.substr(0,32);
                            pieceNum = QString::fromStdString(fileName.substr(32,3)).toInt();
                        }
                        char name[1024];
                        memset(name,0,sizeof(name));
                        strcpy(name,fileName.c_str());
                        FILE *fp = fopen(name,"at+");
                        std::cout<<"name = "<<name<<std::endl;
                        if(fp == NULL)
                        {
                            std::cout<<"fopen error\n";
                        }
                        int ret = fwrite(fileMessage.data(), 1,fileLength[1],fp);
                        //std::cout<<"ret = "<<ret<<std::endl;
                        fclose(fp);
                    }
                }
            }
            //jsonMessage.clear();fileMessage.clear();
            jsonMessage.resize(0);fileMessage.resize(0);
            Received[1] = false;
            downloadSuccess++;
            ui->RoundBar->setValue((1.0*downloadSuccess)/(1.0*downlaodPieces)*100);
            //cout<<"download = "<<downloadSuccess<<endl;
            if(downloadSuccess == downlaodPieces)
            {
                downloadclient1->disconnectFromHost();
                downloadclient1->waitForDisconnected();
                downloadclient2->disconnectFromHost();
                downloadclient2->waitForDisconnected();
                mergePiece(md5,ui);
            }
        }
        else
            break;
    }
}

void mergePiece(string md5,Ui::DownloadDialog *ui)
{
    ui->progressBar->setVisible(true);
    ui->RoundBar->setVisible(false);
    ui->RoundBar->setDecimals(2);
    ui->label->setText("File is arrangeing,wait please!");

    down->md5 = md5;
    down->downlaodPieces = downlaodPieces;
    down->downloadFileName = downloadFileName;
    down->start();
    downlaodPieces = downloadSuccess = downloadFailed = -1;

}
void DownloadDialog::mergeRecv()
{
    ui->label->setText("File download Success!");
    ui->progressBar->setVisible(false);
    ui->RoundBar->setVisible(true);
    ui->pushButton->setVisible(true);
}

DownloadDialog::DownloadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DownloadDialog)
{
    ui->setupUi(this);
}
DownloadDialog::DownloadDialog(QWidget *parent, QJsonObject obj, QJsonObject downloadObj) :
    QDialog(parent),
    ui(new Ui::DownloadDialog)
{
    down = new DownloadMergeThread();
    connect(down, SIGNAL(mgergeSend()),this, SLOT(mergeRecv()));
    Received[0] = Received[1] = false;
    ui->setupUi(this);

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->pushButton->setVisible(false);

    downloadclient1 = new QTcpSocket(this);
    downloadclient2 = new QTcpSocket(this);
    connect(downloadclient1, SIGNAL(readyRead()), this, SLOT(readDownloadMessage1()));//当client1有消息接受时会触发接收
    connect(downloadclient2, SIGNAL(readyRead()), this, SLOT(readDownloadMessage2()));//当client2有消息接受时会触发接收
    downloadFile(obj,downloadObj);
    //ui->progressBar->setMaximum(100);
    //ui->progressBar->setValue(0);
    ui->progressBar->setVisible(false);
    ui->RoundBar->setVisible(true);
    ui->RoundBar->setValue(0);

}

DownloadDialog::~DownloadDialog()
{
    delete ui;
}

void DownloadDialog::on_pushButton_clicked()
{
    this->close();
}
