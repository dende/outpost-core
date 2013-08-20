/*
 * Copyright (c) 2013, German Aerospace Center (DLR)
 * All Rights Reserved.
 *
 * See the file "LICENSE" for the full license governing this code.
 */

#ifndef COBC_RTOS_SEMAPHORE_HPP
#define COBC_RTOS_SEMAPHORE_HPP

#include "detect.h"

#if COBC_RTOS == COBC_RTOS_NONE
#	include "none/semaphore.h"
#elif COBC_RTOS == COBC_RTOS_RTEMS
#	include "rtems/semaphore.h"
#elif COBC_RTOS == COBC_RTOS_FREERTOS
#	include "freertos/semaphore.h"
#elif COBC_RTOS == COBC_RTOS_POSIX
#	include "posix/semaphore.h"
#endif

#endif // COBC_RTOS_SEMAPHORE_HPP
