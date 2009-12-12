#ifndef PROTOCONVERT_H
#define PROTOCONVERT_H
#include <QtCore>
#include <QtNetwork>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include "identity.h"
#include "bufferinfo.h"
#ifdef MAIN_CPP
#include "proxy/protocol.pb.h"
#else
#include "protocol.pb.h"
#endif
class Identity;
class Network;
class IrcUser;
class NetworkInfo;
class IrcChannel;
class Message;
void convertBufferInfoToProto(const BufferInfo *binfo,quasselproxy::Buffer* proto);
void convertNetworkToProto(const Network* net,quasselproxy::Network* proto);
void convertIrcUserToProto(const IrcUser* usr,quasselproxy::IrcUser *proto);//Currently deprecated, may be used as special whois
void convertIrcChannel(IrcChannel *chan,quasselproxy::IrcChannel *proto);
void convertIdentity(const Identity *id,quasselproxy::Identity *proto);
void convertMessage(const Message *msg,quasselproxy::Message *proto);
#endif
