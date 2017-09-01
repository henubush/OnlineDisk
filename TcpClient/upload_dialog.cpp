#include "upload_dialog.h"
#include "ui_upload_dialog.h"
#include "upload.h"
#include "mainwindow.h"
#include <iostream>

using namespace std;
const int Kpiecesize = 64*1000*1000;

//bool uploadReceived[2];
//int uploadJsonLength[2];
int filePieces,successPieces;
QTcpSocket *clientToMaster;
bool uploadRequestReceived;
int uploadRequestJsonLength;
//QTcpSocket *uploadclient1,*uploadclient2;

UploadThread *up;

Upload_Dialog::Upload_Dialog(QWidget *parent,QString path) :
    QDialog(parent),
    ui(new Ui::Upload_Dialog)
{
    successPieces = 0;
    //uploadReceived[0] = uploadReceived[1] = false;
    ui->setupUi(this);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);
    ui->pushButton->setVisible(false);
    ui->RoundBar->setVisible(false);
    ui->RoundBar->setDecimals(2);

    //this->setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏
    clientToMaster = new QTcpSocket(this);
//    uploadclient1 = new QTcpSocket();
//    uploadclient2 = new QTcpSocket();
//    connect(uploadclient1, SIGNAL(readyRead()), this, SLOT(readUploadMessage1()));//当client1有消息接受时会触发接收
//    connect(uploadclient2, SIGNAL(readyRead()), this, SLOT(readUploadMessage2()));//当client2有消息接受时会触发接收
    connect(clientToMaster, SIGNAL(readyRead()), this, SLOT(readMessage()));//当client有消息接受时会触发接收
    up = new UploadThread();
    connect(up, SIGNAL(resultSend(const double&)),this, SLOT(resultRead(const double&)));

    UploadRequestThread *t = new UploadRequestThread();
    connect(t, SIGNAL(md5Send(const QJsonObject&)),this, SLOT(md5Read(const QJsonObject&)));

    t->path = path;
    t->start();
}
Upload_Dialog::Upload_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Upload_Dialog)
{
    ui->setupUi(this);
}

void Upload_Dialog::md5Read(const QJsonObject& json)
{
    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);
    cout<<json_str.toStdString()<<endl;

    char msg[10240];
    strcpy(msg,json_str.toStdString().c_str());
    clientToMaster->connectToHost(QHostAddress("10.0.0.201"), 1024);
    clientToMaster->write(msg);
}

void Upload_Dialog::readMessage()
{
    while(1)
    {
        if(clientToMaster->bytesAvailable() >= 5 && !uploadRequestReceived)
        {
            uploadRequestReceived = true;
            uploadRequestJsonLength = QString(clientToMaster->read(5)).toInt();
        }
        if(clientToMaster->bytesAvailable() >= uploadRequestJsonLength)
        {
            QByteArray jsonMessage; //暂存接受到的数据
            jsonMessage = clientToMaster->read(uploadRequestJsonLength);
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
                    if(function == "uploadResponse")
                    {
                        cout<<"function = "<< function <<endl;
                        handleUploadMessage(obj,ui);
                    }
                }
                else
                    std::cout<<"Something error."<<std::endl;
            }
            else
                std::cout<<"Something error."<<std::endl;
            jsonMessage.clear();
            uploadRequestReceived = false;
        }
        else
            break;
    }
}

