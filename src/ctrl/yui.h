
#ifndef YUI_H
#define YUI_H

#include "cdbase.h"
#include "sh2core.h"
#include "sh2int.h"
#include "scsp.h"
#include "smpc.h"
#include "vdp1.h"
#include "yabause.h"

#ifdef __cplusplus
extern "C" {
#endif

void YuiMsg(const char *format, ...);

void YuiErrorMsg(const char *string);

int  YuiGetFB(void);
void YuiSwapBuffers(void);

void YuiEndOfFrame(void);

int YuiUseOGLOnThisThread();

int YuiRevokeOGLOnThisThread();

#ifndef TIMING_SWAP
#define YuiTimedSwapBuffers YuiSwapBuffers
#else
void YuiTimedSwapBuffers(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
