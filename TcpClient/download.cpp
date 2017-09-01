#include "download.h"

const string KDownloadPath = "/home/bush/Downloads/";
using namespace std;

DownloadMergeThread::DownloadMergeThread(QObject *parent) :
    QThread(parent)
{
}

void DownloadMergeThread::run()
{
    cout<<"merge start."<<endl;
    char name[1024];
    char fileName[1024];
    memset(fileName,0,sizeof(fileName));
    cout<<"downloadFIleName = "<<downloadFileName<<endl;
    strcpy(fileName,KDownloadPath.c_str());
    strcpy(fileName+KDownloadPath.length(),downloadFileName.c_str());

    char filebuf[1024*1024];
    FILE *fd = fopen(fileName,"at+");
    if(fd == NULL)cout<<"fopen error."<<endl;
    for(int i=1;i<=downlaodPieces;i++)
    {
        memset(name,0,sizeof(name));
        string md5tem = md5;
        md5tem += ('0'+i/100);
        md5tem += ('0'+(i%100)/10);
        md5tem += ('0'+(i%10));
        strcpy(name,md5tem.c_str());
        //cout<<md5<<endl;
        FILE *fp = fopen(name,"rb+");
        while(!feof(fp))
        {
            int ret = fread(filebuf, 1,1024*1024, fp);
            fwrite(filebuf, 1, ret, fd);
        }
        fclose(fp);
        remove(name);
    }
    fclose(fd);
    mgergeSend();
}
