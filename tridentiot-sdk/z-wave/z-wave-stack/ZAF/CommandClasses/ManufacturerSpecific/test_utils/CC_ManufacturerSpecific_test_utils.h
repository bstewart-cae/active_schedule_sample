/**
 * @file CC_ManufacturerSpecific_test_utils.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZAF_COMMANDCLASSES_MANUFACTURERSPECIFIC_TEST_UTILS_CC_MANUFACTURERSPECIFIC_TEST_UTILS_H_
#define ZAF_COMMANDCLASSES_MANUFACTURERSPECIFIC_TEST_UTILS_CC_MANUFACTURERSPECIFIC_TEST_UTILS_H_

extern "C" {
#include <test_common.h>
}

#if 0
class ICCTester
{
public:
  void RegisterHandler(received_frame_status_t(*pHandler)(RECEIVE_OPTIONS_TYPE_EX *,ZW_APPLICATION_TX_BUFFER *, uint8_t));
protected:
  command_handler_input_t chi;
};

class CCManufacturerSpecificTester : ICCTester
{
public:
  void CreateDeviceSpecificGetFrame(void);
  void InvokeHandler(void);
};
#endif

command_handler_input_t * manufacturer_specific_get_frame_create(void);
command_handler_input_t * device_specific_get_frame_create(const uint8_t device_id_type);

#endif /* ZAF_COMMANDCLASSES_MANUFACTURERSPECIFIC_TEST_UTILS_CC_MANUFACTURERSPECIFIC_TEST_UTILS_H_ */
