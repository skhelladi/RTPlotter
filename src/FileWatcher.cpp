#include "FileWatcher.h"

FileWatcher::FileWatcher(QObject *parent) : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &FileWatcher::onFileChanged);
}

void FileWatcher::watchFile(const QString& path)
{
    m_watcher.addPath(path);
}

void FileWatcher::stop()
{
    m_watcher.removePaths(m_watcher.files());
}


void FileWatcher::onFileChanged(const QString& path)
{
    emit fileChanged(path);
}
