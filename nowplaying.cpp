/*
 *   Copyright 2007,2008 by Alex Merry <alex.merry@kdemail.net>
 *   Copyright 2008 by Tony Murray <murraytony@gmail.com>
 *
 *   Some code (text size calculation) taken from clock applet:
 *   Copyright 2007 by Sebastian Kuegler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .
 */

#include "nowplaying.h"
#include "infopanel.h"

#include <KDebug>

#include <Plasma/Slider>

#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>


K_EXPORT_PLASMA_APPLET(nowplaying, NowPlaying)


NowPlaying::NowPlaying(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args),
      m_controller(0),
      m_state(NoPlayer),
      m_currentLayout(NoLayout),
      m_volume(0),
      m_length(0),
      m_textPanel(new InfoPanel)
{
    resize(300, 200); // ideal planar size

    connect(this, SIGNAL(metadataChanged(QMap<QString,QString>)),
            m_textPanel, SLOT(updateMetadata(QMap<QString,QString>)));
    connect(this, SIGNAL(coverChanged(QPixmap)),
            m_textPanel, SLOT(updateArtwork(QPixmap)));
}

NowPlaying::~NowPlaying()
{
}

void NowPlaying::init()
{
    switch (formFactor())
    {
        case Plasma::Horizontal:
            layoutHorizontal();
            break;
        case Plasma::Vertical:
            layoutHorizontal(); // FIXME
            break;
        default:
            layoutPlanar();
            break;
    }

    Plasma::DataEngine* nowPlayingEngine = dataEngine("nowplaying");

    if (nowPlayingEngine)
    {
        connect(nowPlayingEngine, SIGNAL(sourceAdded(QString)),
                SLOT(playerAdded(QString)));
        connect(nowPlayingEngine, SIGNAL(sourceRemoved(QString)),
                SLOT(playerRemoved(QString)));

        findPlayer();
    }
    else
    {
        kDebug() << "Now Playing engine not found";
    }
}

void NowPlaying::layoutPlanar()
{
    if (m_currentLayout != PlanarLayout)
    {
        setAspectRatioMode(Plasma::IgnoreAspectRatio);
        setMinimumSize(200, 100);

        Plasma::ToolTipManager::self()->unregisterWidget(this);

        QGraphicsGridLayout* layout = new QGraphicsGridLayout();
        m_textPanel->show();
        layout->addItem(m_textPanel, 0, 0);

        setLayout(layout);

        m_currentLayout = PlanarLayout;
    }
}

void NowPlaying::layoutHorizontal()
{
    if (m_currentLayout != HorizontalLayout)
    {
        setMinimumSize(QSizeF());

        m_textPanel->hide();

        QGraphicsLinearLayout* layout = new QGraphicsLinearLayout();

        Plasma::ToolTipManager::self()->registerWidget(this);

        kDebug() << "Minimum size before changing layout" << minimumSize();
        kDebug() << "Preferred size before changing layout" << preferredSize();
        setLayout(layout);
        kDebug() << "Minimum size after changing layout" << minimumSize();
        kDebug() << "Preferred size after changing layout" << preferredSize();

        m_currentLayout = HorizontalLayout;
    }
}

void NowPlaying::constraintsEvent(Plasma::Constraints constraints)
{
    if (constraints & Plasma::FormFactorConstraint)
    {
        switch (formFactor())
        {
            case Plasma::Horizontal:
                layoutHorizontal();
                break;
            case Plasma::Vertical:
                layoutHorizontal(); // FIXME
                break;
            default:
                layoutPlanar();
                break;
        }
    }

    if (constraints & Plasma::SizeConstraint)
    {
        switch (formFactor())
        {
            case Plasma::Horizontal:
                setPreferredSize(contentsRect().height() * 2, contentsRect().height());
                break;
            case Plasma::Vertical:
                setPreferredSize(contentsRect().width(), contentsRect().width() / 2);
                break;
            default:
                break;
        }
    }
}

