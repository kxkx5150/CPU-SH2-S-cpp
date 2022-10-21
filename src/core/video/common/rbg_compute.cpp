﻿
extern "C" {
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"
#include <math.h>
}

#define YGLDEBUG

#define DEBUGWIP

const char prg_generate_rbg[] = SHADER_VERSION_COMPUTE
    "#ifdef GL_ES\n"
    "precision highp float; \n"
    "precision highp int;\n"
    "precision highp image2D;\n"
    "#endif\n"
    "layout(local_size_x = 4, local_size_y = 4) in;\n"
    "layout(rgba8, binding = 0) writeonly uniform image2D outSurface;\n"
    "layout(std430, binding = 1) readonly buffer VDP2 { uint vram[]; };\n"
    " struct vdp2rotationparameter_struct{ \n"
    " uint PlaneAddrv[16];\n"
    " float Xst;\n"
    " float Yst;\n"
    " float Zst;\n"
    " float deltaXst;\n"
    " float deltaYst;\n"
    " float deltaX;\n"
    " float deltaY;\n"
    " float A;\n"
    " float B;\n"
    " float C;\n"
    " float D;\n"
    " float E;\n"
    " float F;\n"
    " float Px;\n"
    " float Py;\n"
    " float Pz;\n"
    " float Cx;\n"
    " float Cy;\n"
    " float Cz;\n"
    " float Mx;\n"
    " float My;\n"
    " float kx;\n"
    " float ky;\n"
    " float KAst;\n"
    " float deltaKAst;\n"
    " float deltaKAx;\n"
    " uint coeftbladdr;\n"
    " int coefenab;\n"
    " int coefmode;\n"
    " int coefdatasize;\n"
    " int use_coef_for_linecolor;\n"
    " float Xp;\n"
    " float Yp;\n"
    " float dX;\n"
    " float dY;\n"
    " int screenover;\n"
    " int msb;\n"
    " uint charaddr;\n"
    " int planew, planew_bits, planeh, planeh_bits;\n"
    " int MaxH, MaxV;\n"
    " float Xsp;\n"
    " float Ysp;\n"
    " float dx;\n"
    " float dy;\n"
    " float lkx;\n"
    " float lky;\n"
    " int KtablV;\n"
    " int ShiftPaneX;\n"
    " int ShiftPaneY;\n"
    " int MskH;\n"
    " int MskV;\n"
    " uint lineaddr;\n"
    " int k_mem_type;\n"
    " uint over_pattern_name;\n"
    " int linecoefenab;\n"
    " int padding;\n"
    "};\n"
    "layout(std430, binding = 2) readonly buffer vdp2Param { \n"
    "  vdp2rotationparameter_struct para[2];\n"
    "};\n"
    "layout(std140, binding = 3) uniform  RBGDrawInfo { \n"
    "  float hres_scale; \n"
    "  float vres_scale; \n"
    "  int cellw_; \n"
    "  int cellh_; \n"
    "  uint paladdr_; \n"
    "  int pagesize; \n"
    "  int patternshift; \n"
    "  int planew; \n"
    "  int pagewh; \n"
    "  int patterndatasize; \n"
    "  uint supplementdata; \n"
    "  int auxmode; \n"
    "  int patternwh;\n"
    "  uint coloroffset;\n"
    "  int transparencyenable;\n"
    "  int specialcolormode;\n"
    "  int specialcolorfunction;\n"
    "  uint specialcode;\n"
    "  int colornumber;\n"
    "  int window_area_mode;"
    "  uint priority;\n"
    "  int startLine;\n"
    "  int endLine;\n"
    "  uint specialprimode;\n"
    "  uint specialfunction;\n"
    "  float alpha_lncl;\n"
    "  uint lncl_table_addr;\n"
    "  uint cram_mode;\n"
    "};\n"
    "layout(std430, binding = 5) readonly buffer VDP2C { uint cram[]; };\n"
    "layout(std430, binding = 6) readonly buffer ROTW { uint  rotWin[]; };\n"
    "layout(rgba8, binding = 7) writeonly uniform image2D lnclSurface;\n"
    "layout(std430, binding = 8) readonly buffer ALPHA { uint  alpha[]; };\n"
    " int GetKValue( int paramid, vec2 pos, out float ky, out float kx, out uint lineaddr ){ \n"
    "  uint kdata;\n"
    "  int kindex = int(para[paramid].deltaKAst*pos.y)+int(para[paramid].deltaKAx*pos.x); \n"
    "  if (para[paramid].coefdatasize == 2) { \n"
    "    uint addr = ( uint( int(para[paramid].coeftbladdr) + (kindex<<1)) &0x7FFFFu); \n"
    "    if( para[paramid].k_mem_type == 0) { \n"
    "	     kdata = vram[ addr>>2 ]; \n"
    "      if( (addr & 0x02u) != 0u ) { kdata >>= 16; } \n"
    "      kdata = (((kdata) >> 8 & 0xFFu) | ((kdata) & 0xFFu) << 8);\n"
    "    }else{\n"
    "      if (cram_mode != 2u) addr |= 0x800u;\n"
    "      kdata = cram[ (addr&0xFFFu)>>2  ]; \n"
    "      if( (addr & 0x02u) != 0u ) { kdata >>= 16; } \n"
    "    }\n"
    "    if ( (kdata & 0x8000u) != 0u) { return -1; }\n"
    "    float kval = 0;\n"
    "	   if((kdata&0x4000u)!=0u) kval=float( int(kdata&0x7FFFu)| int(0xFFFF8000u) )/1024.0;\n"
    "    else kval=float(kdata&0x7FFFu)/1024.0;\n"
    "    if (para[paramid].coefmode == 0) {\n"
    "			 kx = kval;\n"
    "			 ky = kval;\n"
    "    } else if (para[paramid].coefmode == 1) {\n"
    "      kx = kval;\n"
    "    } else if (para[paramid].coefmode == 2) {\n"
    "      ky = kval;\n"
    "    } \n"
    "  }else{\n"
    "    uint addr = ( uint( int(para[paramid].coeftbladdr) + (kindex<<2))&0x7FFFFu); \n"
    "    if( para[paramid].k_mem_type == 0) { \n"
    "	     kdata = vram[ addr>>2 ]; \n"
    "      kdata = ((kdata&0xFF000000u) >> 24 | ((kdata) >> 8 & 0xFF00u) | ((kdata) & 0xFF00u) << 8 | "
    "(kdata&0x000000FFu) << 24);\n"
    "    }else{\n"
    "      if (cram_mode != 2u) addr |= 0x800u;\n"
    "      kdata = cram[ (addr&0xFFFu)>>2 ]; \n"
    "      kdata = ((kdata&0xFFFF0000u)>>16|(kdata&0x0000FFFFu)<<16);\n"
    "    }\n"
    "	 if( para[paramid].linecoefenab != 0) lineaddr = (kdata >> 24) & 0x7Fu; else lineaddr = 0u;\n"
    "	 if((kdata&0x80000000u)!=0u){ return -1;}\n"
    "    float kval = 0;\n"
    "	 if((kdata&0x00800000u)!=0u) kval=float( int(kdata&0x00FFFFFFu)| int(0xFF800000u) )/65536.0;\n"
    "  else kval=float(kdata&0x00FFFFFFu)/65536.0;\n"
    "    if (para[paramid].coefmode == 0) {\n"
    "			 kx = kval;\n"
    "			 ky = kval;\n"
    "    } else if (para[paramid].coefmode == 1) {\n"
    "      kx = kval;\n"
    "    } else if (para[paramid].coefmode == 2) {\n"
    "      ky = kval;\n"
    "    } \n"
    "  }\n"
    "  return 0;\n"
    " }\n"

    "bool isWindowInside(uint x, uint y)\n"
    "{\n"
    "  uint upLx = rotWin[y] & 0xFFFFu;\n"
    "  uint upRx = (rotWin[y] >> 16) & 0xFFFFu;\n"
    "  // inside\n"
    "  if (window_area_mode == 1)\n"
    "  {\n"
    "    if (rotWin[y] == 0u) return false;\n"
    "    if (x >= upLx && x <= upRx)\n"
    "    {\n"
    "      return true;\n"
    "    }\n"
    "    else {\n"
    "      return false;\n"
    "    }\n"
    "    // outside\n"
    "  }\n"
    "  else {\n"
    "    if (rotWin[y] == 0u) return true;\n"
    "    if (x < upLx) return true;\n"
    "    if (x > upRx) return true;\n"
    "    return false;\n"
    "  }\n"
    "  return false;\n"
    "}\n"

    "uint get_cram_msb(uint colorindex) { \n"
    " uint shift = 1; \n"
    "	uint colorval = 0u; \n"
    " if (cram_mode == 2u) shift = 2; \n"
    "	colorindex = ((colorindex<<shift)&0xFFFu); \n"
    "	colorval = cram[colorindex >> 2]; \n"
    "	if ((colorindex & 0x02u) != 0u) { colorval >>= 16; } \n"
    "	return (colorval & 0x8000u); \n"
    "}\n"

    " vec4 vdp2color(uint alpha_, uint prio, uint cc_on, uint index) {\n"
    " uint ret = (((alpha_ & 0xF8u) | prio) << 24 | ((cc_on & 0x1u)<<16) | (index& 0xFEFFFFu));\n"
    " return vec4(float((ret >> 0)&0xFFu)/255.0,float((ret >> 8)&0xFFu)/255.0, float((ret >> 16)&0xFFu)/255.0, "
    "float((ret >> 24)&0xFFu)/255.0);\n"
    "}\n"

    "int PixelIsSpecialPriority( uint specialcode, uint dot ) { \n"
    "  dot &= 0xfu; \n"
    "  if ( (specialcode & 0x01u) != 0u && (dot == 0u || dot == 1u) ){ return 1;} \n"
    "  if ( (specialcode & 0x02u) != 0u && (dot == 2u || dot == 3u) ){ return 1;} \n"
    "  if ( (specialcode & 0x04u) != 0u && (dot == 4u || dot == 5u) ){ return 1;} \n"
    "  if ( (specialcode & 0x08u) != 0u && (dot == 6u || dot == 7u) ){ return 1;} \n"
    "  if ( (specialcode & 0x10u) != 0u && (dot == 8u || dot == 9u) ){ return 1;} \n"
    "  if ( (specialcode & 0x20u) != 0u && (dot == 0xau || dot == 0xbu) ){ return 1;} \n"
    "  if ( (specialcode & 0x40u) != 0u && (dot == 0xcu || dot == 0xdu) ){ return 1;} \n"
    "  if ( (specialcode & 0x80u) != 0u && (dot == 0xeu || dot == 0xfu) ){ return 1;} \n"
    "  return 0; \n"
    "} \n"

    "uint Vdp2SetSpecialPriority(uint dot) {\n"
    "  uint prio = priority;\n"
    "  if (specialprimode == 2u) {\n"
    "    prio = priority & 0xEu;\n"
    "    if ((specialfunction & 1u) != 0u) {\n"
    "      if (PixelIsSpecialPriority(specialcode, dot) != 0u)\n"
    "      {\n"
    "        prio |= 1u;\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "	return prio;\n"
    "}\n"

    "uint setCCOn(uint index, uint dot) {\n"
    "    uint cc_ = 1u;\n"
    "    switch (specialcolormode)\n"
    "    {\n"
    "    case 1:\n"
    "      if (specialcolorfunction == 0) { cc_ = 0; } break;\n"
    "    case 2:\n"
    "      if (specialcolorfunction == 0) { cc_ = 0; }\n"
    "      else { if ((specialcode & (1u << ((dot & 0xFu) >> 1))) == 0u) { cc_ = 0; } } \n"
    "      break; \n"
    "    case 3:\n"
    "	   if (get_cram_msb(index) == 0u) { cc_ = 0; }\n"
    "	   break;\n"
    "    }\n"
    "    return cc_;\n"
    "}\n"

    "vec4 Vdp2ColorRamGetColorOffset(uint offset) { \n"
    "  uint flag = 0x380u;\n"
    "  if (cram_mode == 1u) flag = 0x780u;\n"
    "  uint index = ((((lncl_table_addr&flag) | (offset&0x7Fu))<<1u)&0xFFFu);\n"
    "  uint temp = (cram[index>>2]);\n"
    "  if( (index & 0x02u) != 0u ) { temp >>= 16; } \n"
    "  return vec4(float((temp >> 0) &0x1F)/31.0, float((temp >> 5) & 0x1Fu)/31.0, float((temp >> 10) "
    "&0x1F)/31.0,alpha_lncl);\n"
    "}\n";

