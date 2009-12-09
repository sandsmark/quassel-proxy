#include "backlogrequester.h"

BacklogRequester::BacklogRequester() : BacklogManager()
{
}
void BacklogRequester::receiveBacklog(BufferId buf, MsgId first, MsgId last, int limit, int additional, QVariantList msgs){
    emit backlogRecieved(buf,first,last,limit,additional,msgs);
}
void BacklogRequester::receiveBacklogAll( MsgId first, MsgId last, int limit, int additional, QVariantList msgs){
    emit backlogRecievedAll(first,last,limit,additional,msgs);
}
