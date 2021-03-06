/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtWidgets>
#include <QDebug>
#include <QFile>
#include "window.h"
#include <QMessageBox>

//! [17]
enum { absoluteFileNameRole = Qt::UserRole + 1 };
//! [17]

//! [18]
static inline QString fileNameOfItem(const QTableWidgetItem *item)
{
    return item->data(absoluteFileNameRole).toString();
}
//! [18]

//! [14]
static inline void openFile(const QString &fileName)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}
//! [14]

//! [0]
Window::Window(QWidget *parent)
    : QWidget(parent)
{
    QPushButton *browseButton = new QPushButton(tr("&Browse..."), this);
    connect(browseButton, &QAbstractButton::clicked, this, &Window::browse);
    findButton = new QPushButton(tr("&Find"), this);
    connect(findButton, &QAbstractButton::clicked, this, &Window::findList);
    cancelButton = new QPushButton(tr("&cancel"), this);
    cancelButton->setEnabled(false);

    fileComboBox = createComboBox(tr("*"));
    connect(fileComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &Window::animateFindClick);
    textComboBox = createComboBox();
    textComboBox->setEnabled(false);
    connect(textComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &Window::animateFindClick);
    directoryComboBox = createComboBox(QDir::toNativeSeparators(QDir::currentPath()));
    connect(directoryComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &Window::animateFindClick);

    filesFoundLabel = new QLabel;

    createFilesTable();
    //! [0]

    //! [1]
    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(new QLabel(tr("Named:")), 0, 0);
    mainLayout->addWidget(fileComboBox, 0, 1, 1, 2);
    mainLayout->addWidget(new QLabel(tr("Containing text:")), 1, 0);
    mainLayout->addWidget(textComboBox, 1, 1, 1, 2);
    mainLayout->addWidget(new QLabel(tr("In directory:")), 2, 0);
    mainLayout->addWidget(directoryComboBox, 2, 1);
    mainLayout->addWidget(browseButton, 2, 2);
    mainLayout->addWidget(filesTable, 3, 0, 1, 3);
    mainLayout->addWidget(filesFoundLabel, 4, 1, 1, 1);
    mainLayout->addWidget(findButton, 4, 0);
    mainLayout->addWidget(cancelButton, 4, 2);

    setWindowTitle(tr("Find Files"));
    const QRect screenGeometry = QApplication::desktop()->screenGeometry(this);
    resize(screenGeometry.width() / 2, screenGeometry.height() / 3);

    worker = new FindWorker;
    thread = new QThread;
    //线程和具体任务操作绑定
    worker->moveToThread(thread);
    //线程结束信号绑定
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    //线程控制信号绑定
    connect(this, &Window::findWork, worker, &FindWorker::onFindWork);
    connect(this, &Window::cancel, worker, &FindWorker::cancel);
    connect(worker, &FindWorker::totalChanged, this , [=](int total){
        filesFoundLabel->setText(QString("already copy %1 files! ").arg(total));
        filesFoundLabel->setWordWrap(true);
    });
    connect(worker, &FindWorker::totalOver, this , [=](){
        findButton->setEnabled(true);
        cancelButton->setEnabled(false);
    });
    connect(worker, &FindWorker::totalNone, this, [=](){
        filesFoundLabel->setText(QString("can not find source file"));
        filesFoundLabel->setWordWrap(true);
        findButton->setEnabled(true);
        cancelButton->setEnabled(false);
        qDebug() << "can not find source file!";
    });
    connect(worker, &FindWorker::fileNotFind, this, [=](){
        filesFoundLabel->setText(QString("can not find configuration file"));
        findButton->setEnabled(true);
        cancelButton->setEnabled(false);
        qDebug() << "can not find configuration file!";
    });
    connect(worker, &FindWorker::plateChanged, this , [=](const QString &text){
        fileComboBox->lineEdit()->setText(text);
    });
    connect(cancelButton, &QAbstractButton::clicked, this, [=](){
        qDebug() << "cancel!";
        //QMetaObject::invokeMethod(worker, "cancel");
        //emit cancel();
        worker->cancel();
    });

    thread->start();
    qDebug() << "MainWindow threadid : " << QThread::currentThreadId();
}

