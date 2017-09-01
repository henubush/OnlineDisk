#ifndef UPLOAD_H
#define UPLOAD_H

#include <QThread>
#include <QString>
#include <QDebug>
#include <iostream>
#include <string>
#include <string.h>
#include <QMainWindow>
#include <QtNetwork>
#include <string>
#include <cstring>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileDialog>

class UploadRequestThread : public QThread
{
    Q_OBJECT

public:
    explicit UploadRequestThread(QObject *parent = 0);
    virtual void run();
    QString path;

signals:
    void md5Send(const QJsonObject&);

public slots:

};
class UploadThread : public QThread
{
    Q_OBJECT

public:
    explicit UploadThread(QObject *parent = 0);
    virtual void run();

    QJsonObject obj;
    QTcpSocket* uploadclient1,* uploadclient2;

private:

signals:
    void resultSend(const double&);

public slots:

};
#endif // UPLOAD_H
