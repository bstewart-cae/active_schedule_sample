/** @file mp_sector.h
 *
 * @brief MP sector driver header file.
 *
 */
/**
 * @defgroup mp_sector_group MP_Sector
 * @ingroup peripheral_group
 * @{
 * @brief Define MP_sector definitions, structures, and functions.
 */
#ifndef __MP_SECTOR_H__
#define __MP_SECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdint.h>
#include "security_enable_config.h"

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/
#ifdef __cplusplus
#define   __I     volatile             /*!< Defines 'read only' permissions */
#else
#define   __I     volatile const       /*!< Defines 'read only' permissions */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

/* following defines should be used for structure members */
#define     __IM     volatile const      /*! Defines 'read only' structure member permissions */
#define     __OM     volatile            /*! Defines 'write only' structure member permissions */
#define     __IOM    volatile            /*! Defines 'read / write' structure member permissions */


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define MP_SECTOR_VERSION_CODE         (0x58570000)
#define MP_SECTOR_VERSION_CODE_MASK    (0xFFFF0000)
#define MP_SECTOR_VERSION_ID_V1        (MP_SECTOR_VERSION_CODE + 0x0001)
#define MP_SECTOR_VERSION_ID_V2        (MP_SECTOR_VERSION_CODE + 0x0002)
#define MP_SECTOR_VERSION_ID           (MP_SECTOR_VERSION_ID_V2)

#define MP_SECTOR_CALIBRATION_TOOL_V1  (0xFFFFFFFF)
#define MP_SECTOR_CALIBRATION_TOOL_V2  (0x5A3C0001)
#define MP_SECTOR_CALIBRATION_SW_V1    (0xFFFFFFFE)
#define MP_SECTOR_CALIBRATION_SW_V2    (0x5A3C0002)

#if (MP_SECTOR_VERSION_ID == MP_SECTOR_VERSION_ID_V1)
#define MP_SECTOR_CALIBRATION_TOOL  (MP_SECTOR_CALIBRATION_TOOL_V1)
#define MP_SECTOR_CALIBRATION_SW    (MP_SECTOR_CALIBRATION_SW_V1)
#else
#define MP_SECTOR_CALIBRATION_TOOL  (MP_SECTOR_CALIBRATION_TOOL_V2)
#define MP_SECTOR_CALIBRATION_SW    (MP_SECTOR_CALIBRATION_SW_V2)
#endif


#if FLASHCTRL_SECURE_EN==1
#define MP_SECTOR_BASE_512K             (0x10070000UL)
#else
#define MP_SECTOR_BASE_512K    (0x00070000UL)
#endif


#define MP_SECTOR_RESERVED_ADDR_512K    (MP_SECTOR_BASE_512K + 0x0000UL)
#define MP_SECTOR_ZIGBEE_ADDR_512K      (MP_SECTOR_BASE_512K + 0x4000UL)
#define MP_SECTOR_BLE_ADDR_512K         (MP_SECTOR_BASE_512K + 0xC000UL)
#define MP_SECTOR_CAL_ADDR_512K         (MP_SECTOR_BASE_512K + 0xF000UL)
#define MP_SECTOR_INFO_ADDR_512K        (MP_SECTOR_BASE_512K + 0xFFC0UL)

#define MP_SECTOR_RESERVED_512K         ((mp_sector_reserved_t *) MP_SECTOR_RESERVED_ADDR_512K)
#define MP_SECTOR_ZIGBEE_512K           ((mp_sector_zigbee_t *) MP_SECTOR_ZIGBEE_ADDR_512K)
#define MP_SECTOR_BLE_512K              ((mp_sector_ble_t *) MP_SECTOR_BLE_ADDR_512K)
#define MP_SECTOR_CAL_512K              ((mp_sector_cal_t *) MP_SECTOR_CAL_ADDR_512K)
#define MP_SECTOR_INFO_512K             ((mp_sector_info_t *) MP_SECTOR_INFO_ADDR_512K)

#if FLASHCTRL_SECURE_EN==1
#define MP_SECTOR_BASE_1024K             (0x100F0000UL)
#else
#define MP_SECTOR_BASE_1024K    (0x000F0000UL)
#endif


#define MP_SECTOR_RESERVED_ADDR_1024K   (MP_SECTOR_BASE_1024K + 0x0000UL)
#define MP_SECTOR_ZIGBEE_ADDR_1024K     (MP_SECTOR_BASE_1024K + 0x4000UL)
#define MP_SECTOR_BLE_ADDR_1024K        (MP_SECTOR_BASE_1024K + 0xC000UL)
#define MP_SECTOR_CAL_ADDR_1024K        (MP_SECTOR_BASE_1024K + 0xF000UL)
#define MP_SECTOR_INFO_ADDR_1024K       (MP_SECTOR_BASE_1024K + 0xFFC0UL)

