#ifndef __EEPROM__H__
#define __EEPROM__H__

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void eeprom_set_clk(u8 state);
extern void eeprom_set_di(u8 state);
extern void eeprom_set_cs(u8 state);
extern int  eeprom_do_read();
extern void eeprom_init(const char *filename);
extern void eeprom_deinit();
extern void eeprom_start(const u8 *table);

#ifdef __cplusplus
}
#endif

#endif