const char prg_continue_rbg[] =
    "void main(){ \n"
    "  int x, y;\n"
    "  int paramid = 0;\n"
    "  int cellw;\n"
    "  uint paladdr; \n"
    "  uint charaddr; \n"
    "  uint lineaddr = 0u; \n"
    "  float ky; \n"
    "  float kx; \n"
    "  uint kdata;\n"
    "  uint cc = 1u;\n"
    "  int discarded = 0;\n"
    "  uint priority_ = priority;\n"
    "  uint patternname = 0xFFFFFFFFu;\n"
    "  ivec2 texel = ivec2(gl_GlobalInvocationID.xy);\n"
    "  ivec2 size = imageSize(outSurface);\n"
    "  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
    "  if (texel.y < (startLine * vres_scale) || texel.y >= (endLine * vres_scale) ) return;\n"
    "  vec2 pos = vec2(texel) / vec2(hres_scale, vres_scale);\n"
    "  vec2 original_pos = floor(vec2(texel) / vec2(hres_scale, vres_scale));\n";

const char prg_rbg_rpmd0_2w[] =
    "//prg_rbg_rpmd0_2w\n"
    "  paramid = 0; \n"
    "  ky = para[paramid].ky; \n"
    "  kx = para[paramid].kx; \n"
    "  lineaddr = para[paramid].lineaddr; \n"
    "  if( para[paramid].coefenab != 0 ){ \n"
    "   if( GetKValue(paramid,pos,ky,kx, lineaddr ) == -1 ) { \n"
    "     if ( para[paramid].linecoefenab != 0) imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
    "     else imageStore(lnclSurface,texel,vec4(0.0));\n"
    "   	imageStore(outSurface,texel,vec4(0.0)); return; \n"
    "   } \n"
    "  }\n";

const char prg_rbg_rpmd1_2w[] =
    "//prg_rbg_rpmd1_2w\n"
    "  paramid = 1; \n"
    "  ky = para[paramid].ky; \n"
    "  kx = para[paramid].kx; \n"
    "  lineaddr = para[paramid].lineaddr; \n"
    "  if( para[paramid].coefenab != 0 ){ \n"
    "   if( GetKValue(paramid,pos,ky,kx,lineaddr ) == -1 ) { \n"
    "     if ( para[paramid].linecoefenab != 0) imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
    "     else imageStore(lnclSurface,texel,vec4(0.0));\n"
    "   	imageStore(outSurface,texel,vec4(0.0)); return;\n"
    "   } \n"
    "  }\n";

const char prg_rbg_rpmd2_2w[] = "//prg_rbg_rpmd2_2w\n"
                                "  paramid = 0; \n"
                                "  ky = para[paramid].ky; \n"
                                "  kx = para[paramid].kx; \n"
                                "  lineaddr = para[paramid].lineaddr; \n"
                                "  if( para[0].coefenab != 0 ){ \n"
                                "    if( para[1].coefenab != 0 ){ \n"
                                "      if( GetKValue(paramid,pos,ky, kx,lineaddr ) == -1 ) { \n"
                                "        paramid = 1; \n"
                                "  			 ky = para[paramid].ky; \n"
                                "        kx = para[paramid].kx; \n"
                                "  			 lineaddr = para[paramid].lineaddr; \n"
                                "        if( GetKValue(paramid,pos,ky,kx,lineaddr ) == -1 ) { \n"
                                "          if ( para[paramid].linecoefenab != 0) \n"
                                "            imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
                                "          else \n"
                                "            imageStore(lnclSurface,texel,vec4(0.0));\n"
                                "   	     imageStore(outSurface,texel,vec4(0.0));\n"
                                "          return;\n"
                                "        } \n"
                                "      } \n"
                                "    } else {\n"
                                "        if( GetKValue(paramid,pos,ky,kx,lineaddr) == -1 ) { \n"
                                "          paramid = 1;\n"
                                "          ky = para[paramid].ky; \n"
                                "          kx = para[paramid].kx; \n"
                                "          lineaddr = para[paramid].lineaddr; \n"
                                "        } \n"
                                "    }\n"
                                "  }\n";

const char prg_get_param_mode03[] = "//prg_get_param_mode03\n"
                                    "  if( isWindowInside( uint(pos.x), uint(pos.y) ) ) { "
                                    "    paramid = 0; \n"
                                    "    if( para[paramid].coefenab != 0 ){ \n"
                                    "      if( GetKValue(paramid,pos,ky,kx,lineaddr ) == -1 ) { \n"
                                    "        paramid=1;\n"
                                    "        if( para[paramid].coefenab != 0 ){ \n"
                                    "          if( GetKValue(paramid,pos,ky,kx,lineaddr ) == -1 ) { \n"
                                    "            if ( para[paramid].linecoefenab != 0) "
                                    "imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
                                    "             imageStore(lnclSurface,texel,vec4(0.0));\n"
                                    "   	       imageStore(outSurface,texel,vec4(0.0)); return;} \n"
                                    "          }else{ \n"
                                    "            ky = para[paramid].ky; \n"
                                    "            kx = para[paramid].kx; \n"
                                    "            lineaddr = para[paramid].lineaddr; \n"
                                    "          }\n"
                                    "        }\n"
                                    "      }else{\n"
                                    "        ky = para[paramid].ky; \n"
                                    "        kx = para[paramid].kx; \n"
                                    "        lineaddr = para[paramid].lineaddr; \n"
                                    "      }\n"
                                    "    }else{\n"
                                    "      paramid = 1; \n"
                                    "      if( para[paramid].coefenab != 0 ){ \n"
                                    "        if( GetKValue(paramid,pos,ky,kx,lineaddr ) == -1 ) { \n"
                                    "          paramid=0;\n"
                                    "          if( para[paramid].coefenab != 0 ){ \n"
                                    "            if( GetKValue(paramid,pos,ky,kx,lineaddr ) == -1 ) { \n"
                                    "              if ( para[paramid].linecoefenab != 0) "
                                    "imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
                                    "              else imageStore(lnclSurface,texel,vec4(0.0));\n"
                                    "   	         imageStore(outSurface,texel,vec4(0.0)); return;\n"
                                    "            } \n"
                                    "          }else{ \n"
                                    "            ky = para[paramid].ky; \n"
                                    "            kx = para[paramid].kx; \n"
                                    "            lineaddr = para[paramid].lineaddr; \n"
                                    "          }\n"
                                    "        }\n"
                                    "      }else{\n"
                                    "        ky = para[paramid].ky; \n"
                                    "        kx = para[paramid].kx; \n"
                                    "        lineaddr = para[paramid].lineaddr; \n"
                                    "      }\n"
                                    "   }\n";

const char prg_rbg_xy[] =
    "//prg_rbg_xy\n"
    "  float Xsp = para[paramid].A * ((para[paramid].Xst + para[paramid].deltaXst * original_pos.y) - "
    "para[paramid].Px) +\n"
    "  para[paramid].B * ((para[paramid].Yst + para[paramid].deltaYst * original_pos.y) - para[paramid].Py) +\n"
    "  para[paramid].C * (para[paramid].Zst - para[paramid].Pz);\n"
    "  float Ysp = para[paramid].D * ((para[paramid].Xst + para[paramid].deltaXst *original_pos.y) - para[paramid].Px) "
    "+\n"
    "  para[paramid].E * ((para[paramid].Yst + para[paramid].deltaYst * original_pos.y) - para[paramid].Py) +\n"
    "  para[paramid].F * (para[paramid].Zst - para[paramid].Pz);\n"
    "  float fh = floor(kx * (Xsp + para[paramid].dx * original_pos.x) + para[paramid].Xp);\n"
    "  float fv = floor(ky * (Ysp + para[paramid].dy * original_pos.x) + para[paramid].Yp);\n";