#define MP_SECTOR_RESERVED_1024K        ((mp_sector_reserved_t *) MP_SECTOR_RESERVED_ADDR_1024K)
#define MP_SECTOR_ZIGBEE_1024K          ((mp_sector_zigbee_t *) MP_SECTOR_ZIGBEE_ADDR_1024K)
#define MP_SECTOR_BLE_1024K             ((mp_sector_ble_t *) MP_SECTOR_BLE_ADDR_1024K)
#define MP_SECTOR_CAL_1024K             ((mp_sector_cal_t *) MP_SECTOR_CAL_ADDR_1024K)
#define MP_SECTOR_INFO_1024K            ((mp_sector_info_t *) MP_SECTOR_INFO_ADDR_1024K)


#if FLASHCTRL_SECURE_EN==1
#define MP_SECTOR_BASE_2048K             (0x101F0000UL)
#else
#define MP_SECTOR_BASE_2048K    (0x001F0000UL)
#endif


#define MP_SECTOR_RESERVED_ADDR_2048K   (MP_SECTOR_BASE_2048K + 0x0000UL)
#define MP_SECTOR_ZIGBEE_ADDR_2048K     (MP_SECTOR_BASE_2048K + 0x4000UL)
#define MP_SECTOR_BLE_ADDR_2048K        (MP_SECTOR_BASE_2048K + 0xC000UL)
#define MP_SECTOR_CAL_ADDR_2048K        (MP_SECTOR_BASE_2048K + 0xF000UL)
#define MP_SECTOR_INFO_ADDR_2048K       (MP_SECTOR_BASE_2048K + 0xFFC0UL)

#define MP_SECTOR_RESERVED_2048K        ((mp_sector_reserved_t *) MP_SECTOR_RESERVED_ADDR_2048K)
#define MP_SECTOR_ZIGBEE_2048K          ((mp_sector_zigbee_t *) MP_SECTOR_ZIGBEE_ADDR_2048K)
#define MP_SECTOR_BLE_2048K             ((mp_sector_ble_t *) MP_SECTOR_BLE_ADDR_2048K)
#define MP_SECTOR_CAL_2048K             ((mp_sector_cal_t *) MP_SECTOR_CAL_ADDR_2048K)
#define MP_SECTOR_INFO_2048K            ((mp_sector_info_t *) MP_SECTOR_INFO_ADDR_2048K)
/*
#define MP_SECTOR_RESERVED_SIZE     (sizeof(mp_sector_reserved_t))
#define MP_SECTOR_ZIGBEE_SIZE       (sizeof(mp_sector_zigbee_t))
#define MP_SECTOR_BLE_SIZE          (sizeof(mp_sector_ble_t))
#define MP_SECTOR_CAL_SIZE          (sizeof(mp_sector_cal_t))
#define MP_SECTOR_INFO_SIZE         (sizeof(mp_sector_info_t))
*/
#define MP_SECTOR_RESERVED_SIZE     (0x4000)
#define MP_SECTOR_ZIGBEE_SIZE       (0x8000)
#define MP_SECTOR_BLE_SIZE          (0x2000)
#define MP_SECTOR_CAL_SIZE          (0x1000)
#define MP_SECTOR_INFO_SIZE         (0x1000)
#define MP_SECTOR_TOTAL_SIZE        (MP_SECTOR_RESERVED_SIZE + MP_SECTOR_ZIGBEE_SIZE + MP_SECTOR_BLE_SIZE + MP_SECTOR_CAL_SIZE + MP_SECTOR_INFO_SIZE)
/*
#if (sizeof(mp_sector_reserved_t) > MP_SECTOR_RESERVED_SIZE)
#error MP Sector Reserved Structure size exceeds definition
#endif
#if (sizeof(mp_sector_zigbee_t) > MP_SECTOR_ZIGBEE_SIZE)
#error MP Sector Zigbee Structure size exceeds definition
#endif
#if (sizeof(mp_sector_ble_t) > MP_SECTOR_BLE_SIZE)
#error MP Sector BLE Structure size exceeds definition
#endif
#if (sizeof(mp_sector_cal_t) > MP_SECTOR_CAL_SIZE)
#error MP Sector Calibration Structure size exceeds definition
#endif
#if (sizeof(mp_sector_info_t) > MP_SECTOR_INFO_SIZE)
#error MP Sector Information Structure size exceeds definition
#endif
*/


#define MP_NULL                     (0xFF)
#define MP_VALID                    (0x7E)
#define MP_INVALID                  (0x00)

