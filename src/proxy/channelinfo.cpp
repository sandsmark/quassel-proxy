#include "channelinfo.h"

ChannelInfo::ChannelInfo()
{
}
ChannelInfo::ChannelInfo(quint32 nid,QString name)
{
    this->nid=nid;
    this->name=name;
}

ChannelInfo::ChannelInfo(const ChannelInfo& info)
{
    operator=(info);
}
void ChannelInfo::operator=(const ChannelInfo& info){
    name=info.name;
    nid=info.nid;
}

