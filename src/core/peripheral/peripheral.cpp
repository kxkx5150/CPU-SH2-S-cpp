/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006, 2013 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file peripheral.c
    \brief Peripheral shared functions.
*/

#include "debug.h"
#include "peripheral.h"

const char * PerPadNames[] =
{
"Up",
"Right",
"Down",
"Left",
"R",
"L",
"Start",
"A",
"B",
"C",
"X",
"Y",
"Z",
NULL
};

const char * PerMouseNames[] =
{
"A",
"B",
"C",
"Start",
NULL
};

PortData_struct PORTDATA1;
PortData_struct PORTDATA2;
static u8 IOPORT[ioPortMAX];

PerInterface_struct * PERCore = NULL;
extern PerInterface_struct * PERCoreList[];

typedef struct {
	u8 name;
	void (*Press)(void *);
	void (*Release)(void *);
   void (*SetAxisValue)(void *, u32);
   void (*MoveAxis)(void *, s32, s32);
} PerBaseConfig_struct;

typedef struct {
	u32 key;
	PerBaseConfig_struct * base;
	void * controller;
} PerConfig_struct;

typedef struct {
        u32 key;
        u8* port;
        u8 mask;
} PerIO_struct;

static PerIO_struct* IOkeys[256];

static PerIO_struct IOalloc[ioPortMAX][8];

#define PERCB(func) ((void (*) (void *)) func)
#define PERVALCB(func) ((void (*) (void *, u32)) func)
#define PERMOVECB(func) ((void (*) (void *, s32, s32)) func)

PerBaseConfig_struct perpadbaseconfig[] = {
	{ PERPAD_UP, PERCB(PerPadUpPressed), PERCB(PerPadUpReleased), NULL, NULL },
	{ PERPAD_RIGHT, PERCB(PerPadRightPressed), PERCB(PerPadRightReleased), NULL, NULL },
	{ PERPAD_DOWN, PERCB(PerPadDownPressed), PERCB(PerPadDownReleased), NULL, NULL },
	{ PERPAD_LEFT, PERCB(PerPadLeftPressed), PERCB(PerPadLeftReleased), NULL, NULL },
	{ PERPAD_RIGHT_TRIGGER, PERCB(PerPadRTriggerPressed), PERCB(PerPadRTriggerReleased), NULL, NULL },
	{ PERPAD_LEFT_TRIGGER, PERCB(PerPadLTriggerPressed), PERCB(PerPadLTriggerReleased), NULL, NULL },
	{ PERPAD_START, PERCB(PerPadStartPressed), PERCB(PerPadStartReleased), NULL, NULL },
	{ PERPAD_A, PERCB(PerPadAPressed), PERCB(PerPadAReleased), NULL, NULL },
	{ PERPAD_B, PERCB(PerPadBPressed), PERCB(PerPadBReleased), NULL, NULL },
	{ PERPAD_C, PERCB(PerPadCPressed), PERCB(PerPadCReleased), NULL, NULL },
	{ PERPAD_X, PERCB(PerPadXPressed), PERCB(PerPadXReleased), NULL, NULL },
	{ PERPAD_Y, PERCB(PerPadYPressed), PERCB(PerPadYReleased), NULL, NULL },
	{ PERPAD_Z, PERCB(PerPadZPressed), PERCB(PerPadZReleased), NULL, NULL },
};