Window::~Window()
{
    thread->quit();
    thread->wait();
    delete thread; //emit QThread::finished
}
//! [1]

//! [2]
void Window::browse()
{
    QString directory =
            QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Find Files"), QDir::currentPath()));

    if (!directory.isEmpty()) {
        if (directoryComboBox->findText(directory) == -1)
            directoryComboBox->addItem(directory);
        directoryComboBox->setCurrentIndex(directoryComboBox->findText(directory));
    }
}
//! [2]

static void updateComboBox(QComboBox *comboBox)
{
    if (comboBox->findText(comboBox->currentText()) == -1)
        comboBox->addItem(comboBox->currentText());
}

//! [13]

static void findRecursion(const QString &path, const QString &pattern, QStringList *result)
{
    QDir currentDir(path);
    const QString prefix = path + QLatin1Char('/');
    foreach (const QString &match, currentDir.entryList(QStringList(pattern), QDir::Files | QDir::NoSymLinks))
        result->append(prefix + match);
    foreach (const QString &dir, currentDir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot))
        findRecursion(prefix + dir, pattern, result);
}

//! [13]
//! [3]

void Window::find()
{
    filesTable->setRowCount(0);

    QString fileName = fileComboBox->currentText();
    QString text = textComboBox->currentText();
    QString path = QDir::cleanPath(directoryComboBox->currentText());
    //! [3]

    updateComboBox(fileComboBox);
    updateComboBox(textComboBox);
    updateComboBox(directoryComboBox);

    //! [4]

    currentDir = QDir(path);
    QStringList files;
    findRecursion(path, fileName.isEmpty() ? QStringLiteral("*") : fileName, &files);
    if (!text.isEmpty())
        files = findFiles(files, text);
    showFiles(files);
}

void Window::findList()
{
    findButton->setEnabled(false);
    cancelButton->setEnabled(true);
    QString path = QDir::cleanPath(directoryComboBox->currentText());
    qDebug() << "Window::findList " << path;
    //QMetaObject::invokeMethod(worker, "onFindWork", Q_ARG(const QString &, path));
    emit findWork(path);
    #if 1
#else
    currentDir = QDir(path);
    QFile file("data.txt");
    if(!file.open(QFile::ReadOnly | QIODevice::Text)){
        qDebug() << "file open failed!";
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

        //查找文件
        //QString fileName = QString(u8"*鲁*");
        QString fileName = QString(u8"*%1*").arg(lineGroup[0]);
        QStringList files;
        findRecursion(path, fileName.isEmpty() ? QStringLiteral("*") : fileName, &files);
        if(files.size() == 0){
            //qDebug() << "找不到车牌";
            continue;
        }
        fileComboBox->lineEdit()->setText(lineGroup[0]);

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
        filesFoundLabel->setText(QString("复制 %1 个文件!").arg(total));
        filesFoundLabel->setWordWrap(true);
        delete temp;
    }
    file.close();

    if(total==0){
        filesFoundLabel->setText(QString("找不到文件"));
    }

    findButton->setEnabled(true);
    cancelButton->setEnabled(false);
#endif
}

//! [4]

void Window::animateFindClick()
{
    findButton->animateClick();
}

