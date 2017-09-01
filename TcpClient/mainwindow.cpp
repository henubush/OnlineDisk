#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "md5.h"
#include "upload_dialog.h"
#include "download_dialog.h"

using namespace std;

bool masterReceived;
int masterJsonLength;

QTcpSocket *client;

string itoa(int num,int len)
{
    string s = "";
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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    client = new QTcpSocket(this);
    ui->setupUi(this);
    connect(client, SIGNAL(readyRead()), this, SLOT(readMessage()));//当client有消息接受时会触发接收
}

MainWindow::~MainWindow()
{
    delete ui;
}
/*
string str;
QString qstr;
//从QString 到 std::string
str = qstr.toStdString();

//从std::string 到QString
qstr = QString::fromStdString(str);
*/

void MainWindow::on_pushButton_clicked()
{
    QString st = ui->lineEdit->text();
    if(st == "")
    {
        QMessageBox::information(NULL, "Error", "Something else happened", QMessageBox::Yes, QMessageBox::Yes);
    }
    else
    {
        Upload_Dialog *up = new Upload_Dialog(this,st);
        up->show();
        ui->listWidget->addItem("Upload " + st);
    }
}

void MainWindow::readMessage()
{
    while(1)
    {
        if(client->bytesAvailable() >= 5 && !masterReceived)
        {
            masterReceived = true;
            //只接收前5位，数据的大小
            masterJsonLength = QString(client->read(5)).toInt();
        }
        if(client->bytesAvailable() >= masterJsonLength)
        {
            QByteArray jsonMessage; //暂存接受到的数据
            jsonMessage = client->read(masterJsonLength);
            QJsonParseError json_error;
            QJsonDocument parse_doucment = QJsonDocument::fromJson(jsonMessage, &json_error);
            if(json_error.error == QJsonParseError::NoError)
            {
                if(parse_doucment.isObject())
                {
                    string function;
                    QJsonObject obj = parse_doucment.object();
                    QJsonObject temobj = obj;
                    if(obj.contains("function"))
                    {
                        QJsonValue function_value = obj.take("function");
                        function = function_value.toString().toStdString();
                    }
                    if(function == "existFilesResponse")
                    {
                        int size;
                        QString fileName;
                        QJsonArray fileName_array;
                        if(obj.contains(("size")))
                        {
                            QJsonValue size_value = obj.take("size");
                            size = size_value.toInt();
                        }

                        if (obj.contains("fileName"))
                        {
                            QJsonValue fileName_value = obj.value("fileName");
                            fileName_array = fileName_value.toArray();
                        }
                        for(int i=0;i<size;i++)
                        {
                            fileName = fileName_array.at(i).toString();
                            ui->comboBox->addItem(fileName);
                        }
                    }
                    if(function == "downloadResponse")
                    {
                        DownloadDialog *down = new DownloadDialog(this,obj,temobj);
                        down->show();
                    }
                }
                else
                    std::cout<<"Something error."<<std::endl;
            }
            else
                std::cout<<"Something error."<<std::endl;
            jsonMessage.clear();
            masterReceived = false;
        }
        else
            break;
    }
}

void MainWindow::on_pushButton_3_clicked()
{

    ui->comboBox->clear();
    QJsonObject json;
    json.insert("function", QString("existFilesRequest"));

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);

    char msg[10240];
    strcpy(msg,json_str.toStdString().c_str());
    client->connectToHost(QHostAddress("10.0.0.201"), 1024);
    client->write(msg);
}
void MainWindow::on_pushButton_2_clicked()
{
    QString fileName  = ui->comboBox->currentText();
    if(fileName == "")
    {
        QMessageBox::information(NULL, "Error", "Something else happened", QMessageBox::Yes, QMessageBox::Yes);
    }
    else
    {
        QJsonObject json;
        json.insert("function", QString("downloadRequest"));
        json.insert("fileName", fileName);
        QJsonDocument document;
        document.setObject(json);
        QByteArray byte_array = document.toJson(QJsonDocument::Compact);
        QString json_str(byte_array);

        char msg[10240];
        strcpy(msg,json_str.toStdString().c_str());
        client->connectToHost(QHostAddress("10.0.0.201"), 1024);
        client->write(msg);
        ui->listWidget->addItem("Download "+fileName);
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    QString filename;
    filename = QFileDialog::getOpenFileName(this,
        tr("File Select"),
        "",
        tr("File (*)")); //选择路径
    ui->lineEdit->setText(filename);
}
