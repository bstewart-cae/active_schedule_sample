/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/*
 *  Header file for generating mocks for sysctrl.h
 */

/**
 * @brief set pin function mode
 *
 * @param[in] pin_number    Specifies the pin number.
 *                                                  GPIO0~GPIO30
 * @param[in] mode          The specail function mode for the pin_number
 *                                              Config GPIO To --> UART/I2S/PWM/SADC/I2C/SPI...
 * @return
 *         NONE
 * @details
 *         each pin has different function pin setting, please read RT58x datasheet to know each pin
 *   function usage setting.
 */
void Pin_Set_Mode_Ex(uint32_t pin_number, uint32_t mode);

/**
 * @brief   Generate a true 32-bits random number.
 *
 * @return  a 32-bits random number
 *
 * @details
 *       Generate a true 32bits random number.
 *       Please notice: this function is block mode, it will block about 4ms ~ 6ms.
 *       If you want to use non-block mode, maybe you should use interrupt mode.
 *
 */
extern uint32_t Get_Random_Number(void);