#define MP_ID_DCDC                        (0x5A010001)
#define MP_ID_LDOMV                       (0x5A010002)
#define MP_ID_LDOANA                      (0x5A010003)
#define MP_ID_LDODIG                      (0x5A010004)
#define MP_ID_RETLDO                      (0x5A010005)
#define MP_ID_VBAT_ADC                    (0x5A010006)
#define MP_ID_AIO_ADC                     (0x5A010007)
#define MP_ID_VCM_ADC                     (0x5A010008)
#define MP_ID_TEMP_ADC                    (0x5A010009)
#define MP_ID_POWER_FAIL                  (0x5A01000A)
#define MP_ID_CRYSTAL_TRIM                (0x5A01000B)
#define MP_ID_RF_BAND_SUPPORT             (0x5A01000C)
#define MP_ID_RFTRIM_2P4G                 (0x5A01000D)
#define MP_ID_RF_TRIM_SUBG0               (0x5A01000E)
#define MP_ID_RF_TRIM_SUBG1               (0x5A01000F)
#define MP_ID_RF_TRIM_SUBG2               (0x5A010010)
#define MP_ID_TX_POWER_TRIM               (0x5A010011)
#define MP_ID_RSSI_TRIM                   (0x5A010012)
#define MP_ID_TEMPK                       (0x5A010013)
#define MP_ID_TX_POWER_TRIM_2             (0x5A010014)
#define MP_ID_RF_TRIM_SUBG3               (0x5A010015)
#define MP_ID_TX_POWER_TRIM_3_RSSI_TRIM_2 (0x5A010016)
#define MP_ID_ANA_COM                     (0x5A010017)
#define MP_ID_BOD                         (0x5A010018)
#define MP_ID_AGC                         (0x5A010019)
#define MP_ID_IMRR                        (0x5A01001A)
#define MP_ID_RF_TRIM_584_2P4G            (0x5A01001B)
#define MP_ID_RF_TRIM_584_SUBG0           (0x5A01001C)
#define MP_ID_RF_TRIM_584_SUBG1           (0x5A01001D)
#define MP_ID_RF_TRIM_584_SUBG2           (0x5A01001E)
#define MP_ID_RF_TRIM_584_SUBG3           (0x5A01001F)

#define MP_ID_NULL                  (0xFFFFFFFF)

#define MP_CNT_DCDC                 (sizeof(mp_cal_regulator_t))
#define MP_CNT_LDOMV                (sizeof(mp_cal_regulator_t))
#define MP_CNT_LDOANA               (sizeof(mp_cal_regulator_t))
#define MP_CNT_LDODIG               (sizeof(mp_cal_regulator_t))
#define MP_CNT_RETLDO               (sizeof(mp_cal_regulator_t))
#define MP_CNT_VBATADC              (sizeof(mp_cal_adc_t))
#define MP_CNT_AIOADC               (sizeof(mp_cal_adc_t))
#define MP_CNT_VCMADC               (sizeof(mp_cal_vcm_adc_t))
#define MP_CNT_TEMPADC              (sizeof(mp_cal_temp_adc_t))
#define MP_CNT_POWERFAIL            (sizeof(mp_cal_regulator_t))
#define MP_CNT_CRYSTALTRIM          (sizeof(mp_cal_xtal_trim_t))
#define MP_CNT_RF_BAND_SUPPORT      (sizeof(mp_cal_rf_band_support_t))
#define MP_CNT_RFTRIM1              (sizeof(mp_cal_rf_trim_1_t))
#define MP_CNT_RFTRIM               (sizeof(mp_cal_rf_trim_t))
#define MP_CNT_TX_POWER_TRIM        (sizeof(mp_tx_power_trim_t))
#define MP_CNT_RSSI_TRIM            (sizeof(mp_rssi_trim_t))
#define MP_CNT_TEMPK                (sizeof(mp_temp_k_t))
#define MP_CNT_TX_POWER_TRIM_2      (sizeof(mp_tx_power_trim_2_t))
#define MP_CNT_ANA_COM_ADC          (sizeof(mp_cal_regulator_t))
#define MP_CNT_BOD_ADC              (sizeof(mp_cal_regulator_t))
#define MP_CNT_AGC                  (sizeof(mp_cal_agc_adc_t))
#define MP_CNT_IMRR                 (sizeof(mp_cal_imrr_t))


#define FT_DCDC_OFFSET            0x20
#define FT_LDOMV_OFFSET           0x30
#define FT_LDOANA_OFFSET          0x40
#define FT_LDODIG_OFFSET          0x50
#define FT_REDLDO_OFFSET          0x60