const char prg_rbg_get_bitmap[] =
    "//prg_rbg_get_bitmap\n"
    "  cellw = cellw_;\n"
    "  charaddr = para[paramid].charaddr;\n"
    "  paladdr = paladdr_;\n"
    "  switch( para[paramid].screenover){ \n "
    "  case 0: // OVERMODE_REPEAT \n"
    "  case 1: // OVERMODE_SELPATNAME \n"
    "    x = int(fh) & (cellw-1);\n"
    "    y = int(fv) & (cellh_-1);\n"
    "    break;\n"
    "  case 2: // OVERMODE_TRANSE \n"
    "    if ((fh < 0.0) || (fh > float(cellw_)) || (fv < 0.0) || (fv > float(cellh_)) ) {\n"
    "     if ( para[paramid].linecoefenab != 0) imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
    "     else imageStore(lnclSurface,texel,vec4(0.0));\n"
    "   	imageStore(outSurface,texel,vec4(0.0)); \n"
    "     return; \n"
    "    }\n"
    "    x = int(fh);\n"
    "    y = int(fv);\n"
    "    break;\n"
    "  case 3: // OVERMODE_512 \n"
    "    if ((fh < 0.0) || (fh > 512.0) || (fv < 0.0) || (fv > 512.0)) {\n"
    "     if ( para[paramid].linecoefenab != 0) imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
    "     else imageStore(lnclSurface,texel,vec4(0.0));\n"
    "   	imageStore(outSurface,texel,vec4(0.0)); \n"
    "     return; \n"
    "    }\n"
    "    x = int(fh);\n"
    "    y = int(fv);\n"
    "    break;\n"
    "  }\n";

const char prg_rbg_overmode_repeat[] =
    "//prg_rbg_overmode_repeat\n"
    "  switch( para[paramid].screenover){ \n "
    "  case 0: // OVERMODE_REPEAT \n"
    "    x = int(fh) & (para[paramid].MaxH-1);\n"
    "    y = int(fv) & (para[paramid].MaxV-1);\n"
    "    break;\n"
    "  case 1: // OVERMODE_SELPATNAME \n"
    "    if ((fh < 0.0) || (fh > float(para[paramid].MaxH)) || (fv < 0.0) || (fv > float(para[paramid].MaxV)) ) {\n"
    "        patternname = para[paramid].over_pattern_name;\n"
    "    }"
    "    x = int(fh);\n"
    "    y = int(fv);\n"
    "    break;\n"
    "  case 2: // OVERMODE_TRANSE \n"
    "    if ((fh < 0.0) || (fh > float(para[paramid].MaxH) ) || (fv < 0.0) || (fv > float(para[paramid].MaxV)) ) {\n"
    "     if ( para[paramid].linecoefenab != 0) imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
    "     else imageStore(lnclSurface,texel,vec4(0.0));\n"
    "   	imageStore(outSurface,texel,vec4(0.0)); \n"
    "     return; \n"
    "    }\n"
    "    x = int(fh);\n"
    "    y = int(fv);\n"
    "    break;\n"
    "  case 3: // OVERMODE_512 \n"
    "    if ((fh < 0.0) || (fh > 512.0) || (fv < 0.0) || (fv > 512.0)) {\n"
    "     if ( para[paramid].linecoefenab != 0) imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
    "     else imageStore(lnclSurface,texel,vec4(0.0));\n"
    "   	imageStore(outSurface,texel,vec4(0.0)); \n"
    "     return; \n"
    "    }\n"
    "    x = int(fh);\n"
    "    y = int(fv);\n"
    "    break;\n"
    "   }\n";

const char prg_rbg_get_patternaddr[] =
    "//prg_rbg_get_patternaddr\n"
    "  int planenum = (x >> para[paramid].ShiftPaneX) + ((y >> para[paramid].ShiftPaneY) << 2);\n"
    "  x &= (para[paramid].MskH);\n"
    "  y &= (para[paramid].MskV);\n"
    "  uint addr = para[paramid].PlaneAddrv[planenum];\n"
    "  addr += uint( (((y >> 9) * pagesize * planew) + \n"
    "  ((x >> 9) * pagesize) + \n"
    "  (((y & 511) >> patternshift) * pagewh) + \n"
    "  ((x & 511) >> patternshift)) << patterndatasize ); \n"
    "  addr &= 0x7FFFFu;\n";

const char prg_rbg_get_pattern_data_1w[] =
    "//prg_rbg_get_pattern_data_1w\n"
    "  if( patternname == 0xFFFFFFFFu){\n"
    "    patternname = vram[addr>>2]; \n"
    "    if( (addr & 0x02u) != 0u ) { patternname >>= 16; } \n"
    "    patternname = (((patternname >> 8) & 0xFFu) | ((patternname) & 0xFFu) << 8);\n"
    "  }\n"
    "  if(colornumber==0) paladdr = ((patternname & 0xF000u) >> 12) | ((supplementdata & 0xE0u) >> 1); else paladdr = "
    "(patternname & 0x7000u) >> 8;\n"
    "  uint flipfunction;\n"
    "  switch (auxmode)\n"
    "  {\n"
    "  case 0: \n"
    "    flipfunction = (patternname & 0xC00u) >> 10;\n"
    "    switch (patternwh)\n"
    "    {\n"
    "    case 1:\n"
    "      charaddr = (patternname & 0x3FFu) | ((supplementdata & 0x1Fu) << 10);\n"
    "      break;\n"
    "    case 2:\n"
    "      charaddr = ((patternname & 0x3FFu) << 2) | (supplementdata & 0x3u) | ((supplementdata & 0x1Cu) << 10);\n"
    "      break;\n"
    "    }\n"
    "    break;\n"
    "  case 1:\n"
    "    flipfunction = 0u;\n"
    "    switch (patternwh)\n"
    "    {\n"
    "    case 1:\n"
    "      charaddr = (patternname & 0xFFFu) | ((supplementdata & 0x1Cu) << 10);\n"
    "      break;\n"
    "    case 2:\n"
    "      charaddr = ((patternname & 0xFFFu) << 2) | (supplementdata & 0x3u) | ((supplementdata & 0x10u) << 10);\n"
    "      break;\n"
    "    }\n"
    "    break;\n"
    "  }\n"
    "  charaddr &= 0x3FFFu;\n"
    "  charaddr *= 0x20u;\n";

const char prg_rbg_get_pattern_data_2w[] = "//prg_rbg_get_pattern_data_2w\n"
                                           "  patternname = vram[addr>>2]; \n"
                                           "  uint tmp1 = patternname & 0x7FFFu; \n"
                                           "  charaddr = patternname >> 16; \n"
                                           "  charaddr = (((charaddr >> 8) & 0xFFu) | ((charaddr) & 0xFFu) << 8);\n"
                                           "  tmp1 = (((tmp1 >> 8) & 0xFFu) | ((tmp1) & 0xFFu) << 8);\n"
                                           "  uint flipfunction = (tmp1 & 0xC000u) >> 14;\n"
                                           "  if(colornumber==0) paladdr = tmp1 & 0x7Fu; else paladdr = tmp1 & 0x70u;\n"
                                           "  uint specialfunction_in = (tmp1 & 0x2000u) >> 13;\n"
                                           "  uint specialcolorfunction_in = (tmp1 & 0x1000u) >> 12;\n"
                                           "  charaddr &= 0x3FFFu;\n"
                                           "  charaddr *= 0x20u;\n";

const char prg_rbg_get_charaddr[] = "//prg_rbg_get_charaddr\n"
                                    "  cellw = 8; \n"
                                    "  if (patternwh == 1) { \n"
                                    "    x &= 0x07;\n"
                                    "    y &= 0x07;\n"
                                    "    if ( (flipfunction & 0x2u) != 0u ) y = 7 - y;\n"
                                    "    if ( (flipfunction & 0x1u) != 0u ) x = 7 - x;\n"
                                    "  }else{\n"
                                    "    if (flipfunction != 0u) { \n"
                                    "      y &= 16 - 1;\n"
                                    "      if ( (flipfunction & 0x2u) != 0u ) {\n"
                                    "        if ( (y & 8) == 0 ) {\n"
                                    "          y = 8 - 1 - y + 16;\n"
                                    "        }else{ \n"
                                    "          y = 16 - 1 - y;\n"
                                    "        }\n"
                                    "      } else if ( (y & 8) != 0 ) { \n"
                                    "        y += 8; \n"
                                    "      }\n"
                                    "      if ((flipfunction & 0x1u) != 0u ) {\n"
                                    "        if ( (x & 8) == 0 ) y += 8;\n"
                                    "        x &= 8 - 1;\n"
                                    "        x = 8 - 1 - x;\n"
                                    "      } else if ( (x & 8) != 0 ) {\n"
                                    "        y += 8;\n"
                                    "        x &= 8 - 1;\n"
                                    "      } else {\n"
                                    "        x &= 8 - 1;\n"
                                    "      }\n"
                                    "    }else{\n"
                                    "      y &= 16 - 1;\n"
                                    "      if ( (y & 8) != 0 ) y += 8;\n"
                                    "      if ( (x & 8) != 0 ) y += 8;\n"
                                    "      x &= 8 - 1;\n"
                                    "    }\n"
                                    "  }\n";

const char prg_rbg_getcolor_4bpp[] = "//prg_rbg_getcolor_4bpp\n"
                                     "  uint dot = 0u;\n"
                                     "  uint cramindex = 0u;\n"
                                     "  uint dotaddr = ((charaddr + uint(((y * cellw) + x) >> 1)) & 0x7FFFFu);\n"
                                     "  dot = vram[ dotaddr >> 2];\n"
                                     "  if( (dotaddr & 0x3u) == 0u ) dot >>= 0;\n"
                                     "  else if( (dotaddr & 0x3u) == 1u ) dot >>= 8;\n"
                                     "  else if( (dotaddr & 0x3u) == 2u ) dot >>= 16;\n"
                                     "  else if( (dotaddr & 0x3u) == 3u ) dot >>= 24;\n"
                                     "  if ( (x & 0x1) == 0 ) dot >>= 4;\n"
                                     "  if ( (dot & 0xFu) == 0u && transparencyenable != 0 ) { \n"
                                     "    discarded = 1; \n"
                                     "  } else {\n"
                                     "    cramindex = (coloroffset + ((paladdr << 4) | (dot & 0xFu)));\n"
                                     "    priority_ = Vdp2SetSpecialPriority(dot);\n"
                                     "    cc = setCCOn(cramindex, dot);\n"
                                     "  }\n";

