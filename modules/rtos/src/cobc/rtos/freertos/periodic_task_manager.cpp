/*
 * Copyright (c) 2013, German Aerospace Center (DLR)
 * All Rights Reserved.
 *
 * See the file "LICENSE" for the full license governing this code.
 */

#include "periodic_task_manager.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../mutex.h"
#include "traits.h"

using namespace cobc::rtos;

PeriodicTaskManager::PeriodicTaskManager() :
	mutex(),
	timerRunning(false),
	lastWakeTime(),
	currentPeriod()
{
}

PeriodicTaskManager::~PeriodicTaskManager()
{
}

PeriodicTaskManager::StatusT
PeriodicTaskManager::nextPeriod(time::Duration period)
{
	MutexGuard lock(mutex);
	StatusT status = Status::running;

	const portTickType nextPeriod = (period.milliseconds() * configTICK_RATE_HZ) / 1000;
	if (timerRunning) {
		if (static_cast<Traits<portTickType>::SignedType>(xTaskGetTickCount() -
				lastWakeTime) > currentPeriod) {
			status = Status::timeout;
		}

		vTaskDelayUntil(&lastWakeTime, currentPeriod);
	}
	else {
		// period is started now, no need to wait
		lastWakeTime = xTaskGetTickCount();
		timerRunning = true;
	}

	currentPeriod = nextPeriod;
	return status;
}

PeriodicTaskManager::StatusT
PeriodicTaskManager::status()
{
	MutexGuard lock(mutex);

	if (!timerRunning) {
		return Status::idle;
	}
	else if (static_cast<Traits<portTickType>::SignedType>(xTaskGetTickCount() -
			lastWakeTime) > currentPeriod) {
		return Status::timeout;
	}
	else {
		return Status::running;
	}
}

void
PeriodicTaskManager::cancel()
{
	MutexGuard lock(mutex);
	timerRunning = false;
}