#define FT_VBAT_OFFSET            0x70
#define FT_AIO_OFFSET             0x80
#define FT_VCM_OFFSET             0x90
#define FT_TEMP_ADC_OFFSET        0xA0
#define FT_0V_ADC_OFFSET          0xB0
#define FT_ANA_OFFSET             0xC0
#define FT_BOD_OFFSET             0xD0
#define FT_AGC_OFFSET             0xE0
#define FT_RX_IMAGE_K             0xF0

#define MP_ID_VOSEL_MIN                     0
#define MP_ID_DCDC_LDOMV_VOSEL_MAX         31
#define MP_ID_LDOANA_DIG_RET_VOSEL_MAX     15

#define MP_ID_BOD_VOSEL_MAX                 10
#define MP_ID_ANA_VOSEL_MAX                 0
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
typedef enum
{
    MP_SECTOR_INVALID = 0,
    MP_SECTOR_VALID_MPTOOL,
    MP_SECTOR_VALID_SWDEFAULT,
    MP_SECTOR_VALID_SWCAL,
} mp_sector_valid_t;

typedef struct __attribute__((packed))
{
    uint32_t mp_id;
    uint8_t  mp_valid;
    uint16_t mp_cnt;
}
mp_sector_head_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint16_t voltage_1;
    uint8_t  vosel_1;
    uint16_t voltage_2;
    uint8_t  vosel_2;
    uint8_t  select;
    uint16_t target_voltage_1;
    uint8_t  target_vosel_1;
    uint16_t target_voltage_2;
    uint8_t  target_vosel_2;
    uint16_t target_voltage_3;
    uint8_t  target_vosel_3;
}
mp_cal_regulator_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint16_t voltage_1;
    uint16_t adc_1;
    uint16_t voltage_2;
    uint16_t adc_2;
    uint16_t target_voltage_1;
    uint16_t target_adc_1;
    uint16_t target_voltage_2;
    uint16_t target_adc_2;
    uint16_t target_voltage_3;
    uint16_t target_adc_3;
}
mp_cal_adc_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  enable;
    uint16_t adc_1;
}
mp_cal_vcm_adc_t;


typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint16_t adc_1;
}
mp_cal_temp_adc_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint16_t adc_3p3;
    uint16_t adc_1p8;
    uint16_t adc_temp;
}
mp_cal_agc_adc_t;


typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint16_t xo_trim;
}
mp_cal_xtal_trim_t;


typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint16_t rxiq_gain;
    uint8_t rxiq_gain_sel;
    uint16_t rxiq_phase;
    uint8_t rxiq_phase_sel;
}
mp_cal_imrr_t;


typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  rf_band;
}
mp_cal_rf_band_support_t;


typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  mode;
    uint16_t cal_cfg;
    uint16_t rx_filter_cap;
    uint16_t tx_dc_offset_i;
    uint16_t tx_dc_offset_q;
    uint16_t tx_iqc_a;
    uint16_t tx_iqc_b;
    uint16_t tx_iqc_c;
}
mp_cal_rf_trim_t;


typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  mode;
    uint16_t cal_cfg;
    uint16_t rx_filter_cap;
    uint16_t tx_dc_offset_i;
    uint16_t tx_dc_offset_q;
    uint16_t tx_iqc_a;
    uint16_t tx_iqc_b;
    uint16_t tx_iqc_c;
    uint8_t  rx_tia_dc_i_code;
    uint8_t  rx_tia_dc_i_path;
    uint8_t  rx_tia_dc_q_code;
    uint8_t  rx_tia_dc_q_path;
    uint8_t  rx_iq_gain;
    uint8_t  rx_iq_gain_sel;
    uint8_t  rx_iq_phase;
    uint8_t  rx_iq_phase_sel;
    uint8_t  tx_hd3_lpf_pw;
    uint8_t  tx_hd3_mixer_gain;
    uint8_t  tx_hd3_mixer_lovcm;
    uint8_t  tx_hd3_txlo_pw;
    uint8_t  tx_hd3_poly_gain;
}
mp_cal_rf_trim_1_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  mode;
    uint8_t  tx_gain_idx_2g_fsk;
    uint8_t  tx_gain_idx_subg0_fsk;
    uint8_t  tx_gain_idx_subg1_fsk;
    uint8_t  tx_gain_idx_subg2_fsk;
}
mp_tx_power_trim_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  mode;
    uint8_t  rssi_trim_2g;
    uint8_t  rssi_trim_subg0;
    uint8_t  rssi_trim_subg1;
    uint8_t  rssi_trim_subg2;
}
mp_rssi_trim_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint16_t ktvalue;
}
mp_temp_k_t;

typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  tx_gain_idx_2g_oqpsk;
    uint8_t  tx_gain_idx_subg0_oqpsk;
    uint8_t  tx_gain_idx_subg1_oqpsk;
    uint8_t  tx_gain_idx_subg2_oqpsk;
}
mp_tx_power_trim_2_t;


typedef struct __attribute__((packed))
{
    mp_sector_head_t head;
    uint8_t  flag;
    uint8_t  tx_gain_idx_subg3_fsk;
    uint8_t  tx_gain_idx_subg3_oqpsk;
    uint8_t  tx_gain_idx_subg4_fsk;
    uint8_t  tx_gain_idx_subg4_oqpsk;
    uint8_t  rssi_trim_subg3;
}
mp_tx_power_trim_3_rssi_trim_2_t;
/*
typedef struct __attribute__((packed)) {
    __IO  uint8_t  RESERVED[0x4000];               //offset: 0x0000
} mp_sector_reserved_t;
*/
/*
typedef struct __attribute__((packed)) {
    __IO  uint8_t  RESERVED[0x8000];               //offset: 0x0000
} mp_sector_zigbee_t;
*/
/*
typedef struct __attribute__((packed)) {
    __IO  uint8_t  RESERVED[0x2000];               //offset: 0x0000
} mp_sector_ble_t;
*/


//typedef struct __attribute__((packed))
//{
//    __IO  mp_cal_regulator_t        DCDC;                   //offset: 0x0000
//    __IO  mp_cal_regulator_t        LDOMV;                   //offset: 0x0018
//    __IO  mp_cal_regulator_t        LDOANA;                  //offset: 0x0030
//    __IO  mp_cal_regulator_t        LDODIG;                   //offset: 0x0048
//    __IO  mp_cal_regulator_t        RETLDO;                 //offset: 0x0060
//    __IO  mp_cal_adc_t              VBAT_ADC;               //offset: 0x0078
//    __IO  mp_cal_adc_t              AIO_ADC;                //offset: 0x0094
//    __IO  mp_cal_vcm_adc_t          VCM_ADC;                //offset: 0x00B0
//    __IO  mp_cal_temp_adc_t         TEMP_ADC;               //offset: 0x00BB
//    __IO  mp_cal_regulator_t        POWER_FAIL;             //offset: 0x00C5
//    __IO  mp_cal_xtal_trim_t        CRYSTAL_TRIM;           //offset: 0x00DD
//    __IO  mp_cal_rf_band_support_t  RF_BAND_SUPPORT;        //offset: 0x00E7
//    __IO  mp_cal_rf_trim_t          RF_TRIM_2P4G;           //offset: 0x00F0
//    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG0;          //offset: 0x0107
//    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG1;          //offset: 0x011E
//    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG2;          //offset: 0x0135
//    __IO  mp_tx_power_trim_t        TX_POWER_TRIM;          //offset: 0x014C
//    __IO  mp_rssi_trim_t            RSSI_TRIM;              //offset: 0x0159
//    __IO  mp_temp_k_t               TEMP_K;                 //offset: 0x0166
//    __IO  mp_tx_power_trim_2_t      TX_POWER_TRIM_2;        //offset: 0x0170
//    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG3;          //offset: 0x017C
//    __IO  mp_tx_power_trim_3_rssi_trim_2_t      TX_POWER_TRIM_3_RSSI_TRIM_2;        //offset: 0x0193
//    /*
//        __IO  uint8_t  RESERVED[0xEB9];                     //offset: 0x0147
//    */
//}
//mp_sector_cal_t;