const char prg_rbg_getcolor_8bpp[] = "//prg_rbg_getcolor_8bpp\n"
                                     "  uint dot = 0u;\n"
                                     "  uint cramindex = 0u;\n"
                                     "  uint dotaddr = (charaddr + uint((y*cellw)+x))&0x7FFFFu;\n"
                                     "  dot = vram[ dotaddr >> 2];\n"
                                     "  if( (dotaddr & 0x3u) == 0u ) dot >>= 0;\n"
                                     "  else if( (dotaddr & 0x3u) == 1u ) dot >>= 8;\n"
                                     "  else if( (dotaddr & 0x3u) == 2u ) dot >>= 16;\n"
                                     "  else if( (dotaddr & 0x3u) == 3u ) dot >>= 24;\n"
                                     "  dot = dot & 0xFFu; \n"
                                     "  if ( dot == 0u && transparencyenable != 0 ) { \n"
                                     "    discarded = 1; \n"
                                     "  } else {\n"
                                     "    cramindex = (coloroffset + ((paladdr << 4) | dot));\n"
                                     "    priority_ = Vdp2SetSpecialPriority(dot);\n"
                                     "    cc = setCCOn(cramindex, dot);\n"
                                     "  }\n";

const char prg_rbg_getcolor_16bpp_palette[] = "//prg_rbg_getcolor_16bpp_palette\n"
                                              "  uint dot = 0u;\n"
                                              "  uint cramindex = 0u;\n"
                                              "  uint dotaddr = (charaddr + uint((y*cellw)+x) * 2u)&0x7FFFFu;\n"
                                              "  dot = vram[dotaddr>>2]; \n"
                                              "  if( (dotaddr & 0x02u) != 0u ) { dot >>= 16; } \n"
                                              "  dot = (((dot) >> 8 & 0xFF) | ((dot) & 0xFF) << 8);\n"
                                              "  if ( dot == 0 && transparencyenable != 0 ) { \n"
                                              "    discarded = 1; \n"
                                              "  } else {\n"
                                              "    cramindex = (coloroffset + dot);\n"
                                              "    priority_ = Vdp2SetSpecialPriority(dot);\n"
                                              "    cc = setCCOn(cramindex, dot);\n"
                                              "  }\n";

const char prg_rbg_getcolor_16bpp_rbg[] =
    "//prg_rbg_getcolor_16bpp_rbg\n"
    "  uint dot = 0u;\n"
    "  uint cramindex = 0u;\n"
    "  uint dotaddr = (charaddr + uint((y*cellw)+x) * 2u)&0x7FFFFu;\n"
    "  dot = vram[dotaddr>>2]; \n"
    "  if( (dotaddr & 0x02u) != 0u ) { dot >>= 16; } \n"
    "  dot = (((dot >> 8) & 0xFFu) | ((dot) & 0xFFu) << 8);\n"
    "  if ( (dot&0x8000u) == 0u && transparencyenable != 0 ) { \n"
    "    discarded = 1; \n"
    "  } else {\n"
    "    cramindex = (dot & 0x1Fu) << 3 | (dot & 0x3E0u) << 6 | (dot & 0x7C00u) << 9;\n"
    "    cc = setCCOn(cramindex, dot);\n"
    "  }\n";

const char prg_rbg_getcolor_32bpp_rbg[] =
    "//prg_rbg_getcolor_32bpp_rbg\n"
    "  uint dot = 0u;\n"
    "  uint cramindex = 0u;\n"
    "  uint dotaddr = (charaddr + uint((y*cellw)+x) * 4u)&0x7FFFFu;\n"
    "  dot = vram[dotaddr>>2]; \n"
    "  dot = ((dot&0xFF000000u) >> 24 | ((dot >> 8) & 0xFF00u) | ((dot) & 0xFF00u) << 8 | (dot&0x000000FFu) << 24);\n"
    "  if ( (dot&0x80000000u) == 0u && transparencyenable != 0 ) { \n"
    "    discarded = 1; \n"
    "  } else {\n"
    "    cc = setCCOn(cramindex, dot);\n"
    "    cramindex = dot & 0x00FFFFFFu;\n"
    "  }\n";

const char prg_generate_rbg_end[] =
    "  if ( para[paramid].linecoefenab != 0) imageStore(lnclSurface,texel,Vdp2ColorRamGetColorOffset(lineaddr));\n"
    "  else imageStore(lnclSurface,texel,vec4(0.0));\n"
    "  if (discarded != 0) imageStore(outSurface,texel,vec4(0.0));\n"
    "  else imageStore(outSurface,texel,vdp2color(alpha[int(original_pos.y)], priority_, cc, cramindex));\n"
    "}\n";

const GLchar *a_prg_rbg_0_2w_bitmap[] = {prg_generate_rbg,   prg_continue_rbg,      prg_rbg_rpmd0_2w,    prg_rbg_xy,
                                         prg_rbg_get_bitmap, prg_rbg_getcolor_8bpp, prg_generate_rbg_end};

const GLchar *a_prg_rbg_0_2w_p1_4bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd0_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_1w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_4bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_0_2w_p2_4bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd0_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_2w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_4bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_0_2w_p1_8bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd0_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_1w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_8bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_0_2w_p2_8bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd0_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_2w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_8bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_1_2w_bitmap[] = {prg_generate_rbg,   prg_continue_rbg,      prg_rbg_rpmd1_2w,    prg_rbg_xy,
                                         prg_rbg_get_bitmap, prg_rbg_getcolor_8bpp, prg_generate_rbg_end};

const GLchar *a_prg_rbg_1_2w_p1_4bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd1_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_1w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_4bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_1_2w_p2_4bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd1_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_2w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_4bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_1_2w_p1_8bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd1_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_1w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_8bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_1_2w_p2_8bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd1_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_2w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_8bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg1_1_2w_p1_4bpp[] = {prg_generate_rbg,
                                           prg_continue_rbg,
                                           prg_rbg_rpmd0_2w,
                                           prg_rbg_xy,
                                           prg_rbg_overmode_repeat,
                                           prg_rbg_get_patternaddr,
                                           prg_rbg_get_pattern_data_1w,
                                           prg_rbg_get_charaddr,
                                           prg_rbg_getcolor_4bpp,
                                           prg_generate_rbg_end};

const GLchar *a_prg_rbg1_1_2w_p2_4bpp[] = {prg_generate_rbg,
                                           prg_continue_rbg,
                                           prg_rbg_rpmd0_2w,
                                           prg_rbg_xy,
                                           prg_rbg_overmode_repeat,
                                           prg_rbg_get_patternaddr,
                                           prg_rbg_get_pattern_data_2w,
                                           prg_rbg_get_charaddr,
                                           prg_rbg_getcolor_4bpp,
                                           prg_generate_rbg_end};

const GLchar *a_prg_rbg1_1_2w_p1_8bpp[] = {prg_generate_rbg,
                                           prg_continue_rbg,
                                           prg_rbg_rpmd0_2w,
                                           prg_rbg_xy,
                                           prg_rbg_overmode_repeat,
                                           prg_rbg_get_patternaddr,
                                           prg_rbg_get_pattern_data_1w,
                                           prg_rbg_get_charaddr,
                                           prg_rbg_getcolor_8bpp,
                                           prg_generate_rbg_end};

const GLchar *a_prg_rbg1_1_2w_p2_8bpp[] = {prg_generate_rbg,
                                           prg_continue_rbg,
                                           prg_rbg_rpmd0_2w,
                                           prg_rbg_xy,
                                           prg_rbg_overmode_repeat,
                                           prg_rbg_get_patternaddr,
                                           prg_rbg_get_pattern_data_2w,
                                           prg_rbg_get_charaddr,
                                           prg_rbg_getcolor_8bpp,
                                           prg_generate_rbg_end};

const GLchar *a_prg_rbg_2_2w_bitmap[] = {prg_generate_rbg,   prg_continue_rbg,      prg_rbg_rpmd2_2w,    prg_rbg_xy,
                                         prg_rbg_get_bitmap, prg_rbg_getcolor_8bpp, prg_generate_rbg_end};

const GLchar *a_prg_rbg_2_2w_p1_4bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd2_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_1w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_4bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_2_2w_p2_4bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd2_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_2w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_4bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_2_2w_p1_8bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd2_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_1w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_8bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_2_2w_p2_8bpp[] = {prg_generate_rbg,
                                          prg_continue_rbg,
                                          prg_rbg_rpmd2_2w,
                                          prg_rbg_xy,
                                          prg_rbg_overmode_repeat,
                                          prg_rbg_get_patternaddr,
                                          prg_rbg_get_pattern_data_2w,
                                          prg_rbg_get_charaddr,
                                          prg_rbg_getcolor_8bpp,
                                          prg_generate_rbg_end};

const GLchar *a_prg_rbg_3_2w_bitmap[] = {prg_generate_rbg,   prg_continue_rbg,      prg_get_param_mode03, prg_rbg_xy,
                                         prg_rbg_get_bitmap, prg_rbg_getcolor_8bpp, prg_generate_rbg_end};

const GLchar *a_prg_rbg_3_2w_p1_4bpp[] = {
    prg_generate_rbg,        prg_continue_rbg,        prg_get_param_mode03,        prg_rbg_xy,
    prg_rbg_overmode_repeat, prg_rbg_get_patternaddr, prg_rbg_get_pattern_data_1w, prg_rbg_get_charaddr,
    prg_rbg_getcolor_4bpp,   prg_generate_rbg_end};

const GLchar *a_prg_rbg_3_2w_p2_4bpp[] = {
    prg_generate_rbg,        prg_continue_rbg,        prg_get_param_mode03,        prg_rbg_xy,
    prg_rbg_overmode_repeat, prg_rbg_get_patternaddr, prg_rbg_get_pattern_data_2w, prg_rbg_get_charaddr,
    prg_rbg_getcolor_4bpp,   prg_generate_rbg_end};

const GLchar *a_prg_rbg_3_2w_p1_8bpp[] = {
    prg_generate_rbg,        prg_continue_rbg,        prg_get_param_mode03,        prg_rbg_xy,
    prg_rbg_overmode_repeat, prg_rbg_get_patternaddr, prg_rbg_get_pattern_data_1w, prg_rbg_get_charaddr,
    prg_rbg_getcolor_8bpp,   prg_generate_rbg_end};

const GLchar *a_prg_rbg_3_2w_p2_8bpp[] = {
    prg_generate_rbg,        prg_continue_rbg,        prg_get_param_mode03,        prg_rbg_xy,
    prg_rbg_overmode_repeat, prg_rbg_get_patternaddr, prg_rbg_get_pattern_data_2w, prg_rbg_get_charaddr,
    prg_rbg_getcolor_8bpp,   prg_generate_rbg_end};

