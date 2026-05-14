#ifndef __MSG_DELIVER_H
#define __MSG_DELIVER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Module_ID_e mod_id;
    MsgHandler_t handler;
    MsgReleaseHandler_t release_handler;
} MsgRouteTable_t;

void msg_deliver_lock(void);
void msg_deliver_unlock(void);
void msg_thread_wakeup(void);

#ifdef __cplusplus
}
#endif

#endif
