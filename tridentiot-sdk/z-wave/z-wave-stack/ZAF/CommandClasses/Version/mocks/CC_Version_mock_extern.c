/**
 * @file CC_Version_mock_extern.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <ZW_classcmd.h>
#include <mock_control.h>
#include <string.h>

#define MOCK_FILE "CC_Version_mock_extern.c"

void CC_Version_GetFirmwareVersion_handler(
    uint8_t firmwareTargetIndex,
    VG_VERSION_REPORT_V2_VG* pVariantgroup)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, firmwareTargetIndex, pVariantgroup);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, firmwareTargetIndex);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pVariantgroup);

  memcpy((uint8_t *)pVariantgroup, (uint8_t *)pMock->output_arg[1].p, sizeof(VG_VERSION_REPORT_V2_VG));
}