typedef struct __attribute__((packed))
{
    __IO  mp_cal_regulator_t        DCDC;                   //offset: 0x0000
    __IO  mp_cal_regulator_t        LDOMV;                   //offset: 0x0018
    __IO  mp_cal_regulator_t        LDOANA;                  //offset: 0x0030
    __IO  mp_cal_regulator_t        LDODIG;                   //offset: 0x0048
    __IO  mp_cal_regulator_t        RETLDO;                 //offset: 0x0060
    __IO  mp_cal_adc_t              VBAT_ADC;               //offset: 0x0078
    __IO  mp_cal_adc_t              AIO_ADC;                //offset: 0x0094
    __IO  mp_cal_vcm_adc_t          VCM_ADC;                //offset: 0x00B0
    __IO  mp_cal_temp_adc_t         TEMP_ADC;               //offset: 0x00BB
    __IO  mp_cal_regulator_t        POWER_FAIL;             //offset: 0x00C5
    __IO  mp_cal_xtal_trim_t        CRYSTAL_TRIM;           //offset: 0x00DD
    __IO  mp_cal_rf_band_support_t  RF_BAND_SUPPORT;        //offset: 0x00E7
    __IO  mp_cal_rf_trim_t          RF_TRIM_2P4G;           //offset: 0x00F0
    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG0;          //offset: 0x0107
    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG1;          //offset: 0x011E
    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG2;          //offset: 0x0135
    __IO  mp_tx_power_trim_t        TX_POWER_TRIM;          //offset: 0x014C
    __IO  mp_rssi_trim_t            RSSI_TRIM;              //offset: 0x0159
    __IO  mp_temp_k_t               TEMP_K;                 //offset: 0x0166
    __IO  mp_tx_power_trim_2_t      TX_POWER_TRIM_2;        //offset: 0x0170
    __IO  mp_cal_rf_trim_t          RF_TRIM_SUBG3;          //offset: 0x017C
    __IO  mp_tx_power_trim_3_rssi_trim_2_t      TX_POWER_TRIM_3_RSSI_TRIM_2;        //offset: 0x0193
    __IO  mp_cal_regulator_t                    ANA_COM;        //offset: 0x0193
    __IO  mp_cal_regulator_t                    BOD_ADC;        //offset: 0x0193
    __IO  mp_cal_agc_adc_t                      AGC_ADC;        //offset: 0x0193
    __IO  mp_cal_imrr_t                         IMRR;        //offset: 0x0193
    __IO  mp_cal_rf_trim_1_t        RF_TRIM_584_2P4G;       //offset: 0x01EC
    __IO  mp_cal_rf_trim_1_t        RF_TRIM_584_SUBG0;      //offset: 0x0210
    __IO  mp_cal_rf_trim_1_t        RF_TRIM_584_SUBG1;      //offset: 0x0234
    __IO  mp_cal_rf_trim_1_t        RF_TRIM_584_SUBG2;      //offset: 0x0258
    __IO  mp_cal_rf_trim_1_t        RF_TRIM_584_SUBG3;      //offset: 0x027C
    /*
        __IO  uint8_t  RESERVED[0xEB9];                     //offset: 0x0147
    */
}
mp_sector_cal_t;






typedef struct __attribute__((packed))
{
    __IO  uint32_t  CAL_DATA_SECTOR_ADDR;          //offset: 0x0FC0
    __IO  uint32_t  CAL_DATA_SECTOR_SIZE;          //offset: 0x0FC4
    __IO  uint32_t  BLE_INFO_SECTOR_ADDR;          //offset: 0x0FC8
    __IO  uint32_t  BLE_INFO_SECTOR_SIZE;          //offset: 0x0FCC
    __IO  uint32_t  ZIGBEE_INFO_SECTOR_ADDR;       //offset: 0x0FD0
    __IO  uint32_t  ZIGBEE_INFO_SECTOR_SIZE;       //offset: 0x0FD4
    __IO  uint32_t  RVED[7];                       //offset: 0x0FD8
    __IO  uint32_t  MP_SECTOR_CALIBRATION;         //offset: 0x0FF4
    __IO  uint32_t  MP_SECTOR_VERSION;             //offset: 0x0FF8
    __IO  uint32_t  MP_SECTOR_SIZE;                //offset: 0x0FFC

}
mp_sector_info_t;





typedef struct __attribute__((packed))
{
    uint8_t     flag;
    uint16_t    voltage_1;
    uint8_t     vosel_1;
    uint16_t    voltage_2;
    uint8_t     vosel_2;
}
ft_cal_regulator_t;

typedef struct __attribute__((packed))
{
    uint8_t     flag;
    uint16_t    voltage_1;
    uint16_t    adc_1;
    uint16_t    voltage_2;
    uint16_t    adc_2;
    uint16_t    voltage_3;
    uint16_t    adc_3;
}
ft_cal_adc_t;

typedef struct __attribute__((packed))
{
    uint8_t  flag;
    uint8_t  enable;
    uint16_t adc_1;
}
ft_cal_vcm_temp_adc_t;


typedef struct __attribute__((packed))
{
    uint8_t  flag;
    uint8_t  adc_3p3;
    uint8_t  adc_1p8;
    uint8_t  adc_temp;
}
ft_cal_agc_adc_t;

typedef struct __attribute__((packed))
{
    uint8_t  flag;
    uint8_t  rx_iq_again;
    uint8_t  rx_iq_again_select;
    uint8_t  rx_iq_phase;
    uint8_t  rx_iq_phase_select;
}
ft_cal_img_k_t;

typedef struct __attribute__((packed))
{
    uint32_t   ver;
    uint32_t   size;
    uint32_t   cal;
    uint32_t   cal_data_sector_size;
    uint32_t   cal_data_sector_addr;
}
mp_sector_inf_t;


