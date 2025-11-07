/*TODO: Trident License Comment
 */
#include <string.h>
#include "flashctl.h"
#include "mp_sector.h"
#include "flashdb_low_lvl.h"

extern void MpCalCrystaltrimInit(mp_cal_xtal_trim_t *mp_cal_xtaltrim);

void tr_platform_token_write(uint8_t  *buffer,
                             uint8_t  buf_size,
                             uint32_t token_addr)
{
  nvm_mfg_token_write(token_addr, buf_size, buffer);
}

void tr_platform_token_read(void     *buffer,
                            uint8_t  buf_size,
                            uint32_t token_addr)
{
  nvm_mfg_token_read(token_addr, buf_size, buffer);
}

void tr_platform_token_process_xtal_trim(uint16_t xtal_trim)
{
  if ((xtal_trim != 0xFFFF) && (xtal_trim < 64))
  {
    mp_cal_xtal_trim_t xtal_data;
    memset(&xtal_data, 0, sizeof(xtal_data));

    xtal_data.flag    = 2;
    xtal_data.xo_trim = xtal_trim;
    MpCalCrystaltrimInit(&xtal_data);
  }
}

// TODO: define functions for other tokens that need to be processed on boot
// see tr_mfg_tokens.c