PerBaseConfig_struct percabinetbaseconfig[] = {
	{ PERPAD_UP, PERCB(PerCabUpPressed), PERCB(PerCabUpReleased), NULL, NULL },
	{ PERPAD_RIGHT, PERCB(PerCabRightPressed), PERCB(PerCabRightReleased), NULL, NULL },
	{ PERPAD_DOWN, PERCB(PerCabDownPressed), PERCB(PerCabDownReleased), NULL, NULL },
	{ PERPAD_LEFT, PERCB(PerCabLeftPressed), PERCB(PerCabLeftReleased), NULL, NULL },
	{ PERJAMMA_TEST, PERCB(PerCabTestPressed), PERCB(PerCabTestReleased), NULL, NULL },
	{ PERJAMMA_SERVICE, PERCB(PerCabServicePressed), PERCB(PerCabServiceReleased), NULL, NULL },
	{ PERJAMMA_START1, PERCB(PerCabStart1Pressed), PERCB(PerCabStart1Released), NULL, NULL },
	{ PERJAMMA_START2, PERCB(PerCabStart2Pressed), PERCB(PerCabStart2Released), NULL, NULL },
	{ PERJAMMA_COIN1, PERCB(PerCabCoin1Pressed), PERCB(PerCabCoin1Released), NULL, NULL },
	{ PERJAMMA_COIN2, PERCB(PerCabCoin2Pressed), PERCB(PerCabCoin2Released), NULL, NULL },
	{ PERJAMMA_MULTICART, PERCB(PerCabMultiCartPressed), PERCB(PerCabMultiCartReleased), NULL, NULL },
	{ PERJAMMA_PAUSE, PERCB(PerCabPausePressed), PERCB(PerCabPauseReleased), NULL, NULL },
	{ PERPAD_A, PERCB(PerCabAPressed), PERCB(PerCabAReleased), NULL, NULL },
	{ PERPAD_B, PERCB(PerCabBPressed), PERCB(PerCabBReleased), NULL, NULL },
	{ PERPAD_C, PERCB(PerCabCPressed), PERCB(PerCabCReleased), NULL, NULL },
	{ PERPAD_X, PERCB(PerCabXPressed), PERCB(PerCabXReleased), NULL, NULL },
	{ PERPAD_Y, PERCB(PerCabYPressed), PERCB(PerCabYReleased), NULL, NULL },
	{ PERPAD_Z, PERCB(PerCabZPressed), PERCB(PerCabZReleased), NULL, NULL },
	{ PERJAMMA_P2_UP, PERCB(PerCabP2UpPressed), PERCB(PerCabP2UpReleased), NULL, NULL },
	{ PERJAMMA_P2_RIGHT, PERCB(PerCabP2RightPressed), PERCB(PerCabP2RightReleased), NULL, NULL },
	{ PERJAMMA_P2_DOWN, PERCB(PerCabP2DownPressed), PERCB(PerCabP2DownReleased), NULL, NULL },
	{ PERJAMMA_P2_LEFT, PERCB(PerCabP2LeftPressed), PERCB(PerCabP2LeftReleased), NULL, NULL },
	{ PERJAMMA_P2_BUTTON1, PERCB(PerCabP2Button1Pressed), PERCB(PerCabP2Button1Released), NULL, NULL },
	{ PERJAMMA_P2_BUTTON2, PERCB(PerCabP2Button2Pressed), PERCB(PerCabP2Button2Released), NULL, NULL },
	{ PERJAMMA_P2_BUTTON3, PERCB(PerCabP2Button3Pressed), PERCB(PerCabP2Button3Released), NULL, NULL },
	{ PERJAMMA_P2_BUTTON4, PERCB(PerCabP2Button4Pressed), PERCB(PerCabP2Button4Released), NULL, NULL },
	{ PERJAMMA_P2_BUTTON5, PERCB(PerCabP2Button5Pressed), PERCB(PerCabP2Button5Released), NULL, NULL },
	{ PERJAMMA_P2_BUTTON6, PERCB(PerCabP2Button6Pressed), PERCB(PerCabP2Button6Released), NULL, NULL },
};

PerBaseConfig_struct permousebaseconfig[] = {
	{ PERMOUSE_LEFT, PERCB(PerMouseLeftPressed), PERCB(PerMouseLeftReleased), NULL, NULL },
	{ PERMOUSE_MIDDLE, PERCB(PerMouseMiddlePressed), PERCB(PerMouseMiddleReleased), NULL, NULL },
	{ PERMOUSE_RIGHT, PERCB(PerMouseRightPressed), PERCB(PerMouseRightReleased), NULL, NULL },
	{ PERMOUSE_START, PERCB(PerMouseStartPressed), PERCB(PerMouseStartReleased), NULL, NULL },
	{ PERMOUSE_AXIS, NULL, NULL, NULL, PERMOVECB(PerMouseMove) },
};

PerBaseConfig_struct peranalogbaseconfig[] = {
	{ PERANALOG_AXIS1, NULL, NULL, PERVALCB(PerAxis1Value), NULL },
	{ PERANALOG_AXIS2, NULL, NULL, PERVALCB(PerAxis2Value), NULL },
	{ PERANALOG_AXIS3, NULL, NULL, PERVALCB(PerAxis3Value), NULL },
	{ PERANALOG_AXIS4, NULL, NULL, PERVALCB(PerAxis4Value), NULL },
	{ PERANALOG_AXIS5, NULL, NULL, PERVALCB(PerAxis5Value), NULL },
	{ PERANALOG_AXIS6, NULL, NULL, PERVALCB(PerAxis6Value), NULL },
	{ PERANALOG_AXIS7, NULL, NULL, PERVALCB(PerAxis7Value), NULL },
};

PerBaseConfig_struct pergunbaseconfig[] = {
	{ PERGUN_TRIGGER, PERCB(PerGunTriggerPressed), PERCB(PerGunTriggerReleased), NULL, NULL },
	{ PERGUN_START, PERCB(PerGunStartPressed), PERCB(PerGunStartReleased), NULL, NULL },
	{ PERGUN_AXIS, NULL, NULL, NULL, PERMOVECB(PerGunMove) },
};

static u32 perkeyconfigsize = 0;
static PerConfig_struct * perkeyconfig = NULL;

static void PerUpdateConfig(PerBaseConfig_struct * baseconfig, int nelems, void * controller);

u8 m_system_output = 0;
u8 m_ioga_mode = 0;
u8 m_ioga_portg = 0;

int IOPortAdd(int key, ioPort port, u8 index) {
  if (index >= 8) return -1;
  IOalloc[port][index].key = key;
  IOalloc[port][index].port = &IOPORT[port];
  IOalloc[port][index].mask = (0x1 << index);
  IOkeys[key] = &(IOalloc[port][index]);
  return 0;
}

static void IOPortPressed(int key) {
  if (IOkeys[(key & 0xFF)] != NULL){
    (*IOkeys[(key & 0xFF)]->port) &= ~IOkeys[(key & 0xFF)]->mask;
  }
}
static void IOPortReleased(int key) {
  if (IOkeys[(key & 0xFF)] != NULL){
    (*IOkeys[(key & 0xFF)]->port) |= IOkeys[(key & 0xFF)]->mask;
  }
}