struct RBGUniform
{
    RBGUniform()
    {
        pagesize             = 0;
        patternshift         = 0;
        planew               = 0;
        pagewh               = 0;
        patterndatasize      = 0;
        supplementdata       = 0;
        auxmode              = 0;
        patternwh            = 0;
        coloroffset          = 0;
        transparencyenable   = 0;
        specialcolormode     = 0;
        specialcolorfunction = 0;
        specialcode          = 0;
        window_area_mode     = 0;
        priority             = 0;
        startLine            = 0;
        endLine              = 0;
    }
    float        hres_scale;
    float        vres_scale;
    int          cellw;
    int          cellh;
    int          paladdr_;
    int          pagesize;
    int          patternshift;
    int          planew;
    int          pagewh;
    int          patterndatasize;
    int          supplementdata;
    int          auxmode;
    int          patternwh;
    unsigned int coloroffset;
    int          transparencyenable;
    int          specialcolormode;
    int          specialcolorfunction;
    unsigned int specialcode;
    int          colornumber;
    int          window_area_mode;
    unsigned int priority;
    int          startLine;
    int          endLine;
    unsigned int specialprimode;
    unsigned int specialfunction;
    float        alpha_lncl;
    unsigned int lncl_table_addr;
    unsigned int cram_mode;
};

class RBGGenerator {
    GLuint prg_rbg_0_2w_bitmap_4bpp_    = 0;
    GLuint prg_rbg_0_2w_bitmap_8bpp_    = 0;
    GLuint prg_rbg_0_2w_bitmap_16bpp_p_ = 0;
    GLuint prg_rbg_0_2w_bitmap_16bpp_   = 0;
    GLuint prg_rbg_0_2w_bitmap_32bpp_   = 0;
    GLuint prg_rbg_0_2w_p1_4bpp_        = 0;
    GLuint prg_rbg_0_2w_p2_4bpp_        = 0;
    GLuint prg_rbg_0_2w_p1_8bpp_        = 0;
    GLuint prg_rbg_0_2w_p2_8bpp_        = 0;
    GLuint prg_rbg_0_2w_p1_16bpp_p_     = 0;
    GLuint prg_rbg_0_2w_p2_16bpp_p_     = 0;
    GLuint prg_rbg_0_2w_p1_16bpp_       = 0;
    GLuint prg_rbg_0_2w_p2_16bpp_       = 0;
    GLuint prg_rbg_0_2w_p1_32bpp_       = 0;
    GLuint prg_rbg_0_2w_p2_32bpp_       = 0;

    GLuint prg_rbg_1_2w_bitmap_4bpp_    = 0;
    GLuint prg_rbg_1_2w_bitmap_8bpp_    = 0;
    GLuint prg_rbg_1_2w_bitmap_16bpp_p_ = 0;
    GLuint prg_rbg_1_2w_bitmap_16bpp_   = 0;
    GLuint prg_rbg_1_2w_bitmap_32bpp_   = 0;
    GLuint prg_rbg_1_2w_p1_4bpp_        = 0;
    GLuint prg_rbg_1_2w_p2_4bpp_        = 0;
    GLuint prg_rbg_1_2w_p1_8bpp_        = 0;
    GLuint prg_rbg_1_2w_p2_8bpp_        = 0;
    GLuint prg_rbg1_1_2w_p1_4bpp_       = 0;
    GLuint prg_rbg1_1_2w_p2_4bpp_       = 0;
    GLuint prg_rbg1_1_2w_p1_8bpp_       = 0;
    GLuint prg_rbg1_1_2w_p2_8bpp_       = 0;
    GLuint prg_rbg_1_2w_p1_16bpp_p_     = 0;
    GLuint prg_rbg_1_2w_p2_16bpp_p_     = 0;
    GLuint prg_rbg_1_2w_p1_16bpp_       = 0;
    GLuint prg_rbg_1_2w_p2_16bpp_       = 0;
    GLuint prg_rbg_1_2w_p1_32bpp_       = 0;
    GLuint prg_rbg_1_2w_p2_32bpp_       = 0;

    GLuint prg_rbg_2_2w_bitmap_4bpp_    = 0;
    GLuint prg_rbg_2_2w_bitmap_8bpp_    = 0;
    GLuint prg_rbg_2_2w_bitmap_16bpp_p_ = 0;
    GLuint prg_rbg_2_2w_bitmap_16bpp_   = 0;
    GLuint prg_rbg_2_2w_bitmap_32bpp_   = 0;
    GLuint prg_rbg_2_2w_p1_4bpp_        = 0;
    GLuint prg_rbg_2_2w_p2_4bpp_        = 0;
    GLuint prg_rbg_2_2w_p1_8bpp_        = 0;
    GLuint prg_rbg_2_2w_p2_8bpp_        = 0;
    GLuint prg_rbg_2_2w_p1_16bpp_p_     = 0;
    GLuint prg_rbg_2_2w_p2_16bpp_p_     = 0;
    GLuint prg_rbg_2_2w_p1_16bpp_       = 0;
    GLuint prg_rbg_2_2w_p2_16bpp_       = 0;
    GLuint prg_rbg_2_2w_p1_32bpp_       = 0;
    GLuint prg_rbg_2_2w_p2_32bpp_       = 0;

    GLuint prg_rbg_3_2w_bitmap_4bpp_    = 0;
    GLuint prg_rbg_3_2w_bitmap_8bpp_    = 0;
    GLuint prg_rbg_3_2w_bitmap_16bpp_p_ = 0;
    GLuint prg_rbg_3_2w_bitmap_16bpp_   = 0;
    GLuint prg_rbg_3_2w_bitmap_32bpp_   = 0;
    GLuint prg_rbg_3_2w_p1_4bpp_        = 0;
    GLuint prg_rbg_3_2w_p2_4bpp_        = 0;
    GLuint prg_rbg_3_2w_p1_8bpp_        = 0;
    GLuint prg_rbg_3_2w_p2_8bpp_        = 0;
    GLuint prg_rbg_3_2w_p1_16bpp_p_     = 0;
    GLuint prg_rbg_3_2w_p2_16bpp_p_     = 0;
    GLuint prg_rbg_3_2w_p1_16bpp_       = 0;
    GLuint prg_rbg_3_2w_p2_16bpp_       = 0;
    GLuint prg_rbg_3_2w_p1_32bpp_       = 0;
    GLuint prg_rbg_3_2w_p2_32bpp_       = 0;

    GLuint               tex_surface_  = 0;
    GLuint               tex_surface_1 = 0;
    GLuint               tex_lncl_[2]  = {0};
    GLuint               ssbo_vram_    = 0;
    GLuint               ssbo_cram_    = 0;
    GLuint               ssbo_rotwin_  = 0;
    GLuint               ssbo_window_  = 0;
    GLuint               ssbo_paraA_   = 0;
    GLuint               ssbo_alpha_   = 0;
    int                  tex_width_    = 0;
    int                  tex_height_   = 0;
    static RBGGenerator *instance_;
    GLuint               scene_uniform = 0;
    RBGUniform           uniform;
    int                  struct_size_;

    void *mapped_vram = nullptr;

  protected:
    RBGGenerator()
    {
        tex_surface_ = 0;
        tex_width_   = 0;
        tex_height_  = 0;
        struct_size_ = sizeof(vdp2rotationparameter_struct);
        int am       = sizeof(vdp2rotationparameter_struct) % 16;
        if (am != 0) {
            struct_size_ += 16 - am;
        }
    }

  public:
    static RBGGenerator *getInstance()
    {
        if (instance_ == nullptr) {
            instance_ = new RBGGenerator();
        }
        return instance_;
    }

