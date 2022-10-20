#ifndef VDP1_COMPUTE_H
#define VDP1_COMPUTE_H

#include "vdp1_prog_compute.h"
#include "vdp1.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
  VDP1_MESH_STANDARD_BANDING = 0,
  VDP1_MESH_IMPROVED_BANDING,
  VDP1_MESH_STANDARD_NO_BANDING,
  VDP1_MESH_IMPROVED_NO_BANDING,
  WRITE,
  READ,
  CLEAR,
  CLEAR_MESH,
  NB_PRG
};

extern void vdp1_compute_init(int width, int height, float ratiow, float ratioh);
extern void vdp1_compute();
extern int get_vdp1_tex(int);
extern int get_vdp1_mesh(int);
extern int vdp1_add(vdp1cmd_struct* cmd, int clipcmd);
extern void vdp1_clear(int id, float *col);
extern u32* vdp1_get_directFB();
extern void vdp1_set_directFB();
extern void vdp1_setup(void);

#ifdef __cplusplus
}
#endif

#endif //VDP1_COMPUTE_H
