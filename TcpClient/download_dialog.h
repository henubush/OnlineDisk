#ifndef DOWNLOAD_DIALOG_H
#define DOWNLOAD_DIALOG_H

#include <QDialog>
#include "mainwindow.h"

using namespace std;
namespace Ui {
class DownloadDialog;
}

class DownloadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DownloadDialog(QWidget *parent = 0);
    explicit DownloadDialog(QWidget *parent,QJsonObject obj,QJsonObject downloadObj);
    ~DownloadDialog();
    QJsonObject obj,downloadObj;

private slots:
    void on_pushButton_clicked();
    void readDownloadMessage1();
    void readDownloadMessage2();
    void mergeRecv();


private:
    Ui::DownloadDialog *ui;
};
void mergePiece(string md5, Ui::DownloadDialog *ui);


#endif // DOWNLOAD_DIALOG_H
