#ifndef FINDWORKER_H
#define FINDWORKER_H

#include <QObject>

class FindWorker : public QObject
{
    Q_OBJECT
public:
    explicit FindWorker(QObject *parent = 0);

signals:
    void plateChanged(const QString &text);
    void totalChanged(int total);
    void totalNone();
    void totalOver();
    void fileNotFind();


public slots:
    void onFindWork(const QString &path);
    void cancel();

private:
    bool m_cancel = false;
};

#endif // FINDWORKER_H