u8 FASTCALL IOPortReadByte(SH2_struct *context, UNUSED u8* memory,  u32 addr)
{
/*
    0x0001 PORT-A (P1)
    0x0003 PORT-B (P2)
    0x0005 PORT-C (SYSTEM)
    0x0007 PORT-D (OUTPUT)
    0x0009 PORT-E (P3)
    0x000b PORT-F (P4 / Extra 6B layout)
    0x000d PORT-G
    0x000f unused
    0x0011 PORT_DIRECTION (each bit configure above IO ports, 1 - input, 0 - output)
    ---x ---- joystick/mahjong panel select
    ---- x--- used in danchih (different mux scheme for the hanafuda panel?)
    0x0013 RS422 TXD1
    0x0015 RS422 TXD2 SERIAL COM Tx
    0x0017 RS422 RXD1
    0x0019 RS422 RXD2 SERIAL COM Rx
    0x001b RS422 FLAG
    0x001d MODE (bit 7 - set PORT-G to counter-mode, bits 0-5 - RS422 satellite mode and node#)  <Technical Bowling>
    0x001f PORT-AD (8ch, write: bits 0-2 - set channel, read: channel data with autoinc channel number)
*/
   addr = addr&0x1F;
   u8 val = 0x0;
   switch(addr) {
     case 0x01: val = IOPORT[PORT_A]; break; // P1
     case 0x03: val = IOPORT[PORT_B]; break; // P2
     case 0x05: val = IOPORT[PORT_C]; break; // SYSTEM //Return press status like declared in stv.cpp
     case 0x07: val = m_system_output; break; // port D, read-backs value written
     case 0x09: val = IOPORT[PORT_E]; break; // P3
     case 0x0b: val = IOPORT[PORT_F]; break; // P4
     case 0x0d: //PORT-G
       if (m_ioga_mode & 0x80) // PORT-G in counter mode
       {
         val = IOPORT[PORT_G0+((m_ioga_portg >> 1) & 3)] >> (((m_ioga_portg & 1) ^ 1) * 8);
         m_ioga_portg = (m_ioga_portg & 0xf8) | ((m_ioga_portg + 1) & 7); // counter# is auto-incremented then read
       }
       else
         val = IOPORT[PORT_G];
       break;
     case 0x1b: val = 0x0; break; // Serial COM READ status
     case 0x1d: val = m_ioga_mode; break;

   }
   return val;
}

u16 FASTCALL IOPortReadWord(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
   return IOPortReadByte(context, memory, addr|1);
}

void FASTCALL IOPortWriteByte(SH2_struct *context, UNUSED u8* memory,UNUSED u32 addr, UNUSED u8 val)
{
   addr = addr&0x1F;
  switch(addr)
  {
    case 0x07:
      m_system_output = val;
      break;
    case 0x09: IOPORT[PORT_F] = val;
               IOPORT[PORT_G] = val;
               break; // P3
    case 0x0b: IOPORT[PORT_E] = val; break; // P4
    case 0x0d:
      //port-g
      m_ioga_portg = val;
      break;
    case 0x1d:
      m_ioga_mode = val;
      break;
  }
}

//////////////////////////////////////////////////////////////////////////////