/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
//extern const mp_sector_zigbee_t mp_sector_zigbee;
//extern const mp_sector_ble_t mp_sector_ble;
extern const mp_sector_cal_t mp_sector_cal;
extern const mp_sector_info_t mp_sector_info;

typedef mp_cal_rf_trim_1_t MPK_RF_TRIM_T;

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
 * @brief load otp value into mp_sector table struct;
 * @param[in] mp_adc_cal
 *           \arg mp_sector_head_t head;
 *           \arg flag;
 *           \arg voltage_1;
 *           \arg adc_1;
 *           \arg voltage_2;
 *           \arg adc_2;
 *           \arg select;
 *           \arg target_voltage_1;
 *           \arg target_adc_1;
 *           \arg target_voltage_2;
 *           \arg target_adc_2;
 *           \arg target_voltage_3;
 *           \arg target_adc_3;
 * @param[in] otp_adc_cal
 *           \arg flag;
 *           \arg voltage_1;
 *           \arg adc_1;
 *           \arg voltage_2;
 *           \arg adc_2;
 *           \arg voltage_3;
 *           \arg adc_3;
 * @retval None
 */
void FtLoadAdcValue( uint32_t mp_id, mp_cal_adc_t *mp_adc_cal, ft_cal_adc_t *ft_adc_cal);

void FtLoad_ANA_BOD_Value(uint32_t mp_id, mp_cal_regulator_t *mp_cal, ft_cal_regulator_t *ft_pmu_cal);
/**
 * @brief Otp val
 * @param[in] mp_cal_reg
 *           \arg mp_sector_head_t head;
 *           \arg flag;
 *           \arg voltage_1;
 *           \arg vosel_1;
 *           \arg voltage_2;
 *           \arg vosel_2;
 *           \arg select;
 *           \arg target_voltage_1;
 *           \arg target_vosel_1;
 *           \arg target_voltage_2;
 *           \arg target_vosel_2;
 *           \arg target_voltage_3;
 *           \arg target_vosel_3;
 * @param[in] mp_id
 * @retval STATUS_SUCCESS
 * @retval STATUS_INVALID_PARAM
 */
uint32_t FtPmuRangeCheck(uint32_t mp_id, mp_cal_regulator_t *mp_pmu_cal);
/**
 * @brief Otp val
 * @param[in] mp_cal_reg
 *           \arg mp_sector_head_t head;
 *           \arg flag;
 *           \arg voltage_1;
 *           \arg adc_1;
 *           \arg voltage_2;
 *           \arg adc_2;
 *           \arg select;
 *           \arg target_voltage_1;
 *           \arg target_adc_1;
 *           \arg target_voltage_2;
 *           \arg target_adc_2;
 *           \arg target_voltage_3;
 *           \arg target_adc_3;
 * @param[in] mp_id
 * @retval STATUS_SUCCESS
 * @retval STATUS_INVALID_PARAM
 */
uint32_t FtAdcRangeCheck(uint32_t mp_id, mp_cal_adc_t *mp_adc_cal_reg);
/**
 * @brief Otp val
 * @param[in] mp_cal_reg
 *           \arg mp_sector_head_t head;
 *           \arg flag;
 *           \arg enable;
 *           \arg adc_1;
 * @param[in] tmp_mp_cal_reg
 *           \arg mp_sector_head_t head;
 *           \arg flag;
 *           \arg adc_1;
 * @param[in] mp_id
 * @retval STATUS_SUCCESS
 * @retval STATUS_INVALID_PARAM
 */

uint32_t FtVcmTempAdcRangeCheck(uint32_t mp_id, mp_cal_vcm_adc_t *mp_cal_reg, mp_cal_temp_adc_t *tmp_mp_cal_reg);
/**
 * @brief Otp pmu value to mp_sector calcation
 * @param[in]  mp_pmu_cal
 *           \arg mp_sector_head_t head;
 *           \arg flag;
 *           \arg voltage_1;
 *           \arg vosel_1;
 *           \arg voltage_2;
 *           \arg vosel_2;
 *           \arg select;
 *           \arg target_voltage_1;
 *           \arg target_vosel_1;
 *           \arg target_voltage_2;
 *           \arg target_vosel_2;
 *           \arg target_voltage_3;
 *           \arg target_vosel_3;
 * @retval None
 */
void FtToMpVoselCal(mp_cal_regulator_t *mp_pmu_cal);
/**
 * @brief Otp adc value to mp_sector calcation
 *           \arg mp_sector_head_t head;
 *           \arg flag;
 *           \arg voltage_1;
 *           \arg adc_1;
 *           \arg voltage_2;
 *           \arg adc_2;
 *           \arg select;
 *           \arg target_voltage_1;
 *           \arg target_adc_1;
 *           \arg target_voltage_2;
 *           \arg target_adc_2;
 *           \arg target_voltage_3;
 *           \arg target_adc_3;
 * @retval None
 */
