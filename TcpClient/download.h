#ifndef DOWNLOAD_H
#define DOWNLOAD_H

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
using namespace std;

class DownloadMergeThread : public QThread
{
    Q_OBJECT

public:
    explicit DownloadMergeThread(QObject *parent = 0);
    virtual void run();

    QJsonObject obj;
    string downloadFileName,md5;
    int downlaodPieces;
private:

signals:
    void mgergeSend();

public slots:

};

#endif // DOWNLOAD_H