void NowPlaying::dataUpdated(const QString &name,
                             const Plasma::DataEngine::Data &data)
{
                    //i18n("No media player found")
                    //i18nc("The state of a music player", "Stopped")
    if (name != m_watchingPlayer) {
        kDebug() << "Wasn't expecting an update from" << name << " but watching " << m_watchingPlayer;
        return;
    }
    if (data.isEmpty()) {
        kDebug() << "Got no data";
        findPlayer();
        return;
    }

    State newstate;
    if (data["State"].toString() == "playing") {
        newstate = Playing;
    } else if (data["State"].toString() == "paused") {
        newstate = Paused;
    } else {
        newstate = Stopped;
    }
    if (newstate != m_state) {
        emit stateChanged(newstate);
        m_state = newstate;
    }

    QString timeText;
    int length = data["Length"].toInt();
    if (length != m_length) {
        m_length = length;
    }
    if (length != 0) {
        int pos = data["Position"].toInt();
        timeText = QString::number(pos / 60) + ':' +
                   QString::number(pos % 60).rightJustified(2, '0') + " / " +
                   QString::number(length / 60) + ':' +
                   QString::number(length % 60).rightJustified(2, '0');
    }

    QMap<QString,QString> metadata;
    metadata["Artist"] = data["Artist"].toString();
    metadata["Album"] = data["Album"].toString();
    metadata["Title"] = data["Title"].toString();
    metadata["Time"] = timeText;
    metadata["Track number"] = QString::number(data["Track number"].toInt());
    metadata["Comment"] = data["Comment"].toString();
    metadata["Genre"] = data["Genre"].toString();

    // the time should usually have changed
    emit metadataChanged(metadata);

    // used for seeing when the track has changed
    if ((metadata["Title"] != m_title) || (metadata["Artist"] != m_artist))
    {
        m_title = metadata["Title"];
        m_artist = metadata["Artist"];
        m_artwork = data["Artwork"].value<QPixmap>();
        emit coverChanged(m_artwork);
        if(Plasma::ToolTipManager::self()->isVisible(this)) {
            toolTipAboutToShow();
        }
    }

    update();
}

void NowPlaying::toolTipAboutToShow()
{
    Plasma::ToolTipContent toolTip;
    if (m_state == Playing || m_state == Paused) {
        toolTip.setMainText(m_title);
        toolTip.setSubText(i18nc("song performer, displayed below the song title", "by %1", m_artist));
        toolTip.setImage(m_artwork.scaled(QSize(50,50),Qt::KeepAspectRatio));
    } else {
        toolTip.setMainText(i18n("No current track."));
    }

    Plasma::ToolTipManager::self()->setContent(this, toolTip);
}

void NowPlaying::playerAdded(const QString &name)
{
    kDebug() << "Player" << name << "added";
    if (m_watchingPlayer == "players") {
        //findPlayer();
        kDebug() << "Installing" << name << "as watched player";
        m_watchingPlayer = name;

        delete m_controller;
        m_controller = dataEngine("nowplaying")->serviceForSource(m_watchingPlayer);
        m_controller->setParent(this);
        emit controllerChanged(m_controller);
        dataEngine("nowplaying")->connectSource(m_watchingPlayer, this, 500);
    }
}

void NowPlaying::playerRemoved(const QString &name)
{
    kDebug() << "Player" << name << "removed";
    if (m_watchingPlayer == name) {
        findPlayer();
    }
}

void NowPlaying::findPlayer()
{
    QStringList players = dataEngine("nowplaying")->sources();
    kDebug() << "Looking for players.  Possibilities:" << players;
    if (players.isEmpty()) {
        m_state = NoPlayer;
        m_watchingPlayer.clear();
        delete m_controller;
        m_controller = 0;

        emit stateChanged(m_state);
        emit controllerChanged(0);
        update();
    } else {
        m_watchingPlayer = players.first();

        delete m_controller;
        m_controller = dataEngine("nowplaying")->serviceForSource(m_watchingPlayer);
        m_controller->setParent(this);
        emit controllerChanged(m_controller);
        kDebug() << "Installing" << m_watchingPlayer << "as watched player";
        dataEngine("nowplaying")->connectSource(m_watchingPlayer, this, 999);
    }
}

void NowPlaying::play()
{
    if (m_controller) {
        m_controller->startOperationCall(m_controller->operationDescription("play"));
    }
}

void NowPlaying::pause()
{
    if (m_controller) {
        m_controller->startOperationCall(m_controller->operationDescription("pause"));
    }
}

void NowPlaying::stop()
{
    if (m_controller) {
        m_controller->startOperationCall(m_controller->operationDescription("stop"));
    }
}

void NowPlaying::prev()
{
    if (m_controller) {
        m_controller->startOperationCall(m_controller->operationDescription("previous"));
    }
}

void NowPlaying::next()
{
    if (m_controller) {
        m_controller->startOperationCall(m_controller->operationDescription("next"));
    }
}

void NowPlaying::setVolume(int volumePercent)
{
    qreal volume = ((qreal)qBound(0, volumePercent, 100)) / 100;
    if (m_controller) {
        KConfigGroup op = m_controller->operationDescription("volume");
        op.writeEntry("level", volume);
        m_controller->startOperationCall(op);
    }
}

void NowPlaying::setPosition(int position)
{
    if (m_controller) {
        KConfigGroup op = m_controller->operationDescription("seek");
        op.writeEntry("seconds", position);
        m_controller->startOperationCall(op);
    }
}

#include "nowplaying.moc"