    void resize(int width, int height)
    {
        if (tex_width_ == width && tex_height_ == height)
            return;

        DEBUGWIP("resize %d, %d\n", width, height);

        glGetError();

        if (tex_surface_ != 0) {
            glDeleteTextures(1, &tex_surface_);
            tex_surface_ = 0;
        }

        glGenTextures(1, &tex_surface_);
        ErrorHandle("glGenTextures");

        tex_width_  = width;
        tex_height_ = height;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_surface_);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        ErrorHandle("glBindTexture");
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_width_, tex_height_);
        ErrorHandle("glTexStorage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ErrorHandle("glTexParameteri");

        if (tex_surface_1 != 0) {
            glDeleteTextures(1, &tex_surface_1);
        }
        glGenTextures(1, &tex_surface_1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_surface_1);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        ErrorHandle("glBindTexture");
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_width_, tex_height_);
        ErrorHandle("glTexStorage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ErrorHandle("glTexParameteri");

        if (tex_lncl_[0] != 0) {
            glDeleteTextures(2, &tex_lncl_[0]);
        }

        glGenTextures(2, &tex_lncl_[0]);
        ErrorHandle("glGenTextures");

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_lncl_[0]);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        ErrorHandle("glBindTexture");
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_width_, tex_height_);
        ErrorHandle("glTexStorage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ErrorHandle("glTexParameteri");

        glBindTexture(GL_TEXTURE_2D, tex_lncl_[1]);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        ErrorHandle("glBindTexture");
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_width_, tex_height_);
        ErrorHandle("glTexStorage2D");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ErrorHandle("glTexParameteri");

        YGLDEBUG("resize tex_surface_=%d, tex_surface_1=%d\n", tex_surface_, tex_surface_1);
    }

    GLuint createProgram(int count, const GLchar **prg_strs)
    {
        GLuint result = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(result, count, prg_strs, NULL);
        glCompileShader(result);
        GLint status;
        glGetShaderiv(result, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint length;
            glGetShaderiv(result, GL_INFO_LOG_LENGTH, &length);
            GLchar *info = new GLchar[length];
            glGetShaderInfoLog(result, length, NULL, info);
            YGLDEBUG("[COMPILE] %s\n", info);
            FILE *fp = fopen("tmp.cpp", "w");
            if (fp) {
                for (int i = 0; i < count; i++) {
                    fprintf(fp, "%s", prg_strs[i]);
                }
                fclose(fp);
            }
            abort();
            delete[] info;
        }
        GLuint program = glCreateProgram();
        glAttachShader(program, result);
        glLinkProgram(program);
        glDetachShader(program, result);
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            GLchar *info = new GLchar[length];
            glGetProgramInfoLog(program, length, NULL, info);
            YGLDEBUG("[LINK] %s\n", info);
            FILE *fp = fopen("tmp.cpp", "w");
            if (fp) {
                for (int i = 0; i < count; i++) {
                    fprintf(fp, "%s", prg_strs[i]);
                }
                fflush(fp);
                fclose(fp);
            }
            abort();
            delete[] info;
        }
        return program;
    }

    void init(int width, int height)
    {

        resize(width, height);
        if (ssbo_vram_ != 0)
            return;

        DEBUGWIP("Init\n");

        glGenBuffers(1, &ssbo_vram_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0x100000, (void *)Vdp2Ram, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &ssbo_paraA_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_paraA_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size_ * 2, NULL, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &scene_uniform);
        glBindBuffer(GL_UNIFORM_BUFFER, scene_uniform);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(RBGUniform), &uniform, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        prg_rbg_0_2w_bitmap_8bpp_ =
            createProgram(sizeof(a_prg_rbg_0_2w_bitmap) / sizeof(char *), (const GLchar **)a_prg_rbg_0_2w_bitmap);
        prg_rbg_0_2w_p1_4bpp_ =
            createProgram(sizeof(a_prg_rbg_0_2w_p1_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_0_2w_p1_4bpp);
        prg_rbg_0_2w_p2_4bpp_ =
            createProgram(sizeof(a_prg_rbg_0_2w_p2_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_0_2w_p2_4bpp);
        prg_rbg_0_2w_p1_8bpp_ =
            createProgram(sizeof(a_prg_rbg_0_2w_p1_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_0_2w_p1_8bpp);
        prg_rbg_0_2w_p2_8bpp_ =
            createProgram(sizeof(a_prg_rbg_0_2w_p2_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_0_2w_p2_8bpp);

        prg_rbg_1_2w_bitmap_8bpp_ =
            createProgram(sizeof(a_prg_rbg_1_2w_bitmap) / sizeof(char *), (const GLchar **)a_prg_rbg_1_2w_bitmap);
        prg_rbg_1_2w_p1_4bpp_ =
            createProgram(sizeof(a_prg_rbg_1_2w_p1_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_1_2w_p1_4bpp);
        prg_rbg_1_2w_p2_4bpp_ =
            createProgram(sizeof(a_prg_rbg_1_2w_p2_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_1_2w_p2_4bpp);
        prg_rbg_1_2w_p1_8bpp_ =
            createProgram(sizeof(a_prg_rbg_1_2w_p1_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_1_2w_p1_8bpp);
        prg_rbg_1_2w_p2_8bpp_ =
            createProgram(sizeof(a_prg_rbg_1_2w_p2_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_1_2w_p2_8bpp);

        prg_rbg1_1_2w_p1_4bpp_ =
            createProgram(sizeof(a_prg_rbg1_1_2w_p1_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg1_1_2w_p1_4bpp);
        prg_rbg1_1_2w_p2_4bpp_ =
            createProgram(sizeof(a_prg_rbg1_1_2w_p2_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg1_1_2w_p2_4bpp);
        prg_rbg1_1_2w_p1_8bpp_ =
            createProgram(sizeof(a_prg_rbg1_1_2w_p1_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg1_1_2w_p1_8bpp);
        prg_rbg1_1_2w_p2_8bpp_ =
            createProgram(sizeof(a_prg_rbg1_1_2w_p2_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg1_1_2w_p2_8bpp);

        prg_rbg_2_2w_bitmap_8bpp_ =
            createProgram(sizeof(a_prg_rbg_2_2w_bitmap) / sizeof(char *), (const GLchar **)a_prg_rbg_2_2w_bitmap);
        prg_rbg_2_2w_p1_4bpp_ =
            createProgram(sizeof(a_prg_rbg_2_2w_p1_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_2_2w_p1_4bpp);
        prg_rbg_2_2w_p2_4bpp_ =
            createProgram(sizeof(a_prg_rbg_2_2w_p2_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_2_2w_p2_4bpp);
        prg_rbg_2_2w_p1_8bpp_ =
            createProgram(sizeof(a_prg_rbg_2_2w_p1_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_2_2w_p1_8bpp);
        prg_rbg_2_2w_p2_8bpp_ =
            createProgram(sizeof(a_prg_rbg_2_2w_p2_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_2_2w_p2_8bpp);

        prg_rbg_3_2w_bitmap_8bpp_ =
            createProgram(sizeof(a_prg_rbg_3_2w_bitmap) / sizeof(char *), (const GLchar **)a_prg_rbg_3_2w_bitmap);
        prg_rbg_3_2w_p1_4bpp_ =
            createProgram(sizeof(a_prg_rbg_3_2w_p1_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_3_2w_p1_4bpp);
        prg_rbg_3_2w_p2_4bpp_ =
            createProgram(sizeof(a_prg_rbg_3_2w_p2_4bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_3_2w_p2_4bpp);
        prg_rbg_3_2w_p1_8bpp_ =
            createProgram(sizeof(a_prg_rbg_3_2w_p1_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_3_2w_p1_8bpp);
        prg_rbg_3_2w_p2_8bpp_ =
            createProgram(sizeof(a_prg_rbg_3_2w_p2_8bpp) / sizeof(char *), (const GLchar **)a_prg_rbg_3_2w_p2_8bpp);
    }

    bool ErrorHandle(const char *name)
    {
        GLenum error_code = glGetError();
        if (error_code == GL_NO_ERROR) {
            return true;
        }
        do {
            const char *msg = "";
            switch (error_code) {
                case GL_INVALID_ENUM:
                    msg = "INVALID_ENUM";
                    break;
                case GL_INVALID_VALUE:
                    msg = "INVALID_VALUE";
                    break;
                case GL_INVALID_OPERATION:
                    msg = "INVALID_OPERATION";
                    break;
                case GL_OUT_OF_MEMORY:
                    msg = "OUT_OF_MEMORY";
                    break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    msg = "INVALID_FRAMEBUFFER_OPERATION";
                    break;
                default:
                    msg = "Unknown";
                    break;
            }
            YGLDEBUG("GLErrorLayer:ERROR:%04x'%s' %s\n", error_code, msg, name);
            abort();
            error_code = glGetError();
        } while (error_code != GL_NO_ERROR);
        return false;
    }

    template <typename T> T Add(T a, T b)
    {
        return a + b;
    }

#define COMPILE_COLOR_DOT(BASE, COLOR, DOT)
#define S(A) A, sizeof(A) / sizeof(char *)

    GLuint compile_color_dot(const char *base[], int size, const char *color, const char *dot)
    {
        base[size - 2] = color;
        base[size - 1] = dot;
        return createProgram(size, (const GLchar **)base);
    }

    void updateRBG0(RBGDrawInfo *rbg, Vdp2 *varVdp2Regs)
    {
        if (varVdp2Regs->RPMD == 0 || (varVdp2Regs->RPMD == 3 && (varVdp2Regs->WCTLD & 0xA) == 0)) {
            if (rbg->info.isbitmap) {
                switch (rbg->info.colornumber) {
                    case 0: {
                        if (prg_rbg_0_2w_bitmap_4bpp_ == 0) {
                            prg_rbg_0_2w_bitmap_4bpp_ = compile_color_dot(S(a_prg_rbg_0_2w_bitmap),
                                                                          prg_rbg_getcolor_4bpp, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_4bpp_);
                        break;
                    }
                    case 1: {
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_8bpp_);
                        break;
                    }
                    case 2: {
                        if (prg_rbg_0_2w_bitmap_16bpp_p_ == 0) {
                            prg_rbg_0_2w_bitmap_16bpp_p_ = compile_color_dot(
                                S(a_prg_rbg_0_2w_bitmap), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_16bpp_p_);
                        break;
                    }
                    case 3: {
                        if (prg_rbg_0_2w_bitmap_16bpp_ == 0) {
                            prg_rbg_0_2w_bitmap_16bpp_ = compile_color_dot(
                                S(a_prg_rbg_0_2w_bitmap), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_16bpp_);
                        break;
                    }
                    case 4: {
                        if (prg_rbg_0_2w_bitmap_32bpp_ == 0) {
                            prg_rbg_0_2w_bitmap_32bpp_ = compile_color_dot(
                                S(a_prg_rbg_0_2w_bitmap), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_32bpp_);
                        break;
                    }
                }
            } else {
                if (rbg->info.patterndatasize == 1) {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_0_2w_p1_16bpp_p_ == 0) {
                                prg_rbg_0_2w_p1_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_0_2w_p1_16bpp_ == 0) {
                                prg_rbg_0_2w_p1_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_0_2w_p1_32bpp_ == 0) {
                                prg_rbg_0_2w_p1_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_32bpp_);
                            break;
                        }
                    }
                } else {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_0_2w_p2_16bpp_p_ == 0) {
                                prg_rbg_0_2w_p2_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p2_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_0_2w_p2_16bpp_ == 0) {
                                prg_rbg_0_2w_p2_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p2_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_0_2w_p2_32bpp_ == 0) {
                                prg_rbg_0_2w_p2_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p2_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_32bpp_);
                            break;
                        }
                    }
                }
            }
        } else if (varVdp2Regs->RPMD == 1) {
            if (rbg->info.isbitmap) {
                switch (rbg->info.colornumber) {
                    case 0: {
                        if (prg_rbg_1_2w_bitmap_4bpp_ == 0) {
                            prg_rbg_1_2w_bitmap_4bpp_ = compile_color_dot(S(a_prg_rbg_1_2w_bitmap),
                                                                          prg_rbg_getcolor_4bpp, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_4bpp_);
                        break;
                    }
                    case 1: {
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_8bpp_);
                        break;
                    }
                    case 2: {
                        if (prg_rbg_1_2w_bitmap_16bpp_p_ == 0) {
                            prg_rbg_1_2w_bitmap_16bpp_p_ = compile_color_dot(
                                S(a_prg_rbg_1_2w_bitmap), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_16bpp_p_);
                        break;
                    }
                    case 3: {
                        if (prg_rbg_1_2w_bitmap_16bpp_ == 0) {
                            prg_rbg_1_2w_bitmap_16bpp_ = compile_color_dot(
                                S(a_prg_rbg_1_2w_bitmap), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_16bpp_);
                        break;
                    }
                    case 4: {
                        if (prg_rbg_1_2w_bitmap_32bpp_ == 0) {
                            prg_rbg_1_2w_bitmap_32bpp_ = compile_color_dot(
                                S(a_prg_rbg_1_2w_bitmap), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_32bpp_);
                        break;
                    }
                }
            } else {
                if (rbg->info.patterndatasize == 1) {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_1_2w_p1_16bpp_p_ == 0) {
                                prg_rbg_1_2w_p1_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p1_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_1_2w_p1_16bpp_ == 0) {
                                prg_rbg_1_2w_p1_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p1_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_1_2w_p1_32bpp_ == 0) {
                                prg_rbg_1_2w_p1_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_32bpp_);
                            break;
                        }
                    }
                } else {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_1_2w_p2_16bpp_p_ == 0) {
                                prg_rbg_1_2w_p2_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p2_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_1_2w_p2_16bpp_ == 0) {
                                prg_rbg_1_2w_p2_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p2_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_1_2w_p2_32bpp_ == 0) {
                                prg_rbg_1_2w_p2_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p2_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_32bpp_);
                            break;
                        }
                    }
                }
            }
        } else if (varVdp2Regs->RPMD == 2) {
            if (rbg->info.isbitmap) {
                switch (rbg->info.colornumber) {
                    case 0: {
                        if (prg_rbg_2_2w_bitmap_4bpp_ == 0) {
                            prg_rbg_2_2w_bitmap_4bpp_ = compile_color_dot(S(a_prg_rbg_2_2w_bitmap),
                                                                          prg_rbg_getcolor_4bpp, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_2_2w_bitmap_4bpp_);
                        break;
                    }
                    case 1: {
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_2_2w_bitmap_8bpp_);
                        break;
                    }
                    case 2: {
                        if (prg_rbg_2_2w_bitmap_16bpp_p_ == 0) {
                            prg_rbg_2_2w_bitmap_16bpp_p_ = compile_color_dot(
                                S(a_prg_rbg_2_2w_bitmap), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_2_2w_bitmap_16bpp_p_);
                        break;
                    }
                    case 3: {
                        if (prg_rbg_2_2w_bitmap_16bpp_ == 0) {
                            prg_rbg_2_2w_bitmap_16bpp_ = compile_color_dot(
                                S(a_prg_rbg_2_2w_bitmap), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_2_2w_bitmap_16bpp_);
                        break;
                    }
                    case 4: {
                        if (prg_rbg_2_2w_bitmap_32bpp_ == 0) {
                            prg_rbg_2_2w_bitmap_32bpp_ = compile_color_dot(
                                S(a_prg_rbg_2_2w_bitmap), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_2_2w_bitmap_32bpp_);
                        break;
                    }
                }

            } else {
                if (rbg->info.patterndatasize == 1) {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p1_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p1_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_2_2w_p1_16bpp_p_ == 0) {
                                prg_rbg_2_2w_p1_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_2_2w_p1_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p1_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_2_2w_p1_16bpp_ == 0) {
                                prg_rbg_2_2w_p1_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_2_2w_p1_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p1_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_2_2w_p1_32bpp_ == 0) {
                                prg_rbg_2_2w_p1_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_2_2w_p1_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p1_32bpp_);
                            break;
                        }
                    }
                } else {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p2_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p2_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_2_2w_p2_16bpp_p_ == 0) {
                                prg_rbg_2_2w_p2_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_2_2w_p2_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p2_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_2_2w_p2_16bpp_ == 0) {
                                prg_rbg_2_2w_p2_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_2_2w_p2_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p2_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_2_2w_p2_32bpp_ == 0) {
                                prg_rbg_2_2w_p2_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_2_2w_p2_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_2_2w_p2_32bpp_);
                            break;
                        }
                    }
                }
            }
        } else if (varVdp2Regs->RPMD == 3) {
            if (rbg->info.isbitmap) {
                switch (rbg->info.colornumber) {
                    case 0: {
                        if (prg_rbg_3_2w_bitmap_4bpp_ == 0) {
                            prg_rbg_3_2w_bitmap_4bpp_ = compile_color_dot(S(a_prg_rbg_3_2w_bitmap),
                                                                          prg_rbg_getcolor_4bpp, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_3_2w_bitmap_4bpp_);
                        break;
                    }
                    case 1: {
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_3_2w_bitmap_8bpp_);
                        break;
                    }
                    case 2: {
                        if (prg_rbg_3_2w_bitmap_16bpp_p_ == 0) {
                            prg_rbg_3_2w_bitmap_16bpp_p_ = compile_color_dot(
                                S(a_prg_rbg_3_2w_bitmap), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_3_2w_bitmap_16bpp_p_);
                        break;
                    }
                    case 3: {
                        if (prg_rbg_3_2w_bitmap_16bpp_ == 0) {
                            prg_rbg_3_2w_bitmap_16bpp_ = compile_color_dot(
                                S(a_prg_rbg_3_2w_bitmap), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_3_2w_bitmap_16bpp_);
                        break;
                    }
                    case 4: {
                        if (prg_rbg_3_2w_bitmap_32bpp_ == 0) {
                            prg_rbg_3_2w_bitmap_32bpp_ = compile_color_dot(
                                S(a_prg_rbg_3_2w_bitmap), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_3_2w_bitmap_32bpp_);
                        break;
                    }
                }

            } else {
                if (rbg->info.patterndatasize == 1) {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p1_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p1_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_3_2w_p1_16bpp_p_ == 0) {
                                prg_rbg_3_2w_p1_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_3_2w_p1_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p1_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_3_2w_p1_16bpp_ == 0) {
                                prg_rbg_3_2w_p1_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_3_2w_p1_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p1_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_3_2w_p1_32bpp_ == 0) {
                                prg_rbg_3_2w_p1_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_3_2w_p1_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p1_32bpp_);
                            break;
                        }
                    }
                } else {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p2_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p2_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_3_2w_p2_16bpp_p_ == 0) {
                                prg_rbg_3_2w_p2_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_3_2w_p2_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p2_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_3_2w_p2_16bpp_ == 0) {
                                prg_rbg_3_2w_p2_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_3_2w_p2_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p2_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_3_2w_p2_32bpp_ == 0) {
                                prg_rbg_3_2w_p2_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_3_2w_p2_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_3_2w_p2_32bpp_);
                            break;
                        }
                    }
                }
            }
        }
    }

    void updateRBG1(RBGDrawInfo *rbg, Vdp2 *varVdp2Regs)
    {
        if (varVdp2Regs->RPMD == 0 || (varVdp2Regs->RPMD == 3 && (varVdp2Regs->WCTLD & 0xA) == 0)) {
            if (rbg->info.isbitmap) {
                switch (rbg->info.colornumber) {
                    case 0: {
                        if (prg_rbg_0_2w_bitmap_4bpp_ == 0) {
                            prg_rbg_0_2w_bitmap_4bpp_ = compile_color_dot(S(a_prg_rbg_0_2w_bitmap),
                                                                          prg_rbg_getcolor_4bpp, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_4bpp_);
                        break;
                    }
                    case 1: {
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_8bpp_);
                        break;
                    }
                    case 2: {
                        if (prg_rbg_0_2w_bitmap_16bpp_p_ == 0) {
                            prg_rbg_0_2w_bitmap_16bpp_p_ = compile_color_dot(
                                S(a_prg_rbg_0_2w_bitmap), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_16bpp_p_);
                        break;
                    }
                    case 3: {
                        if (prg_rbg_0_2w_bitmap_16bpp_ == 0) {
                            prg_rbg_0_2w_bitmap_16bpp_ = compile_color_dot(
                                S(a_prg_rbg_0_2w_bitmap), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_16bpp_);
                        break;
                    }
                    case 4: {
                        if (prg_rbg_0_2w_bitmap_32bpp_ == 0) {
                            prg_rbg_0_2w_bitmap_32bpp_ = compile_color_dot(
                                S(a_prg_rbg_0_2w_bitmap), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_0_2w_bitmap_32bpp_);
                        break;
                    }
                }
            } else {
                if (rbg->info.patterndatasize == 1) {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_0_2w_p1_16bpp_p_ == 0) {
                                prg_rbg_0_2w_p1_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_0_2w_p1_16bpp_ == 0) {
                                prg_rbg_0_2w_p1_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_0_2w_p1_32bpp_ == 0) {
                                prg_rbg_0_2w_p1_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p1_32bpp_);
                            break;
                        }
                    }
                } else {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_0_2w_p2_16bpp_p_ == 0) {
                                prg_rbg_0_2w_p2_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p2_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_0_2w_p2_16bpp_ == 0) {
                                prg_rbg_0_2w_p2_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p2_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_0_2w_p2_32bpp_ == 0) {
                                prg_rbg_0_2w_p2_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p2_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_0_2w_p2_32bpp_);
                            break;
                        }
                    }
                }
            }
        } else {
            if (rbg->info.isbitmap) {
                switch (rbg->info.colornumber) {
                    case 0: {
                        if (prg_rbg_1_2w_bitmap_4bpp_ == 0) {
                            prg_rbg_1_2w_bitmap_4bpp_ = compile_color_dot(S(a_prg_rbg_1_2w_bitmap),
                                                                          prg_rbg_getcolor_4bpp, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_4bpp_);
                        break;
                    }
                    case 1: {
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_8bpp_);
                        break;
                    }
                    case 2: {
                        if (prg_rbg_1_2w_bitmap_16bpp_p_ == 0) {
                            prg_rbg_1_2w_bitmap_16bpp_p_ = compile_color_dot(
                                S(a_prg_rbg_1_2w_bitmap), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_16bpp_p_);
                        break;
                    }
                    case 3: {
                        if (prg_rbg_1_2w_bitmap_16bpp_ == 0) {
                            prg_rbg_1_2w_bitmap_16bpp_ = compile_color_dot(
                                S(a_prg_rbg_1_2w_bitmap), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_16bpp_);
                        break;
                    }
                    case 4: {
                        if (prg_rbg_1_2w_bitmap_32bpp_ == 0) {
                            prg_rbg_1_2w_bitmap_32bpp_ = compile_color_dot(
                                S(a_prg_rbg_1_2w_bitmap), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                        }
                        DEBUGWIP("prog %d\n", __LINE__);
                        glUseProgram(prg_rbg_1_2w_bitmap_32bpp_);
                        break;
                    }
                }
            } else {
                if (rbg->info.patterndatasize == 1) {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg1_1_2w_p1_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg1_1_2w_p1_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_1_2w_p1_16bpp_p_ == 0) {
                                prg_rbg_1_2w_p1_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg1_1_2w_p1_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_1_2w_p1_16bpp_ == 0) {
                                prg_rbg_1_2w_p1_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg1_1_2w_p1_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_1_2w_p1_32bpp_ == 0) {
                                prg_rbg_1_2w_p1_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_0_2w_p1_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p1_32bpp_);
                            break;
                        }
                    }
                } else {
                    switch (rbg->info.colornumber) {
                        case 0: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_4bpp_);
                            break;
                        }
                        case 1: {
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_8bpp_);
                            break;
                        }
                        case 2: {
                            if (prg_rbg_1_2w_p2_16bpp_p_ == 0) {
                                prg_rbg_1_2w_p2_16bpp_p_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p2_4bpp), prg_rbg_getcolor_16bpp_palette, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_16bpp_p_);
                            break;
                        }
                        case 3: {
                            if (prg_rbg_1_2w_p2_16bpp_ == 0) {
                                prg_rbg_1_2w_p2_16bpp_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p2_4bpp), prg_rbg_getcolor_16bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_16bpp_);
                            break;
                        }
                        case 4: {
                            if (prg_rbg_1_2w_p2_32bpp_ == 0) {
                                prg_rbg_1_2w_p2_32bpp_ = compile_color_dot(
                                    S(a_prg_rbg_1_2w_p2_4bpp), prg_rbg_getcolor_32bpp_rbg, prg_generate_rbg_end);
                            }
                            DEBUGWIP("prog %d\n", __LINE__);
                            glUseProgram(prg_rbg_1_2w_p2_32bpp_);
                            break;
                        }
                    }
                }
            }
        }
    }

    void update(RBGDrawInfo *rbg, Vdp2 *varVdp2Regs)
    {
        if (prg_rbg_0_2w_p1_4bpp_ == 0)
            return;

        GLuint error;
        int    local_size_x = 4;
        int    local_size_y = 4;

        int work_groups_x = ceil(float(tex_width_) / float(local_size_x));
        int work_groups_y = ceil(float(tex_height_) / float(local_size_y));

        u8 VRAMNeedAnUpdate = Vdp2RamIsUpdated();

        error = glGetError();

        if (rbg->info.idScreen == RBG0)
            updateRBG0(rbg, varVdp2Regs);
        else
            updateRBG1(rbg, varVdp2Regs);

        ErrorHandle("glUseProgram");

        if (VRAMNeedAnUpdate != 0) {
            LOG("VRAM Update %x\n", VRAMNeedAnUpdate);
            u32 start = 0;
            u32 size  = 0;
            switch (VRAMNeedAnUpdate) {
                case 0b0001:
                    size = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b0010:
                    start = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    size  = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b0011:
                    size = 0x40000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b0100:
                    start = 0x40000 << (Vdp2Regs->VRSIZE >> 15);
                    size  = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b0101:
                    size = 0x60000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b0110:
                    start = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    size  = 0x40000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b0111:
                    size = 0x60000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1000:
                    start = 0x60000 << (Vdp2Regs->VRSIZE >> 15);
                    size  = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1001:
                    size = 0x80000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1010:
                    start = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    size  = 0x60000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1011:
                    size = 0x80000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1100:
                    start = 0x40000 << (Vdp2Regs->VRSIZE >> 15);
                    size  = 0x40000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1101:
                    size = 0x80000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1110:
                    start = 0x20000 << (Vdp2Regs->VRSIZE >> 15);
                    size  = 0x60000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                case 0b1111:
                    size = 0x80000 << (Vdp2Regs->VRSIZE >> 15);
                    break;
                default:
                    break;
            }

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
            if (mapped_vram == nullptr)
                mapped_vram =
                    glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 0x100000,
                                     GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
            memcpy(&((u8 *)mapped_vram)[0], &Vdp2Ram[0], 0x80000 << (Vdp2Regs->VRSIZE >> 15));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
            mapped_vram = nullptr;
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vram_);
        ErrorHandle("glBindBufferBase");

        if (rbg->info.specialcolormode == 3 || rbg->paraA.k_mem_type != 0 || rbg->paraB.k_mem_type != 0 ||
            (_Ygl->linecolorcoef_tex[0] != 0) || (_Ygl->linecolorcoef_tex[1] != 0)) {
            if (ssbo_cram_ == 0) {
                glGenBuffers(1, &ssbo_cram_);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cram_);
                glBufferData(GL_SHADER_STORAGE_BUFFER, 0x1000, NULL, GL_DYNAMIC_DRAW);
            }
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cram_);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 0x1000, (void *)Vdp2ColorRam);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_cram_);
        }

        if (ssbo_rotwin_ == 0) {
            glGenBuffers(1, &ssbo_rotwin_);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_rotwin_);
            glBufferData(GL_SHADER_STORAGE_BUFFER, 0x800, NULL, GL_DYNAMIC_DRAW);
        }
        if (rbg->info.RotWin != NULL) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_rotwin_);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 0x800, (void *)rbg->info.RotWin);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_rotwin_);
        }

        if (ssbo_alpha_ == 0) {
            glGenBuffers(1, &ssbo_alpha_);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_alpha_);
            glBufferData(GL_SHADER_STORAGE_BUFFER, 270 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_alpha_);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 270 * sizeof(int), (void *)&rbg->alpha[0]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_alpha_);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_paraA_);
        if (rbg->info.idScreen == RBG0) {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2rotationparameter_struct), (void *)&rbg->paraA);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, struct_size_, sizeof(vdp2rotationparameter_struct),
                            (void *)&rbg->paraB);
        } else {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vdp2rotationparameter_struct), (void *)&rbg->paraB);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, struct_size_, sizeof(vdp2rotationparameter_struct),
                            (void *)&rbg->paraA);
        }
        ErrorHandle("glBufferSubData");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_paraA_);

        uniform.vres_scale           = (float)_Ygl->heightRatio;
        uniform.hres_scale           = (float)_Ygl->widthRatio;
        uniform.cellw                = rbg->info.cellw;
        uniform.cellh                = rbg->info.cellh;
        uniform.paladdr_             = rbg->info.paladdr;
        uniform.pagesize             = rbg->pagesize;
        uniform.patternshift         = rbg->patternshift;
        uniform.planew               = rbg->info.planew;
        uniform.pagewh               = rbg->info.pagewh;
        uniform.patterndatasize      = rbg->info.patterndatasize;
        uniform.supplementdata       = rbg->info.supplementdata;
        uniform.auxmode              = rbg->info.auxmode;
        uniform.patternwh            = rbg->info.patternwh;
        uniform.coloroffset          = rbg->info.coloroffset;
        uniform.transparencyenable   = rbg->info.transparencyenable;
        uniform.specialcolormode     = rbg->info.specialcolormode;
        uniform.specialcolorfunction = rbg->info.specialcolorfunction;
        uniform.specialcode          = rbg->info.specialcode;
        uniform.colornumber          = rbg->info.colornumber;
        uniform.window_area_mode     = rbg->info.RotWinMode;
        uniform.priority             = rbg->info.priority;
        uniform.startLine            = rbg->info.startLine;
        uniform.endLine              = rbg->info.endLine;
        uniform.specialprimode       = rbg->info.specialprimode;
        uniform.specialfunction      = rbg->info.specialfunction;
        uniform.alpha_lncl           = ((~(varVdp2Regs->CCRLB & 0x1F) << 3) | NONE) / 255.0f;
        uniform.lncl_table_addr      = Vdp2RamReadWord(NULL, Vdp2Ram, (varVdp2Regs->LCTA.all & 0x7FFFF) << 1);
        uniform.cram_mode            = Vdp2Internal.ColorMode;

        glBindBuffer(GL_UNIFORM_BUFFER, scene_uniform);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RBGUniform), (void *)&uniform);
        ErrorHandle("glBufferSubData");
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, scene_uniform);

        if (rbg->rgb_type == 0x04) {
            DEBUGWIP("Draw RBG1 [%d -> %d]\n", uniform.startLine, uniform.endLine);
            glBindImageTexture(0, tex_surface_1, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            glBindImageTexture(7, tex_lncl_[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            ErrorHandle("glBindImageTexture 1");
        } else {
            DEBUGWIP("Draw RBG0 [%d -> %d]\n", uniform.startLine, uniform.endLine);
            glBindImageTexture(0, tex_surface_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            glBindImageTexture(7, tex_lncl_[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
            ErrorHandle("glBindImageTexture 0");
        }

        glDispatchCompute(work_groups_x, work_groups_y, 1);
        ErrorHandle("glDispatchCompute");

        glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
        glBindImageTexture(7, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    GLuint getTexture(int id)
    {
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        if (id == 1) {
            return tex_surface_;
        }
        return tex_surface_1;
    }

    GLuint getLnclTexture(int id)
    {
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        if (id == 1) {
            return tex_lncl_[1];
        }
        return tex_lncl_[0];
    }

    void onFinish()
    {
        if (ssbo_vram_ != 0 && mapped_vram == nullptr) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vram_);
            mapped_vram = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 0x100000,
                                           GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }
};

RBGGenerator *RBGGenerator::instance_ = nullptr;

extern "C" {
void RBGGenerator_init(int width, int height)
{
    if (_Ygl->rbg_use_compute_shader == 0)
        return;

    RBGGenerator *instance = RBGGenerator::getInstance();
    instance->init(width, height);
}
void RBGGenerator_resize(int width, int height)
{
    if (_Ygl->rbg_use_compute_shader == 0)
        return;
    YGLDEBUG("RBGGenerator_resize\n");
    RBGGenerator *instance = RBGGenerator::getInstance();
    instance->resize(width, height);
}
void RBGGenerator_update(RBGDrawInfo *rbg, Vdp2 *varVdp2Regs)
{
    if (_Ygl->rbg_use_compute_shader == 0)
        return;
    RBGGenerator *instance = RBGGenerator::getInstance();
    instance->update(rbg, varVdp2Regs);
}
GLuint RBGGenerator_getTexture(int id)
{
    if (_Ygl->rbg_use_compute_shader == 0)
        return 0;

    RBGGenerator *instance = RBGGenerator::getInstance();
    return instance->getTexture(id);
}

GLuint RBGGenerator_getLnclTexture(int id)
{
    if (_Ygl->rbg_use_compute_shader == 0)
        return 0;

    RBGGenerator *instance = RBGGenerator::getInstance();
    return instance->getLnclTexture(id);
}

void RBGGenerator_onFinish()
{

    if (_Ygl->rbg_use_compute_shader == 0)
        return;
    RBGGenerator *instance = RBGGenerator::getInstance();
    instance->onFinish();
}
}
