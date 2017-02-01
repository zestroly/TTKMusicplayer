#include "musicdownloadqueryxmartistthread.h"
#include "musicdownloadqueryxmthread.h"
#include "musicnumberutils.h"
#include "musictime.h"
#///QJson import
#include "qjson/parser.h"

MusicDownLoadQueryXMArtistThread::MusicDownLoadQueryXMArtistThread(QObject *parent)
    : MusicDownLoadQueryThreadAbstract(parent)
{
    m_index = 0;
}

QString MusicDownLoadQueryXMArtistThread::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownLoadQueryXMArtistThread::startSearchSong(QueryType type, const QString &artist)
{
    Q_UNUSED(type);
    startSearchSong(artist);
}

void MusicDownLoadQueryXMArtistThread::startSearchSong(const QString &artist)
{
    QUrl musicUrl = MusicCryptographicHash::decryptData(XM_SONG_ARTIST_URL, URL_KEY);

    if(m_reply)
    {
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("Origin", MusicCryptographicHash::decryptData(XM_BASE_URL, URL_KEY).toUtf8());
    request.setRawHeader("Referer", MusicCryptographicHash::decryptData(XM_BASE_URL, URL_KEY).toUtf8());
#ifndef QT_NO_SSL
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslConfig);
#endif
    QNetworkReply *reply = m_manager->post(request, MusicCryptographicHash::decryptData(WY_SONG_QUERY_URL, URL_KEY).arg(artist).toUtf8());
    connect(reply, SIGNAL(finished()), SLOT(downLoadFinished()) );
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
                   SLOT(replyError(QNetworkReply::NetworkError)) );
}

void MusicDownLoadQueryXMArtistThread::downLoadFinished()
{
    emit clearAllItems();      ///Clear origin items
    m_musicSongInfos.clear();  ///Empty the last search to songsInfo
    m_songIds.clear();
    m_index = 0;

    QNetworkReply *reply = MObject_cast(QNetworkReply*, QObject::sender());
    if(reply && reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll(); ///Get all the data obtained by request
        QJson::Parser parser;
        bool ok;
        QVariant data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("data"))
            {
                QVariantList datas = value["data"].toList();
                foreach(const QVariant &var, datas)
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    value = var.toMap();
                    if(value["song_status"].toInt() != 0)
                    {
                        continue;
                    }

                    m_songIds << value["song_id"].toString();
                }
            }
        }
        startSongListQuery();
    }
    else
    {
        emit downLoadDataChanged(QString());
        deleteAll();
    }
}

void MusicDownLoadQueryXMArtistThread::songListFinished()
{
    QNetworkReply *reply = MObject_cast(QNetworkReply*, QObject::sender());
    if(reply && reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();

        QJson::Parser parser;
        bool ok;
        QVariant data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("data"))
            {
                value = value["data"].toMap();
                QVariantList datas = value["trackList"].toList();
                foreach(const QVariant &var, datas)
                {
                    if(var.isNull())
                    {
                        continue;
                    }

                    ++m_index;
                    value = var.toMap();
                    if(value["song_status"].toInt() != 0)
                    {
                        continue;
                    }

                    MusicObject::MusicSongInfomation musicInfo;
                    musicInfo.m_singerName = value["singers"].toString();
                    musicInfo.m_songName = value["songName"].toString();
                    musicInfo.m_timeLength = MusicTime::msecTime2LabelJustified(value["length"].toInt()*1000);

                    if(m_currentType != MovieQuery)
                    {
                        musicInfo.m_songId = QString::number(value["songId"].toULongLong());
                        musicInfo.m_artistId = QString::number(value["artistId"].toULongLong());
                        musicInfo.m_smallPicUrl = MusicCryptographicHash::decryptData(XM_SONG_PIC_URL, URL_KEY) +
                                                  value["album_logo"].toString().replace("_1.", "_4.");
                        musicInfo.m_lrcUrl = value["lyric_url"].toString();

                        ///music normal songs urls
                        QVariantList auditions = value["allAudios"].toList();
                        foreach(const QVariant &audition, auditions)
                        {
                            QVariantMap audUrlsValue = audition.toMap();
                            if(audUrlsValue.isEmpty())
                            {
                                continue;
                            }

                            int bitrate = map2NormalBitrate(audUrlsValue["rate"].toInt());
                            if(m_queryAllRecords)
                            {
                                readFromMusicSongAttribute(&musicInfo, audUrlsValue, bitrate);
                            }
                            else
                            {
                                if(m_searchQuality == tr("ST") && bitrate == MB_32)
                                    readFromMusicSongAttribute(&musicInfo, audUrlsValue, MB_32);
                                else if(m_searchQuality == tr("SD") && bitrate == MB_128)
                                    readFromMusicSongAttribute(&musicInfo, audUrlsValue, MB_128);
                                else if(m_searchQuality == tr("HD") && bitrate == MB_192)
                                    readFromMusicSongAttribute(&musicInfo, audUrlsValue, MB_192);
                                else if(m_searchQuality == tr("SQ") && bitrate == MB_320)
                                    readFromMusicSongAttribute(&musicInfo, audUrlsValue, MB_320);
                                else if(m_searchQuality == tr("CD") && bitrate == MB_500)
                                    readFromMusicSongAttribute(&musicInfo, audUrlsValue, MB_500);
                            }
                        }

                        if(musicInfo.m_songAttrs.isEmpty())
                        {
                            continue;
                        }

                        emit createSearchedItems(musicInfo.m_songName, musicInfo.m_singerName, musicInfo.m_timeLength);
                        m_musicSongInfos << musicInfo;

                        if( m_index >= m_songIds.count() || m_musicSongInfos.count() == 0)
                        {
                            emit downLoadDataChanged(QString());
                            deleteAll();
                            return;
                        }
                    }
                }
            }
        }
    }
    else
    {
        emit downLoadDataChanged(QString());
        deleteAll();
    }
}

void MusicDownLoadQueryXMArtistThread::startSongListQuery()
{
    foreach(const QString &id, m_songIds)
    {
        QNetworkRequest request;
        request.setUrl(MusicCryptographicHash::decryptData(WY_SONG_PLAYLIST_URL, URL_KEY).arg(id));
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        request.setRawHeader("Origin", MusicCryptographicHash::decryptData(XM_BASE_URL, URL_KEY).toUtf8());
        request.setRawHeader("Referer", MusicCryptographicHash::decryptData(XM_BASE_URL, URL_KEY).toUtf8());
    #ifndef QT_NO_SSL
        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);
    #endif
        QNetworkReply *reply = m_manager->get(request);
        connect(reply, SIGNAL(finished()), SLOT(songListFinished()) );
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)) );
    }
}

void MusicDownLoadQueryXMArtistThread::readFromMusicSongAttribute(MusicObject::MusicSongInfomation *info,
                                const QVariantMap &key, int bitrate)
{
    MusicObject::MusicSongAttribute attr;
    attr.m_url = key["filePath"].toString();
    attr.m_size = MusicUtils::Number::size2Label(key["fileSize"].toInt());
    attr.m_format = key["format"].toString();
    attr.m_bitrate = bitrate;
    info->m_songAttrs.append(attr);
}

int MusicDownLoadQueryXMArtistThread::map2NormalBitrate(int bitrate)
{
    if(bitrate > 0 && bitrate < 128)
        return MB_32;
    else if(bitrate > 128 && bitrate < 192)
        return MB_128;
    else if(bitrate > 192 && bitrate < 320)
        return MB_320;
    else if(bitrate > 320)
        return MB_500;
    else
        return bitrate;
}