int PerInit(int coreid) {
   int i;

   // So which core do we want?
   if (coreid == PERCORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; PERCoreList[i] != NULL; i++)
   {
      if (PERCoreList[i]->id == coreid)
      {
         // Set to current core
         PERCore = PERCoreList[i];
         break;
      }
   }

   if (PERCore == NULL)
      return -1;

   if (PERCore->Init() != 0)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PerDeInit(void) {
   if (PERCore)
      PERCore->DeInit();
   PERCore = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadUpPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xEF;
   SMPCLOG("Up\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadUpReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xEF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadDownPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xDF;
   SMPCLOG("Down\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadDownReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xDF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRightPressed(PerPad_struct * pad) {
   *pad->padbits &= 0x7F;
   SMPCLOG("Right\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRightReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0x7F;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLeftPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xBF;
   SMPCLOG("Left\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLeftReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xBF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadStartPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xF7;
   SMPCLOG("Start\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadStartReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xF7;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadAPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xFB;
   SMPCLOG("A\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadAReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xFB;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadBPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xFE;
   SMPCLOG("B\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadBReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xFE;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadCPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xFD;
   SMPCLOG("C\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadCReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xFD;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadXPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xBF;
   SMPCLOG("X\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadXReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xBF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadYPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xDF;
   SMPCLOG("Y\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadYReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xDF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadZPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xEF;
   SMPCLOG("Z\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadZReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xEF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRTriggerPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0x7F;
   SMPCLOG("Right Trigger\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRTriggerReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0x7F;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLTriggerPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xF7;
   SMPCLOG("Left Trigger\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLTriggerReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xF7;
}

//////////////////////////////////////////////////////////////////////////////

/*
  IOPortAdd(KEYPAD(PERPAD_UP, 0), PORT_A, 0x5);
  IOPortAdd(KEYPAD(PERPAD_RIGHT, 0), PORT_A, 0x6);
  IOPortAdd(KEYPAD(PERPAD_DOWN, 0), PORT_A, 0x4);
  IOPortAdd(KEYPAD(PERPAD_LEFT, 0), PORT_A, 0x7);
  IOPortAdd(KEYPAD(PERPAD_A, 0), PORT_A, 0x0);
  IOPortAdd(KEYPAD(PERPAD_B, 0), PORT_A, 0x1);
  IOPortAdd(KEYPAD(PERPAD_C, 0), PORT_A, 0x2);
  IOPortAdd(KEYPAD(PERPAD_X, 0), PORT_A, 0x3);

  IOPortAdd(PERJAMMA_COIN1, PORT_C, 0x0 );
  IOPortAdd(PERJAMMA_COIN2, PORT_C, 0x1 );
  IOPortAdd(PERJAMMA_TEST, PORT_C, 0x2);
  IOPortAdd(PERJAMMA_SERVICE, PORT_C, 0x3);
  IOPortAdd(PERJAMMA_START1, PORT_C, 0x4);
  IOPortAdd(PERJAMMA_START2, PORT_C, 0x5);
  IOPortAdd(PERJAMMA_MULTICART, PORT_C, 0x6);
  IOPortAdd(PERJAMMA_PAUSE, PORT_C, 0x7);
*/

/* P1 inputs */

void PerCabUpPressed(PerCab_struct * pad) {
   pad[PORT_A] &= ~(0x1 << 0x5);
}

void PerCabUpReleased(PerCab_struct * pad) {
   pad[PORT_A] |= (0x1 << 0x5);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabDownPressed(PerCab_struct * pad) {
   pad[PORT_A] &= ~(0x1 << 0x4);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabDownReleased(PerCab_struct * pad) {
   pad[PORT_A] |= (0x1 << 0x4);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabRightPressed(PerCab_struct * pad) {
   pad[PORT_A] &= ~(0x1 << 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabRightReleased(PerCab_struct * pad) {
   pad[PORT_A] |= (0x1 << 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabLeftPressed(PerCab_struct * pad) {
   pad[PORT_A] &= ~(0x1 << 0x7);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabLeftReleased(PerCab_struct * pad) {
   pad[PORT_A] |= (0x1 << 0x7);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabAPressed(PerCab_struct * pad) {
   pad[PORT_A] &= ~(0x1 << 0x0);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabAReleased(PerCab_struct * pad) {
   pad[PORT_A] |= (0x1 << 0x0);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabBPressed(PerCab_struct * pad) {
   pad[PORT_A] &= ~(0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabBReleased(PerCab_struct * pad) {
   pad[PORT_A] |= (0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabCPressed(PerCab_struct * pad) {
   pad[PORT_A] &= ~(0x1 << 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabCReleased(PerCab_struct * pad) {
   pad[PORT_A] |= (0x1 << 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabXPressed(PerCab_struct * pad) {
   switch (yabsys.stvInputType)
   {
      case STV6B:
         pad[PORT_F] &= ~(0x1 << 0x0); break;
      case STV:
      default:
         pad[PORT_A] &= ~(0x1 << 0x3); break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerCabXReleased(PerCab_struct * pad) {
   switch (yabsys.stvInputType)
   {
      case STV6B:
         pad[PORT_F] |= (0x1 << 0x0); break;
      case STV:
      default:
         pad[PORT_A] |= (0x1 << 0x3); break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerCabYPressed(PerCab_struct * pad) {
   pad[PORT_F] &= ~(0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabYReleased(PerCab_struct * pad) {
   pad[PORT_F] |= (0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabZPressed(PerCab_struct * pad) {
   pad[PORT_F] &= ~(0x1 << 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabZReleased(PerCab_struct * pad) {
   pad[PORT_F] |= (0x1 << 0x2);
}

/* P2 Inputs */

void PerCabP2UpPressed(PerCab_struct * pad) {
   pad[PORT_B] &= ~(0x1 << 0x5);
}

void PerCabP2UpReleased(PerCab_struct * pad) {
   pad[PORT_B] |= (0x1 << 0x5);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2DownPressed(PerCab_struct * pad) {
   pad[PORT_B] &= ~(0x1 << 0x4);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2DownReleased(PerCab_struct * pad) {
   pad[PORT_B] |= (0x1 << 0x4);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2RightPressed(PerCab_struct * pad) {
   pad[PORT_B] &= ~(0x1 << 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2RightReleased(PerCab_struct * pad) {
   pad[PORT_B] |= (0x1 << 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2LeftPressed(PerCab_struct * pad) {
   pad[PORT_B] &= ~(0x1 << 0x7);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2LeftReleased(PerCab_struct * pad) {
   pad[PORT_B] |= (0x1 << 0x7);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button1Pressed(PerCab_struct * pad) {
   pad[PORT_B] &= ~(0x1 << 0x0);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button1Released(PerCab_struct * pad) {
   pad[PORT_B] |= (0x1 << 0x0);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button2Pressed(PerCab_struct * pad) {
   pad[PORT_B] &= ~(0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button2Released(PerCab_struct * pad) {
   pad[PORT_B] |= (0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button3Pressed(PerCab_struct * pad) {
   pad[PORT_B] &= ~(0x1 << 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button3Released(PerCab_struct * pad) {
   pad[PORT_B] |= (0x1 << 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button4Pressed(PerCab_struct * pad) {
   switch (yabsys.stvInputType)
   {
      case STV6B:
         pad[PORT_F] &= ~(0x1 << 0x4); break;
      case STV:
      default:
         pad[PORT_B] &= ~(0x1 << 0x3); break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button4Released(PerCab_struct * pad) {
   switch (yabsys.stvInputType)
   {
      case STV6B:
         pad[PORT_F] |= (0x1 << 0x4); break;
      case STV:
      default:
         pad[PORT_B] |= (0x1 << 0x3); break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button5Pressed(PerCab_struct * pad) {
   pad[PORT_F] &= ~(0x1 << 0x5);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button5Released(PerCab_struct * pad) {
   pad[PORT_F] |= (0x1 << 0x5);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button6Pressed(PerCab_struct * pad) {
   pad[PORT_F] &= ~(0x1 << 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabP2Button6Released(PerCab_struct * pad) {
   pad[PORT_F] |= (0x1 << 0x6);
}

/* System Inputs*/

void PerCabCoin1Released(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x0);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabCoin1Pressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x0);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabCoin2Released(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabCoin2Pressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x1);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabTestPressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabTestReleased(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabServicePressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x3);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabServiceReleased(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x3);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabStart1Released(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x4);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabStart1Pressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x4);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabStart2Released(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x5);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabStart2Pressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x5);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabMultiCartPressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabMultiCartReleased(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabPausePressed(PerCab_struct * pad) {
   pad[PORT_C] &= ~(0x1 << 0x7);
}

//////////////////////////////////////////////////////////////////////////////

void PerCabPauseReleased(PerCab_struct * pad) {
   pad[PORT_C] |= (0x1 << 0x7);
}

//////////////////////////////////////////////////////////////////////////////


void PerMouseLeftPressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 1;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseLeftReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xFE;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseMiddlePressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 4;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseMiddleReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xFFFB;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseRightPressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 2;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseRightReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xFD;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseStartPressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 8;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseStartReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xF7;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseMove(PerMouse_struct * mouse, s32 dispx, s32 dispy)
{
   int negx, negy, overflowx, overflowy;
   u8 diffx, diffy;

   negx = ((mouse->mousebits[0] >> 4) & 1);
   negy = ((mouse->mousebits[0] >> 5) & 1);
   overflowx = ((mouse->mousebits[0] >> 6) & 1);
   overflowy = ((mouse->mousebits[0] >> 7) & 1);

   if (negx) diffx = ~(mouse->mousebits[1]) & 0xFF;
   else diffx = mouse->mousebits[1];
   if (negy) diffy = ~(mouse->mousebits[2]) & 0xFF;
   else diffy = mouse->mousebits[2];

   if (dispx >= 0)
   {
      if (negx)
      {
         if (dispx - diffx > 0)
         {
            diffx = dispx - diffx;
            negx = 0;
         }
         else diffx -= -dispx;
      }
      else diffx += dispx;
   }
   else
   {
      if (negx) diffx += -dispx;
      else
      {
         if (diffx + dispx > 0) diffx += dispx;
         else
         {
            diffx = -dispx - diffx;
            negx = 1;
         }
      }
   }

   if (dispy >= 0)
   {
      if (negy)
      {
         if (dispy - diffy > 0)
         {
            diffy = dispy - diffy;
            negy = 0;
         }
         else diffy -= -dispy;
      }
      else diffy += dispy;
   }
   else
   {
      if (negy) diffy += -dispy;
      else
      {
         if (diffy + dispy > 0) diffy += dispy;
         else
         {
            diffy = -dispy - diffy;
            negy = 1;
         }
      }
   }

   mouse->mousebits[0] = (overflowy << 7) | (overflowx << 6) | (negy << 5) | (negx << 4) | (mouse->mousebits[0] & 0x0F);
   if (negx) mouse->mousebits[1] = ~(diffx);
   else mouse->mousebits[1] = diffx;
   if (negy) mouse->mousebits[2] = ~(diffy);
   else mouse->mousebits[2] = diffy;
}

//////////////////////////////////////////////////////////////////////////////

void PerAxis1Value(PerAnalog_struct * analog, u32 val)
{
   analog->analogbits[2] = (u8)val;

   //handle wheel left/right standard pad presses depending on wheel position
   if (analog->perid == PERWHEEL)
   {
      int left_is_pressed = (analog->analogbits[0] & (1 << 6)) == 0;
      int right_is_pressed = (analog->analogbits[0] & (1 << 7)) == 0;

      if (val <= 0x67)
         analog->analogbits[0] &= 0xBF;//press left
      else if (left_is_pressed && val >= 0x6f)
         analog->analogbits[0] |= ~0xBF;//release left


      if (val >= 0x97)
         analog->analogbits[0] &= 0x7F;//press right
      else if(right_is_pressed && val <= 0x8f)
         analog->analogbits[0] |= ~0x7F;//release right
   }
   else if (analog->perid == PERMISSIONSTICK ||
      analog->perid == PERTWINSTICKS)
   {
      int left_is_pressed = (analog->analogbits[0] & (1 << 6)) == 0;
      int right_is_pressed = (analog->analogbits[0] & (1 << 7)) == 0;

      if (val <= 0x56)
         analog->analogbits[0] &= 0xBF;//press left
      else if (left_is_pressed && val >= 0x6a)
         analog->analogbits[0] |= ~0xBF;//release left


      if (val >= 0xab)
         analog->analogbits[0] &= 0x7F;//press right
      else if (right_is_pressed && val <= 0x95)
         analog->analogbits[0] |= ~0x7F;//release right
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerAxis2Value(PerAnalog_struct * analog, u32 val)
{
   analog->analogbits[3] = (u8)val;

   if (analog->perid == PERMISSIONSTICK ||
      analog->perid == PERTWINSTICKS)
   {
      int up_is_pressed = (analog->analogbits[0] & (1 << 4)) == 0;
      int down_is_pressed = (analog->analogbits[0] & (1 << 5)) == 0;

      if (val <= 0x65)
         analog->analogbits[0] &= 0xEF;//press up
      else if (up_is_pressed && val >= 0x6a)
         analog->analogbits[0] |= ~0xEF;//release up


      if (val >= 0xa9)
         analog->analogbits[0] &= 0xDF;//press down
      else if (down_is_pressed && val <= 0x94)
         analog->analogbits[0] |= ~0xDF;//release down
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerAxis3Value(PerAnalog_struct * analog, u32 val)
{
   if (analog->perid != PERMISSIONSTICK)
      analog->analogbits[4] = (u8)val;
   else
      analog->analogbits[4] = (u8)(-(s8)val);//axis inverted on mission stick
}

//////////////////////////////////////////////////////////////////////////////

void PerAxis4Value(PerAnalog_struct * analog, u32 val)
{
   analog->analogbits[5] = (u8)val;
}

//////////////////////////////////////////////////////////////////////////////

//left stick L/R
void PerAxis5Value(PerAnalog_struct * analog, u32 val)
{
   analog->analogbits[6] = (u8)val;
}

//////////////////////////////////////////////////////////////////////////////

//left stick U/D
void PerAxis6Value(PerAnalog_struct * analog, u32 val)
{
      analog->analogbits[7] = (u8)val;
}

//////////////////////////////////////////////////////////////////////////////

//left stick throttle
void PerAxis7Value(PerAnalog_struct * analog, u32 val)
{
   if (analog->perid != PERTWINSTICKS)
      analog->analogbits[8] = (u8)val;
   else
      analog->analogbits[8] = (u8)(-(s8)val);//axis inverted on mission stick
}

//////////////////////////////////////////////////////////////////////////////

void PerGunTriggerPressed(PerGun_struct * gun)
{
   *(gun->gunbits) &= 0xEF;
}

//////////////////////////////////////////////////////////////////////////////

void PerGunTriggerReleased(PerGun_struct * gun)
{
   *(gun->gunbits) |= 0x10;
}

//////////////////////////////////////////////////////////////////////////////

void PerGunStartPressed(PerGun_struct * gun)
{
   *(gun->gunbits) &= 0xDF;
}

//////////////////////////////////////////////////////////////////////////////

void PerGunStartReleased(PerGun_struct * gun)
{
   *(gun->gunbits) |= 0x20;
}

//////////////////////////////////////////////////////////////////////////////

void PerGunMove(PerGun_struct * gun, s32 dispx, s32 dispy)
{
   int x, y;
   x = (*(gun->gunbits+1) << 8) +  *(gun->gunbits+2) + (dispx / 4);
   y = (*(gun->gunbits+3) << 8) +  *(gun->gunbits+4) - (dispy / 4);

   if (x < 0)
      x = 0;
   else if (x >= 320) // fix me
      x = 319;

   if (y < 0)
      y = 0;
   else if (y >= 224) // fix me
      y = 223;

   *(gun->gunbits+1) = x >> 8;
   *(gun->gunbits+2) = x;
   *(gun->gunbits+3) = y >> 8;
   *(gun->gunbits+4) = y;
}

//////////////////////////////////////////////////////////////////////////////

void * PerAddPeripheral(PortData_struct *port, int perid)
{
   int pernum = port->data[0] & 0xF;
   int i;
   int peroffset=1;
   u8 size;
   int current = 1;
   void * controller;

   if (pernum == 0xF)
     return NULL;
   else if (perid == PERGUN && pernum == 1) // Gun doesn't work with multi-tap
     return NULL;

   // if only one peripheral is connected use 0xF0(unless Gun), otherwise use 0x00 or 0x10
   if (pernum == 0)
   {
      pernum = 1;

      if (perid != PERGUN)
         port->data[0] = 0xF1;
      else
         port->data[0] = 0xA0;
   }
   else
   {
      if (pernum == 1)
      {
         u8 tmp = peroffset;
         tmp += (port->data[peroffset] & 0xF) + 1;

         for(i = 0;i < 5;i++)
            port->data[tmp + i] = 0xFF;
      }
      pernum = 6;
      port->data[0] = 0x16;

      // figure out where we're at, then add peripheral id + 1
      current = 0;
      size = port->data[peroffset] & 0xF;
      while ((current < pernum) && (size != 0xF))
      {
         peroffset += size + 1;
         current++;
         size = port->data[peroffset] & 0xF;
      }

      if (current == pernum)
      {
         return NULL;
      }
      current++;
   }

   port->data[peroffset] = perid;
   peroffset++;

   // set peripheral data for peripheral to default values and adjust size
   // of port data
   switch (perid)
   {
      case PERPAD:
         port->data[peroffset] = 0xFF;
         port->data[peroffset+1] = 0xFF;
         port->size = peroffset+(perid&0xF);
         break;
      case PERWHEEL:
         port->data[peroffset] = 0xFF;
         port->data[peroffset+1] = 0xFF;
         port->data[peroffset+2] = 0x7F;
         port->size = peroffset+(perid&0xF);
         break;
      case PERMISSIONSTICK:
         port->data[peroffset] = 0xFF;
         port->data[peroffset + 1] = 0xFF;
         port->data[peroffset + 2] = 0x7F;
         port->data[peroffset + 3] = 0x7F;
         port->data[peroffset + 4] = 0x7F;
         port->size = peroffset + (perid & 0xF);
         break;
      case PERTWINSTICKS://also double mission sticks
         port->data[peroffset] = 0xFF;
         port->data[peroffset + 1] = 0xFF;
         //right stick
         port->data[peroffset + 2] = 0x7F;
         port->data[peroffset + 3] = 0x7F;
         port->data[peroffset + 4] = 0x7F;
         //left stick
         port->data[peroffset + 5] = 0x7F;
         port->data[peroffset + 6] = 0x7F;
         port->data[peroffset + 7] = 0x7F;
         port->size = peroffset + (perid & 0xF);
         break;
      case PER3DPAD:
         port->data[peroffset] = 0xFF;
         port->data[peroffset+1] = 0xFF;
         port->data[peroffset+2] = 0x7F;
         port->data[peroffset+3] = 0x7F;
         port->data[peroffset+4] = 0x7F;
         port->data[peroffset+5] = 0x7F;
         port->size = peroffset+(perid&0xF);
         break;
      case PERGUN:
         port->data[peroffset] = 0x7C;
         port->data[peroffset + 1] = 0xFF;
         port->data[peroffset + 2] = 0xFF;
         port->data[peroffset + 3] = 0xFF;
         port->data[peroffset + 4] = 0xFF;
         port->size = 1;
         break;
      case PERKEYBOARD:
         port->data[peroffset] = 0xFF;
         port->data[peroffset+1] = 0xF8;
         port->data[peroffset+2] = 0x06;
         port->data[peroffset+3] = 0x00;
         port->size = peroffset+(perid&0xF);
         break;
      case PERMOUSE:
         port->data[peroffset] = 0;
         port->data[peroffset + 1] = 0;
         port->data[peroffset + 2] = 0;
         port->size = peroffset+(perid&0xF);
         break;
      default: break;
   }

   if (perid != PERGUN)
   {
      u8 tmp = peroffset;
      tmp += (perid & 0xF);
      for(i = 0;i < (pernum - current);i++)
      {
         port->data[tmp + i] = 0xFF;
         port->size++;
      }
   }

   controller = (port->data + (peroffset - 1));
   switch (perid)
   {
      case PERPAD:
         PerUpdateConfig(perpadbaseconfig, sizeof(perpadbaseconfig)/sizeof(PerBaseConfig_struct), controller);
         break;
      case PERWHEEL:
      case PERMISSIONSTICK:
      case PER3DPAD:
      case PERTWINSTICKS:
         PerUpdateConfig(perpadbaseconfig, sizeof(perpadbaseconfig)/sizeof(PerBaseConfig_struct), controller);
         PerUpdateConfig(peranalogbaseconfig, sizeof(peranalogbaseconfig)/sizeof(PerBaseConfig_struct), controller);
         break;
      case PERGUN:
         PerUpdateConfig(pergunbaseconfig, sizeof(pergunbaseconfig)/sizeof(PerBaseConfig_struct), controller);
         break;
      case PERMOUSE:
         PerUpdateConfig(permousebaseconfig, sizeof(permousebaseconfig)/sizeof(PerBaseConfig_struct), controller);
         break;
   }
   return controller;
}

//////////////////////////////////////////////////////////////////////////////

int PerGetId(void * peripheral)
{
   u8 * id = (u8*)peripheral;
   return *id;
}

//////////////////////////////////////////////////////////////////////////////

void PerFlush(PortData_struct * port)
{
   /* FIXME this function only flush data if there's a mouse connected as
    * first peripheral */
  u8 perid = port->data[1];
  if (perid == 0xE3)
  {
     PerMouse_struct * mouse = (PerMouse_struct *) (port->data + 1);

     mouse->mousebits[0] &= 0x0F;
     mouse->mousebits[1] = 0;
     mouse->mousebits[2] = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////

void PerKeyDown(u32 key)
{
	unsigned int i = 0;

	while(i < perkeyconfigsize)
	{
		if (key == perkeyconfig[i].key)
		{
			if (perkeyconfig[i].base->Press) perkeyconfig[i].base->Press(perkeyconfig[i].controller);
		}
		i++;
	}
}

//////////////////////////////////////////////////////////////////////////////

void PerKeyUp(u32 key)
{
	unsigned int i = 0;

	while(i < perkeyconfigsize)
	{
		if (key == perkeyconfig[i].key)
		{
			if (perkeyconfig[i].base->Release) perkeyconfig[i].base->Release(perkeyconfig[i].controller);
		}
		i++;
	}
}

//////////////////////////////////////////////////////////////////////////////

void PerSetKey(u32 key, u8 name, void * controller)
{
	unsigned int i = 0;
	while(i < perkeyconfigsize)
	{
		if ((name == perkeyconfig[i].base->name) && (controller == perkeyconfig[i].controller))
		{
			perkeyconfig[i].key = key;
		}
		i++;
	}
}

//////////////////////////////////////////////////////////////////////////////

void PerAxisValue(u32 key, u8 val)
{
   unsigned int i = 0;

   while(i < perkeyconfigsize)
   {
      if (key == perkeyconfig[i].key)
      {
         if (perkeyconfig[i].base->SetAxisValue)
            perkeyconfig[i].base->SetAxisValue(perkeyconfig[i].controller, val);
      }
      i++;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerAxisMove(u32 key, s32 dispx, s32 dispy)
{
   unsigned int i = 0;

   while(i < perkeyconfigsize)
   {
      if (key == perkeyconfig[i].key)
      {
         if (perkeyconfig[i].base->MoveAxis)
            perkeyconfig[i].base->MoveAxis(perkeyconfig[i].controller, dispx, dispy);
      }
      i++;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerPortReset(void)
{
  int i;
        PORTDATA1.data[0] = 0xF0;
        PORTDATA1.size = 1;
        PORTDATA2.data[0] = 0xF0;
        PORTDATA2.size = 1;

        for (i=0; i<ioPortMAX; i++)
          IOPORT[i] = 0xFF; //IOPORT are in pull up mode.
        for (i=0; i<256; i++)
          IOkeys[i] = NULL;

	perkeyconfigsize = 0;
        if (perkeyconfig)
           free(perkeyconfig);
	perkeyconfig = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void PerUpdateConfig(PerBaseConfig_struct * baseconfig, int nelems, void * controller)
{
   u32 oldsize = perkeyconfigsize;
   u32 i, j;

   perkeyconfigsize += nelems;

	 PerConfig_struct *new_data = (PerConfig_struct*)realloc(perkeyconfig, perkeyconfigsize * sizeof(PerConfig_struct));
 	if (new_data == NULL)
 	{
		YuiMsg("Peripheral realloc Error\n");
 	} else {
     perkeyconfig = new_data;
 	}

   j = 0;
   for(i = oldsize;i < perkeyconfigsize;i++)
   {
      perkeyconfig[i].base = baseconfig + j;
      perkeyconfig[i].controller = controller;
			perkeyconfig[i].key = -1;
      j++;
   }
}

//////////////////////////////////////////////////////////////////////////////

PerPad_struct * PerPadAdd(PortData_struct * port)
{
   return (PerPad_struct *) PerAddPeripheral(port, PERPAD);
}

//////////////////////////////////////////////////////////////////////////////

PerMouse_struct * PerMouseAdd(PortData_struct * port)
{
   return (PerMouse_struct *) PerAddPeripheral(port, PERMOUSE);
}

//////////////////////////////////////////////////////////////////////////////

PerAnalog_struct * PerWheelAdd(PortData_struct * port)
{
   return (PerAnalog_struct *) PerAddPeripheral(port, PERWHEEL);
}

//////////////////////////////////////////////////////////////////////////////

PerAnalog_struct * PerMissionStickAdd(PortData_struct * port)
{
   return (PerAnalog_struct *) PerAddPeripheral(port, PERMISSIONSTICK);
}

//////////////////////////////////////////////////////////////////////////////

PerAnalog_struct * Per3DPadAdd(PortData_struct * port)
{
   return (PerAnalog_struct *) PerAddPeripheral(port, PER3DPAD);
}

//////////////////////////////////////////////////////////////////////////////

PerAnalog_struct * PerTwinSticksAdd(PortData_struct * port)
{
   return (PerAnalog_struct *) PerAddPeripheral(port, PERTWINSTICKS);
}

//////////////////////////////////////////////////////////////////////////////

PerGun_struct * PerGunAdd(PortData_struct * port)
{
   return (PerGun_struct *) PerAddPeripheral(port, PERGUN);
}

PerCab_struct * PerCabAdd(PortData_struct * port)
{
  PerUpdateConfig(percabinetbaseconfig, sizeof(percabinetbaseconfig)/sizeof(PerBaseConfig_struct), IOPORT);
  return IOPORT;
}

//////////////////////////////////////////////////////////////////////////////
// Dummy Interface
//////////////////////////////////////////////////////////////////////////////

int PERDummyInit(void);
void PERDummyDeInit(void);
int PERDummyHandleEvents(void);

u32 PERDummyScan(u32 flags);
void PERDummyFlush(void);
void PERDummyKeyName(u32 key, char * name, int size);

PerInterface_struct PERDummy = {
PERCORE_DUMMY,
"Dummy Input Interface",
PERDummyInit,
PERDummyDeInit,
PERDummyHandleEvents,
PERDummyScan,
0,
PERDummyFlush,
PERDummyKeyName
};

//////////////////////////////////////////////////////////////////////////////

int PERDummyInit(void) {

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERDummyDeInit(void) {
}

//////////////////////////////////////////////////////////////////////////////

int PERDummyHandleEvents(void) {

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 PERDummyScan(u32 flags) {
   // Scan and return next action based on flags value
   // See PERSF_* in peripheral.h for full list of flags.
   // If no specified flags are supported return 0

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERDummyFlush(void) {
}

//////////////////////////////////////////////////////////////////////////////

void PERDummyKeyName(UNUSED u32 key, char * name, UNUSED int size) {
	*name = 0;
}
