#include "protoconvert.h"
#include "identity.h"
#include "bufferinfo.h"
#include "message.h"
#include "network.h"
#include "protocol.pb.h"
#include "prototools.h"

void convertBufferInfoToProto(const BufferInfo *binfo,quasselproxy::Buffer* proto){
    proto->set_bid(binfo->bufferId().toInt());
    proto->set_nid(binfo->networkId().toInt());
    proto->set_gid(binfo->groupId());
    proto->set_name(toStdStringUtf8(binfo->bufferName()));
    proto->set_type_invalid((binfo->type()&BufferInfo::InvalidBuffer)==BufferInfo::InvalidBuffer);
    proto->set_type_status((binfo->type()&BufferInfo::StatusBuffer)==BufferInfo::StatusBuffer);
    proto->set_type_channel((binfo->type()&BufferInfo::ChannelBuffer)==BufferInfo::ChannelBuffer);
    proto->set_type_query((binfo->type()&BufferInfo::QueryBuffer)==BufferInfo::QueryBuffer);
    proto->set_type_group((binfo->type()&BufferInfo::GroupBuffer)==BufferInfo::GroupBuffer);
    //FIXME: Check how to send activity
    /*if(bufferActivivty.contains(binfo->bufferId().toInt())){//FIXME: Send activity info separatly
        BufferInfo::ActivityLevel actLvl=BufferInfo::ActivityLevel(bufferActivivty.value(binfo->bufferId().toInt()));
        proto->set_activity_none(actLvl.testFlag(BufferInfo::NoActivity));
        proto->set_activity_other(actLvl.testFlag(BufferInfo::OtherActivity));
        proto->set_activity_newmessage(actLvl.testFlag(BufferInfo::NewMessage));
        proto->set_activity_highlight(actLvl.testFlag(BufferInfo::Highlight));
    }*/
}
void convertNetworkToProto(const Network* net,quasselproxy::Network* proto){
    proto->set_nid(net->networkId().toInt());
    proto->set_name(toStdStringUtf8(net->networkName()));
    proto->set_idid(net->identity().toInt());
    proto->set_userandomserver(net->useRandomServer());
    foreach(QString perf,net->perform()){
        proto->add_perform(toStdStringUtf8(perf));
    }
    proto->set_useautoidentify(net->useAutoIdentify());
    proto->set_autoreconnectinterval(net->autoReconnectInterval());
    proto->set_unlimitedreconnectretries(net->unlimitedReconnectRetries());
    proto->set_rejoinchannels(net->rejoinChannels());
    switch(net->connectionState()){
        case Network::Disconnected:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Disconnected);
            break;
        case Network::Connecting:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Connecting);
            break;
        case Network::Initializing:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Initializing);
            break;
        case Network::Initialized:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Initialized);
            break;
        case Network::Reconnecting:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Reconnecting);
            break;
        case Network::Disconnecting:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Disconnecting);
            break;
    }
    /*switch(net->channelModeType()){
        case Network::A_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_A_CHANMODE);
            break;
        case Network::B_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_B_CHANMODE);
            break;
        case Network::C_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_C_CHANMODE);
            break;
        case Network::D_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_D_CHANMODE);
            break;
    }*/
}
void convertIrcUserToProto(const IrcUser* usr,quasselproxy::IrcUser *proto){//Currently deprecated, may be used as special whois
#ifdef COMMENTED_OUT_TO_FIX_COMPILE
  proto->set_nid(usr->network()->networkId().toInt());
  if(!usersInt.contains(ChannelInfo(usr->network()->networkId().toInt(),usr->nick()))){
      printf("User not found:(%s)%s\n",usr->network()->networkName().toUtf8().constData(),usr->nick().toUtf8().constData());
      return;
  }

  proto->set_uid(usersInt.value(ChannelInfo(usr->network()->networkId().toInt(),usr->nick())));
#endif
  proto->set_nick(toStdStringUtf8(usr->nick()));
  proto->set_user(toStdStringUtf8(usr->user()));
  proto->set_host(toStdStringUtf8(usr->host()));
  proto->set_realname(toStdStringUtf8(usr->realName()));
  //proto->set_awaymessage(toStdStringUtf8(usr->awayMessage()));//send this?
  proto->set_away(usr->isAway());
  proto->set_server(toStdStringUtf8(usr->server()));
  proto->set_logintime(usr->loginTime().toTime_t());
  proto->set_operator_(toStdStringUtf8(usr->ircOperator()));
  proto->set_suserhost(toStdStringUtf8(usr->suserHost()));
  proto->set_usermodes(toStdStringUtf8(usr->userModes()));
#ifdef COMMENTED_OUT_TO_FIX_COMPILE
  foreach(QString chan,usr->channels())
      proto->add_channels(channelsInt.value(ChannelInfo(usr->network()->identity().toInt(),chan)));
#endif
  printf("Adduser:%s\n",usr->nick().toUtf8().constData());
}
void convertIrcChannel(IrcChannel *chan,quasselproxy::IrcChannel *proto){//Currently deprecated, may be used for user list etc.
/*
  //Mode stuf, mirrors internal quassel stuf, not understood, used or implemented yet
  message AMode{
    required bytes mode=1;
    repeated string value=2;
  }
  message BCMode{
    required bytes mode=1;
    optional string value=2;
  }
  repeated AMode amodes=10;
  repeated BCMode bmodes=11;
  repeated BCMode cmodes=12;
  repeated bytes dmodes=13;

*/
#ifdef COMMENTED_OUT_TO_FIX_COMPILE
    if(!channelsInt.contains(ChannelInfo(chan->network()->networkId().toInt(),chan->name()))){
        printf("Channel not found:(%s)%s\n",chan->network()->networkName().toUtf8().constData(),chan->name().toUtf8().constData());
        return;
    }
    proto->set_cid(channelsInt.value(ChannelInfo(chan->network()->networkId().toInt(),chan->name())));
#endif
    proto->set_nid(chan->network()->networkId().toInt());
    proto->set_name(toStdStringUtf8(chan->name()));
    proto->set_topic(toStdStringUtf8(chan->topic()));
    proto->set_password(toStdStringUtf8(chan->password()));
    //proto->set
    quint32 nid=chan->network()->networkId().toInt();
#ifdef COMMENTED_OUT_TO_FIX_COMPILE
    foreach(IrcUser *usr,chan->ircUsers()){
        if(usersInt.contains(ChannelInfo(nid,usr->nick()))){//don't add user if it doesn't exist
            quasselproxy::IrcChannel::UserMode *um =proto->add_users();
            um->set_uid(usersInt.value(ChannelInfo(usr->network()->networkId().toInt(),usr->nick())));
            printf("SCID%d,%s;%s\n",um->uid(),usr->network()->networkName().toUtf8().constData(),usr->nick().toUtf8().constData());
            um->set_mode(toStdStringUtf8(chan->userModes(usr->nick())));
        }
    }
    //FIXME:Implement mode stuff
#endif
}
void convertIdentity(const Identity *id,quasselproxy::Identity *proto){
    proto->set_idid(id->id().toInt());
    proto->set_identityname(toStdStringUtf8(id->identityName()));
    proto->set_realname(toStdStringUtf8(id->realName()));
    foreach(QString nick,id->nicks())
        proto->add_nicks(toStdStringUtf8(nick));
    proto->set_awaynick(toStdStringUtf8(id->awayNick()));
    proto->set_awaynickenabled(proto->awaynickenabled());
    proto->set_awayreason(toStdStringUtf8(id->awayReason()));
    proto->set_awayreasonenabled(id->awayReasonEnabled());
    proto->set_detatchawayenabled(id->detachAwayEnabled());
    proto->set_detatchawayreason(toStdStringUtf8(id->detachAwayReason()));
    proto->set_ident(toStdStringUtf8(id->ident()));
    proto->set_kickreason(toStdStringUtf8(id->kickReason()));
    proto->set_partreason(toStdStringUtf8(id->partReason()));
    proto->set_quitreason(toStdStringUtf8(id->quitReason()));
}
