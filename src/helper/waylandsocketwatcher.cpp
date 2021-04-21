/***************************************************************************
* Copyright (c) 2021 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
* Copyright (C) 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the
* Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
***************************************************************************/

#include <QDebug>
#include <QStandardPaths>

#include "waylandsocketwatcher.h"

namespace SDDM {

WaylandSocketWatcher::WaylandSocketWatcher(QObject *parent )
    : QObject(parent)
    , m_runtimeDir(QDir(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)))
    , m_socketPath(m_runtimeDir.absoluteFilePath(QLatin1String("wayland-0")))
{
}

WaylandSocketWatcher::Status WaylandSocketWatcher::status() const
{
    return m_status;
}

QString WaylandSocketWatcher::socketPath() const
{
    return m_socketPath;
}

void WaylandSocketWatcher::start()
{
    m_watcher = new QFileSystemWatcher(this);

    // Give the compositor some time to start
    m_timer.setSingleShot(true);
    m_timer.setInterval(15000);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        // Time is up and a socket was not found
        if (!m_watcher.isNull())
            m_watcher->deleteLater();
        qWarning("Wayland socket watcher for \"%s\" timed out",
                 qPrintable(m_socketPath));
        m_status = Failed;
        Q_EMIT failed();
    });

    // Check if the socket exists
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
            [this](const QString &path) {
        qDebug() << "Directory" << path << "has changed, checking for" << m_socketPath;

        if (QFile::exists(m_socketPath)) {
            m_timer.stop();
            if (!m_watcher.isNull())
                m_watcher->deleteLater();
            m_status = Started;
            Q_EMIT started();
        }
    });

    // Watch for runtime directory changes
    if (!m_runtimeDir.exists() || !m_watcher->addPath(m_runtimeDir.absolutePath())) {
        qWarning("Cannot watch directory \"%s\" for Wayland socket \"%s\"",
                 qPrintable(m_runtimeDir.absolutePath()),
                 qPrintable(m_socketPath));
        m_watcher->deleteLater();
        m_status = Failed;
        Q_EMIT failed();
    }

    // Start
    m_timer.start();
}

void WaylandSocketWatcher::stop()
{
    m_timer.stop();
    if (!m_watcher.isNull())
        m_watcher->deleteLater();
    m_watcher.clear();
    m_status = Stopped;
    Q_EMIT stopped();
}

} // namespace SDDM