//! [5]
QStringList Window::findFiles(const QStringList &files, const QString &text)
{
    QProgressDialog progressDialog(this);
    progressDialog.setCancelButtonText(tr("&Cancel"));
    progressDialog.setRange(0, files.size());
    progressDialog.setWindowTitle(tr("Find Files"));

    //! [5] //! [6]
    QMimeDatabase mimeDatabase;
    QStringList foundFiles;

    for (int i = 0; i < files.size(); ++i) {
        progressDialog.setValue(i);
        progressDialog.setLabelText(tr("Searching file number %1 of %n...", 0, files.size()).arg(i));
        QCoreApplication::processEvents();
        //! [6]

        if (progressDialog.wasCanceled())
            break;

        //! [7]
        const QString fileName = files.at(i);
        const QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileName);
        if (mimeType.isValid() && !mimeType.inherits(QStringLiteral("text/plain"))) {
            qWarning() << "Not searching binary file " << QDir::toNativeSeparators(fileName);
            continue;
        }
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QString line;
            QTextStream in(&file);
            while (!in.atEnd()) {
                if (progressDialog.wasCanceled())
                    break;
                line = in.readLine();
                if (line.contains(text, Qt::CaseInsensitive)) {
                    foundFiles << files[i];
                    break;
                }
            }
        }
    }
    return foundFiles;
}
//! [7]

//! [8]
void Window::showFiles(const QStringList &files)
{
    for (int i = 0; i < files.size(); ++i) {
        const QString &fileName = files.at(i);
        const QString toolTip = QDir::toNativeSeparators(fileName);
        const QString relativePath = QDir::toNativeSeparators(currentDir.relativeFilePath(fileName));
        const qint64 size = QFileInfo(fileName).size();
        QTableWidgetItem *fileNameItem = new QTableWidgetItem(relativePath);
        fileNameItem->setData(absoluteFileNameRole, QVariant(fileName));
        fileNameItem->setToolTip(toolTip);
        fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *sizeItem = new QTableWidgetItem(tr("%1 KB")
                                                          .arg(int((size + 1023) / 1024)));
        sizeItem->setData(absoluteFileNameRole, QVariant(fileName));
        sizeItem->setToolTip(toolTip);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);

        int row = filesTable->rowCount();
        filesTable->insertRow(row);
        filesTable->setItem(row, 0, fileNameItem);
        filesTable->setItem(row, 1, sizeItem);
    }
    filesFoundLabel->setText(tr("%n file(s) found (Double click on a file to open it)", 0, files.size()));
    filesFoundLabel->setWordWrap(true);
}
//! [8]

//! [10]
QComboBox *Window::createComboBox(const QString &text)
{
    QComboBox *comboBox = new QComboBox;
    comboBox->setEditable(true);
    comboBox->addItem(text);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return comboBox;
}
//! [10]

//! [11]
void Window::createFilesTable()
{
    filesTable = new QTableWidget(0, 2);
    filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList labels;
    labels << tr("Filename") << tr("Size");
    filesTable->setHorizontalHeaderLabels(labels);
    filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    filesTable->verticalHeader()->hide();
    filesTable->setShowGrid(false);
    //! [15]
    filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(filesTable, &QTableWidget::customContextMenuRequested,
            this, &Window::contextMenu);
    connect(filesTable, &QTableWidget::cellActivated,
            this, &Window::openFileOfItem);
    //! [15]
}
//! [11]


//! [12]

void Window::openFileOfItem(int row, int /* column */)
{
    const QTableWidgetItem *item = filesTable->item(row, 0);
    openFile(fileNameOfItem(item));
}

//! [12]

//! [16]
void Window::contextMenu(const QPoint &pos)
{
    const QTableWidgetItem *item = filesTable->itemAt(pos);
    if (!item)
        return;
    QMenu menu;
#ifndef QT_NO_CLIPBOARD
    QAction *copyAction = menu.addAction("Copy Name");
#endif
    QAction *openAction = menu.addAction("Open");
    QAction *action = menu.exec(filesTable->mapToGlobal(pos));
    if (!action)
        return;
    const QString fileName = fileNameOfItem(item);
    if (action == openAction)
        openFile(fileName);
#ifndef QT_NO_CLIPBOARD
    else if (action == copyAction)
        QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(fileName));
#endif
}


//! [16]



