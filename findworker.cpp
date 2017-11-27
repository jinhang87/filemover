#include "findworker.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>
#include <QThread>

static void findRecursion(const QString &path, const QString &pattern, QStringList *result)
{
    QDir currentDir(path);
    const QString prefix = path + QLatin1Char('/');
    foreach (const QString &match, currentDir.entryList(QStringList(pattern), QDir::Files | QDir::NoSymLinks))
        result->append(prefix + match);
    foreach (const QString &dir, currentDir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot))
        findRecursion(prefix + dir, pattern, result);
}

FindWorker::FindWorker(QObject *parent) : QObject(parent)
{

}

void FindWorker::onFindWork(const QString &path)
{
    qDebug() << "FindWorker::onFindWork! " << QThread::currentThreadId();
    QDir currentDir = QDir(path);
    QFile file("data.txt");
    if(!file.open(QFile::ReadOnly | QIODevice::Text)){
        qDebug() << "file open failed!";
        emit fileNotFind();
        return;
    }
    int total = 0;
    QTextStream in(&file);
    while(!in.atEnd()){
        if(m_cancel){
            m_cancel = false;
            break;
        }

        //读取一行
        QString line = in.readLine();
        QStringList lineGroup = line.split("\t");
        //qDebug() << lineGroup[0] << lineGroup[6];

        //无车牌跳过
        if(lineGroup[0] == QStringLiteral("无车牌")){
            continue;
        }

#if 0
        fileComboBox->lineEdit()->setText(lineGroup[0]);
#endif
        emit plateChanged(lineGroup[0]);

        //查找文件
        //QString fileName = QString(u8"*鲁*");
        QString fileName = QString(u8"*%1*").arg(lineGroup[0]);
        QStringList files;
        findRecursion(path, fileName.isEmpty() ? QStringLiteral("*") : fileName, &files);
        if(files.size() == 0){
            //qDebug() << "找不到车牌";
            continue;
        }

        //创建文件夹
        QString newDirName = QDir::cleanPath(path + QLatin1String("/../") + lineGroup[6]);
        qDebug() << newDirName;
        QDir *temp = new QDir;
        bool exist = temp->exists(newDirName);
        if(!exist){
            temp->mkdir(newDirName);
        }

        //拷贝文件
        for(auto f : files){
            QFileInfo fileInfo(f);
            QString newFileDirName = newDirName + QLatin1Char('/') + fileInfo.fileName();
            QFileInfo NewFileInfo(newFileDirName);
            if(temp->exists(newFileDirName)){
                temp->remove(newFileDirName);
            }
            if(!QFile::copy(f, newFileDirName)){
                qDebug() << "copy error" << f << " to " << newFileDirName;
            }
        }
        total += files.size();
#if 0
        filesFoundLabel->setText(QString("复制 %1 个文件!").arg(total));
        filesFoundLabel->setWordWrap(true);
#endif
        emit totalChanged(total);
        delete temp;
    }
    file.close();

    if(total==0){
#if 0
        filesFoundLabel->setText(QString("找不到文件"));
#endif
        emit totalNone();
    }
}

void FindWorker::cancel()
{
    qDebug() << "FindWorker::cancel! " << QThread::currentThreadId();;
    m_cancel = true;
}
