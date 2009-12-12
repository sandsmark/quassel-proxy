#include "proxymessageprocessor.h"
#include "proxyuser.h"
ProxyMessageProcessor::ProxyMessageProcessor(QObject *parent,ProxyUser *proxy):AbstractMessageProcessor(parent){
  this->proxy=proxy;
}
void ProxyMessageProcessor::reset(){
}
void ProxyMessageProcessor::process(Message &msg){
    printf("MSG:<%s>%s\n",msg.sender().toUtf8().constData(),msg.contents().toUtf8().constData());
}
void ProxyMessageProcessor::process(QList<Message> &msgs){
    foreach(Message msg,msgs){
        printf("MSGS:<%s>%s\n",msg.sender().toUtf8().constData(),msg.contents().toUtf8().constData());
    }

}
