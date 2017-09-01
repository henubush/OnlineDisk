#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <string>
#include <cstring>
#include <string.h>
#include <QMessageBox>
#include <iostream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileDialog>
#include <thread>
#include <set>
#include <queue>
#include "upload.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void readMessage();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;
};

std::string itoa(int num,int len);

#endif // MAINWINDOW_H
