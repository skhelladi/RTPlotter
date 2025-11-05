#ifndef FILEWATCHER_H
#define FILEWATCHER_H

#include <QObject>
#include <QFileSystemWatcher>

class FileWatcher : public QObject
{
    Q_OBJECT
public:
    explicit FileWatcher(QObject *parent = nullptr);
    void watchFile(const QString& path);
        void stop();
        QStringList files() const { return m_watcher.files(); }
    
    signals:
        void fileChanged(const QString& path);

private slots:
    void onFileChanged(const QString& path);

private:
    QFileSystemWatcher m_watcher;
};

#endif // FILEWATCHER_H
