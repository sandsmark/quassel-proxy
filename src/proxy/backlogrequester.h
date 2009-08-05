#ifndef BACKLOGREQUESTER_H
#define BACKLOGREQUESTER_H
#include "backlogmanager.h"
class BacklogRequester : public BacklogManager
{
    Q_OBJECT;
public:
    BacklogRequester();
    virtual void receiveBacklog(BufferId, MsgId, MsgId, int, int, QVariantList);
    virtual void receiveBacklogAll(MsgId, MsgId, int, int, QVariantList);
signals:
    void backlogRecieved(BufferId, MsgId, MsgId, int, int, QVariantList);
    void backlogRecievedAll(MsgId, MsgId, int, int, QVariantList);

};

#endif // BACKLOGREQUESTER_H
