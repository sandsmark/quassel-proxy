#ifndef CHANNELINFO_H
#define CHANNELINFO_H
#include <QString>
#include <QHash>

class ChannelInfo
{
public:
    ChannelInfo();
    ChannelInfo(quint32,QString );
    ChannelInfo(const ChannelInfo& info);
    void operator=(const ChannelInfo& info);
    quint32 nid;
    QString name;
};
uint qHash ( const ChannelInfo & key );
inline bool operator==(const ChannelInfo &c1, const ChannelInfo &c2){
     return c1.name==c2.name && c2.nid==c2.nid;
}
inline uint qHash ( const ChannelInfo & key ){
    return qHash(key.name)^qHash(key.nid);
}

#endif // CHANNELINFO_H
