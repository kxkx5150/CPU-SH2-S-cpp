
#ifndef VIDOGL_H
#define VIDOGL_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)

#include "vdp1.h"

#define VIDCORE_OGL 1

extern VideoInterface_struct VIDOGL;
#endif

#ifdef __cplusplus
}
#endif

#endif
