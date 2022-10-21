
#ifndef THREADS_H
#define THREADS_H

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    YAB_THREAD_SCSP = 0,
    YAB_THREAD_MSH2,
    YAB_THREAD_SSH2,
    YAB_THREAD_VDP,
    YAB_THREAD_GDBSTUBCLIENT,
    YAB_THREAD_GDBSTUBLISTENER,
    YAB_THREAD_NETLINKLISTENER,
    YAB_THREAD_NETLINKCONNECT,
    YAB_THREAD_NETLINKCLIENT,
    YAB_THREAD_OPENAL,
    YAB_THREAD_VIDSOFT_LAYER_NBG3,
    YAB_THREAD_VIDSOFT_LAYER_NBG2,
    YAB_THREAD_VIDSOFT_LAYER_NBG1,
    YAB_THREAD_VIDSOFT_LAYER_NBG0,
    YAB_THREAD_VIDSOFT_LAYER_RBG0,
    YAB_THREAD_VIDSOFT_VDP1,
    YAB_THREAD_VIDSOFT_PRIORITY_0,
    YAB_THREAD_VIDSOFT_PRIORITY_1,
    YAB_THREAD_VIDSOFT_PRIORITY_2,
    YAB_THREAD_VIDSOFT_PRIORITY_3,
    YAB_THREAD_VIDSOFT_PRIORITY_4,
    YAB_THREAD_VIDSOFT_LAYER_SPRITE,

    YAB_THREAD_VDP1_0,
    YAB_THREAD_VDP1_1,
    YAB_THREAD_VDP1_2,
    YAB_THREAD_VDP1_3,

    YAB_THREAD_VDP2_BACK,
    YAB_THREAD_VDP2_LINE,
    YAB_THREAD_VDP2_NBG3,
    YAB_THREAD_VDP2_NBG2,
    YAB_THREAD_VDP2_NBG1,
    YAB_THREAD_VDP2_NBG0,
    YAB_THREAD_VDP2_RBG0,
    YAB_THREAD_VDP2_RBG1,
    YAB_THREAD_CS_CMD_0,
    YAB_THREAD_CS_CMD_1,
    YAB_NUM_THREADS
};

#define YAB_NUM_SEMAPHORES 2

int YabThreadStart(unsigned int id, void *(*func)(void *), void *arg);

void YabThreadWait(unsigned int id);

void YabThreadCancel(unsigned int id);

void YabThreadYield(void);

void YabThreadSleep(void);

void YabThreadRemoteSleep(unsigned int id);

void YabThreadWake(unsigned int id);

typedef void *YabEventQueue;

YabEventQueue *YabThreadCreateQueue(int qsize);

void YabThreadDestoryQueue(YabEventQueue *queue_t);

void YabAddEventQueue(YabEventQueue *queue_t, void *evcode);

void *YabWaitEventQueue(YabEventQueue *queue_t);

int YaGetQueueSize(YabEventQueue *queue_t);

void YabWaitEmptyQueue(YabEventQueue *queue_t);

typedef void *YabSem;

void    YabSemPost(YabSem *mtx);
void    YabSemWait(YabSem *mtx);
YabSem *YabThreadCreateSem(int val);
void    YabThreadFreeMutex(YabSem *mtx);

typedef void *YabMutex;

void      YabThreadLock(YabMutex *mtx);
void      YabThreadUnLock(YabMutex *mtx);
YabMutex *YabThreadCreateMutex();
void      YabThreadFreeMutex(YabMutex *mtx);

typedef void *YabCond;

void     YabThreadCondWait(YabCond *cond, YabMutex *mtx);
void     YabThreadCondSignal(YabCond *cond);
YabCond *YabThreadCreateCond();
void     YabThreadFreeCond(YabCond *mtx);

typedef void *YabBarrier;

void        YabThreadBarrierWait(YabBarrier *bar);
YabBarrier *YabThreadCreateBarrier(int nbWorkers);

void YabThreadSetCurrentThreadAffinityMask(int mask);
int  YabThreadGetCurrentThreadAffinityMask();

void YabThreadUSleep(unsigned int stime);

#ifdef __cplusplus
}
#endif

#endif
