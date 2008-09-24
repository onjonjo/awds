#ifndef __LINUXBASIC_CALLBACK_H_
#define __LINUXBASIC_CALLBACK_H_

typedef void (*linuxbasic_cb_t)(void* Basic, const char* TopoDump);

typedef struct _linuxbasic_cb {

    void*            Basic;
    linuxbasic_cb_t  Func;
} linuxbasic_cb;

#endif
