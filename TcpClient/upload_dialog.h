#ifndef UPLOAD_DIALOG_H
#define UPLOAD_DIALOG_H

#include <QDialog>
#include <QtNetwork>
#include <unistd.h>

namespace Ui {
class Upload_Dialog;
}

class Upload_Dialog : public QDialog
{
    Q_OBJECT

private slots:
    void md5Read(const QJsonObject &json);
    void resultRead(const double &res);
    void readMessage();
//    void readUploadMessage1();
//    void readUploadMessage2();

    void on_pushButton_clicked();

public:
    explicit Upload_Dialog(QWidget *parent = 0);
    explicit Upload_Dialog(QWidget *parent = 0,QString path="");
    ~Upload_Dialog();

private:
    Ui::Upload_Dialog *ui;
};
void handleUploadMessage(QJsonObject obj,Ui::Upload_Dialog *ui);
//void uploadFile(QJsonObject obj);

#endif // UPLOAD_DIALOG_H
