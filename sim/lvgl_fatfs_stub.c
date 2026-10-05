/** Desktop preview stub for the embedded FatFs delete operation. */
#ifndef STM32H723xx

#include "lvgl_fatfs.h"

bool lvgl_fatfs_remove(const char *path)
{
    (void)path;
    return false;
}

#endif
