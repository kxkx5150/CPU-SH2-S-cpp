
#ifndef VIDCS_H
#define VIDCS_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
#include "vdp1.h"
#define VIDCORE_CS 2
extern VideoInterface_struct VIDCS;
#endif

#ifdef __cplusplus
}
#endif

#endif