void handleUploadMessage(QJsonObject obj, Ui::Upload_Dialog *ui)
{
    int existFileName,existMd5;
    if(obj.contains("existFileName"))
    {
        QJsonValue existFileName_value = obj.take("existFileName");
        existFileName = existFileName_value.toInt();
    }
    if(obj.contains("existMd5"))
    {
        QJsonValue existMd5_value = obj.take("existMd5");
        existMd5 = existMd5_value.toInt();
    }
    if(existMd5 == 1)
    {
        ui->label->setText("Fast Upload Success!");
        ui->RoundBar->setVisible(true);
        ui->progressBar->setVisible(false);
        ui->RoundBar->setValue(100);

        /*ui->progressBar->setMaximum(100);
        ui->progressBar->setValue(100);
        */
        ui->pushButton->setVisible(true);

    }
    else if(existMd5 == 0)
    {
        /*ui->progressBar->setMaximum(100);
        ui->progressBar->setValue(0);*/
        ui->progressBar->setVisible(false);
        ui->RoundBar->setVisible(true);
        ui->RoundBar->setValue(0);
        //uploadFile(obj);
        up->obj = obj;
        up->start();
    }
    else
        QMessageBox::information(NULL, "Error", "Something else happened", QMessageBox::Yes, QMessageBox::Yes);
}
/*
void uploadFile(QJsonObject obj)
{
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
        filePieces = pieces;
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
        strcpy(msg+5,itoa(ret,10).c_str());
        strcpy(msg+5+10,json_str.toStdString().c_str());
        int totalLen = 5+10+len;
        if(port == 1025)
        {
            uploadclient2->connectToHost(QHostAddress(ip), port);
            uploadclient2->waitForConnected();
            uploadclient2->write(msg,totalLen);
            uploadclient2->waitForBytesWritten();
            uploadclient2->write(filebuf,ret);
            uploadclient2->waitForBytesWritten(ret+totalLen);

            uploadclient2->disconnectFromHost();
            uploadclient2->waitForDisconnected();
            cout<<"to write = "<<uploadclient2->bytesToWrite()<<endl;
        }
        else if(port == 1026)
        {

            uploadclient1->connectToHost(QHostAddress(ip), port);
            uploadclient1->waitForConnected();
            uploadclient1->write(msg,totalLen);
            uploadclient1->waitForBytesWritten();
            uploadclient1->write(filebuf,ret);
            uploadclient1->waitForBytesWritten(ret+totalLen);

            uploadclient1->disconnect();
            uploadclient1->waitForDisconnected();
            cout<<"to write = "<<uploadclient1->bytesToWrite()<<endl;
        }
        //sleep(2);
        cout<<"ret = "<<ret<<endl;
    }
    free(filebuf);
    fclose(fp);
}

void Upload_Dialog::readUploadMessage1()
{
    while(1)
    {
        while(uploadclient1->bytesAvailable() >= 5 && !uploadReceived[0])
        {
            uploadReceived[0] = true;
            uploadJsonLength[0] = QString(uploadclient1->read(5)).toInt();
        }
        if(uploadclient1->bytesAvailable() >= uploadJsonLength[0])
        {
            QByteArray jsonMessage; //暂存接受到的数据
            jsonMessage = uploadclient1->read(uploadJsonLength[0]);
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
                    if(function == "uploadResponse")
                    {
                        QJsonValue write_value = obj.take("write");
                        string writeOK = write_value.toString().toStdString();
                        if(writeOK == "OK")
                        {
                            //uploadclient1->abort();
                            //uploadclient1->disconnectFromHost();
                            //uploadclient1->waitForDisconnected();
                            successPieces++;
                            ui->progressBar->setValue(successPieces*1.0/filePieces*100);
                        }
                        if(successPieces == filePieces)
                        {
                            ui->label->setText("Upload Success!");
                            ui->pushButton->setVisible(true);
                        }
                    }
                }
                else
                    std::cout<<"Something error."<<std::endl;
            }
            else
                std::cout<<"Something error."<<std::endl;
            jsonMessage.clear();
            uploadReceived[0] = false;
        }
        else
            break;
    }
}

void Upload_Dialog::readUploadMessage2()
{
    while(1)
    {
        while(uploadclient2->bytesAvailable() >= 5 && !uploadReceived[1])
        {
            uploadReceived[1] = true;
            uploadJsonLength[1] = QString(uploadclient2->read(5)).toInt();
        }
        if(uploadclient2->bytesAvailable() >= uploadJsonLength[1])
        {
            QByteArray jsonMessage; //暂存接受到的数据
            jsonMessage = uploadclient2->read(uploadJsonLength[1]);
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
                    if(function == "uploadResponse")
                    {
                        QJsonValue write_value = obj.take("write");
                        string writeOK = write_value.toString().toStdString();
                        if(writeOK == "OK")
                        {
                            //uploadclient2->abort();
                            //uploadclient2->disconnectFromHost();
                            //uploadclient2->waitForDisconnected();
                            successPieces++;
                            ui->progressBar->setValue(successPieces*1.0/filePieces*100);
                            if(successPieces == filePieces)
                            {
                                ui->label->setText("Upload Success!");
                                ui->pushButton->setVisible(true);
                            }
                        }
                    }
                }
                else
                    std::cout<<"Something error."<<std::endl;
            }
            else
                std::cout<<"Something error."<<std::endl;
            jsonMessage.clear();
            uploadReceived[1] = false;
        }
        else
            break;
    }
}*/
void Upload_Dialog::resultRead(const double& res)
{
    //ui->progressBar->setValue(res);
    ui->RoundBar->setValue(res);
    if(res == 100)
    {
        ui->pushButton->setVisible(true);
        ui->label->setText("Upload File Success!");
    }
}

Upload_Dialog::~Upload_Dialog()
{
    delete ui;
}

void Upload_Dialog::on_pushButton_clicked()
{
    this->close();
}