void FtToMpAdcCal(mp_cal_adc_t *mp_cal);
/**
 * @brief ft data to mp_sector value calibration function
 * @param  Nonoe
 * @retval None
 */
uint32_t FtToMpCalibration(void);
/**
* @ingroup mp_sector_group
* @brief Function to get the Mp sector information initinal value
* @param[in] MpSectorInf
 *            \arg ver
 *            \arg size
 *            \arg cal
 *            \arg cal_data_sector_size
 *            \arg cal_data_sector_addr
* @retval STATUS_SUCCESS
* @retval STATUS_INVALID_PARAM
*/
uint32_t GetMpSectorInfo(mp_sector_inf_t *MpSectorInf);

/**
 * @brief Function to initinal the Mp sector calibration the value
 */
uint32_t MpSectorInit(void);

/**
 * @brief Function write rf mp secotr value
 * @param[in] mp_id
 * @param[in] mp_cal_rf
 *            \arg mp_sector_head_t head;
 *            \arg flag
 *            \arg mode
 *            \arg cal_cfg
 *            \arg rx_filter_cap
 *            \arg tx_dc_offset_i
 *            \arg tx_dc_offset_q
 *            \arg tx_iqc_a
 *            \arg tx_iqc_b
 *            \arg tx_iqc_c
 *            \arg rx_iq_gain
 *            \arg rx_iq_gain_sel
 *            \arg rx_iq_phase
 *            \arg rx_iq_phase_sel
 *            \arg tx_hd3_lpf_pw
 *            \arg tx_hd3_mixer_gain
 *            \arg tx_hd3_mixer_lovcm
 *            \arg tx_hd3_txlo_pw
 *            \arg tx_hd3_poly_gain
 * @retval STATUS_SUCCESS rf trim is successful
 * @retval STATUS_ERROR   write address is invaild
 */
uint32_t MpCalRftrimWrite(uint32_t mp_id, MPK_RF_TRIM_T *mp_cal_rf);

/**
 * @brief Function read rf mp secotr value
 * @param[in] mp_id
 * @param[in] byte_cnt read data bytes count
 * @param[out] mp_sec_data point a mp_sector data buffer
 * @retval STATUS_SUCCESS read mp sector value is succesful
 * @retval STATUS_INVALID_REQUEST read mp sector value is invaild
 */
uint32_t MpCalRftrimRead(uint32_t mp_id, uint32_t byte_cnt, uint8_t *mp_sec_data);
/**
 * @brief mp sector adc intinal
 */
void MpCalAdcInit(void);
/**
 * @brief mp sector adc intinal
 */
uint32_t MpCalVbatAdcRead(mp_cal_adc_t *mp_cal_adc);
/**
 * @brief mp sector adc intinal
 */
uint32_t MpCalAioAdcRead(mp_cal_adc_t *mp_cal_adc);
/**
 * @brief mp sector adc intinal
 */
uint32_t MpCalVcmAdcRead(mp_cal_vcm_adc_t *mp_cal_vcmadc);
/**
 * @brief mp sector adc intinal
 */
uint32_t MpCalTempAdcRead(mp_cal_temp_adc_t *mp_cal_tempadc);
/**
 * @brief mp sector adc intinal
 */
uint32_t MpCalKtRead(mp_temp_k_t *mp_cal_k);


/**
*
* @brief Mp sector read ana value
*/
uint32_t MpCalAnaRead(mp_cal_regulator_t *mp_cal_ana);
/**
*
* @brief Mp sector read BOD value
*/
uint32_t MpCalBodRead(mp_cal_regulator_t *mp_cal_bod);
/**
*
* @brief Mp sector Read AGC Value
*/
uint32_t MpCalAgcRead(mp_cal_agc_adc_t *mp_cal_agc);
/**
*
* @brief SW_Update_MpSectorTable
*/
uint32_t MpSectorCalSWUpdate(void);
/**
*
* @brief Check Security Page FT Data flag
*/
uint32_t MpSectorCheckFT(uint8_t ft_offset);
/**
*
* @brief Mp sector read tx pwr cfg
*/
uint8_t MpSectorReadTxPwrCfg(void);
/**
*
* @brief Mp sector update tx pwr cfg
*/
uint32_t MpSectorWriteTxPwrCfg(uint8_t txpowerlevel);

#ifdef __cplusplus
};
#endif

#endif /* end of __MP_SECTOR_H__ */

/** @} */ /* End of Peripheral Group */
