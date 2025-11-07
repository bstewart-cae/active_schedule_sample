/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Assert.h>
#include "zpal_storage_utils.h"

static const char *storage_path;

const char *zpal_storage_utils_get_base_path(void)
{
 return storage_path;
}

void zpal_storage_utils_set_base_path(const char *base_path)
{
  storage_path = base_path;
#if 0
  char *path = strdup(storage_path);
  char *part = path;
  char *slash;

  while ((slash = strchr(part, '/')) != 0)
  {
    if (slash != part)
    {
      *slash = '\0';
      if (access(path, F_OK) != 0)
      {
        const int result = mkdir(path, S_IRWXU);
        ASSERT(result == 0);
      }
      *slash = '/';
    }
    part = slash + 1;
  }
  if (access(path, F_OK) != 0)
  {
    const int result = mkdir(path, S_IRWXU);
    ASSERT(result == 0);
  }

  free(path);
#endif
}
