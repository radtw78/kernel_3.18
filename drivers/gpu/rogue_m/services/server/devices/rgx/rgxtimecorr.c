/*************************************************************************/ /*!
@File
@Title          Device specific time correlation and calibration routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device specific time correlation and calibration routines
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "rgxtimecorr.h"
#include "rgxfwutils.h"

#if TSAI_DS5
    #include "streamline_annotate.h"
#endif
/******************************************************************************
 *
 * - A calibration period is started on power-on and after a DVFS transition,
 *   and it's closed before a power-off and before a DVFS transition
 *   (so power-on -> dfvs -> dvfs -> power-off , power on -> dvfs -> dvfs...,
 *   where each arrow is a calibration period)
 *
 * - The timers on the Host and on the FW are correlated at the beginning of
 *   each period together with the (possibly calibrated) current GPU frequency
 *
 * - If the frequency has not changed since the last power-off/on sequence or
 *   before/after a DVFS transition (-> the transition didn't really happen)
 *   then multiple consecutive periods are merged (the higher the numbers the
 *   better the accuracy in the computed clock speed)
 *
 * - Correlation and calibration are also done more or less periodically
 *   (using a best effort approach)
 *
 *****************************************************************************/
#if TSAI /* backport from 1.9 */
static IMG_UINT32 g_ui32ClockSource = RGXTIMECORR_CLOCK_SCHED; /* 1.5 DDK uses OSClockns64 */

static void _RGXGPUFreqCalibratePreClockSourceChange(IMG_HANDLE hDevHandle);
static void _RGXGPUFreqCalibratePostClockSourceChange(IMG_HANDLE hDevHandle);


static PVRSRV_ERROR __attribute__((unused)) _SetClock(const PVRSRV_DEVICE_NODE *psDeviceNode,
                              const void *psPrivate,
                              IMG_UINT32 ui32Value)
{
	static const IMG_CHAR *apszClocks[] = {
		"mono", "mono_raw", "sched"
	};
#if TSAI
	IMG_UINT32 oldvalue = g_ui32ClockSource;
	(void)oldvalue;
#endif
	if (ui32Value >= RGXTIMECORR_CLOCK_LAST)
	{
		PVR_DPF((PVR_DBG_ERROR, "Invalid clock source type (%u)", ui32Value));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	_RGXGPUFreqCalibratePreClockSourceChange((PVRSRV_DEVICE_NODE *) psDeviceNode);

	PVR_DPF((PVR_DBG_WARNING, "Setting time correlation clock from \"%s\" to \"%s\"",
			apszClocks[g_ui32ClockSource],
			apszClocks[ui32Value]));

	g_ui32ClockSource = ui32Value;

	_RGXGPUFreqCalibratePostClockSourceChange((PVRSRV_DEVICE_NODE *) psDeviceNode);

	PVR_UNREFERENCED_PARAMETER(psPrivate);
	PVR_UNREFERENCED_PARAMETER(apszClocks);

	return PVRSRV_OK;
}

static PVRSRV_ERROR __attribute__((unused)) _GetClock(const PVRSRV_DEVICE_NODE *psDeviceNode,
                              const void *psPrivate,
                              IMG_UINT32 *pui32Value)
{
	*pui32Value = g_ui32ClockSource;

	PVR_UNREFERENCED_PARAMETER(psPrivate);

	return PVRSRV_OK;
}

IMG_UINT64 RGXGPUFreqCalibrateClockns64(void)
{
	IMG_UINT64 ui64Clock;

	switch (g_ui32ClockSource) {
		case RGXTIMECORR_CLOCK_MONO:
			return ((void) OSClockMonotonicns64(&ui64Clock), ui64Clock);
		case RGXTIMECORR_CLOCK_MONO_RAW:
			return OSClockMonotonicRawns64();
		case RGXTIMECORR_CLOCK_SCHED:
			return OSClockns64();
		default:
			PVR_ASSERT(IMG_FALSE);
			return 0;
	}
}

IMG_UINT64 RGXGPUFreqCalibrateClockus64(void)
{
	IMG_UINT32 rem;
	return OSDivide64r64(RGXGPUFreqCalibrateClockns64(), 1000, &rem);
}

void RGXGetTimeCorrData(PVRSRV_DEVICE_NODE *psDeviceNode,
							RGXFWIF_TIME_CORR *psTimeCorrs,
							IMG_UINT32 ui32NumOut)
{
	PVRSRV_RGXDEV_INFO    *psDevInfo     = psDeviceNode->pvDevice;
	RGXFWIF_GPU_UTIL_FWCB *psGpuUtilFWCB = psDevInfo->psRGXFWIfGpuUtilFWCb;
	IMG_UINT32 ui32CurrentIndex = psGpuUtilFWCB->ui32TimeCorrSeqCount;

	while(ui32NumOut--)
	{
		*(psTimeCorrs++) = psGpuUtilFWCB->sTimeCorr[RGXFWIF_TIME_CORR_CURR_INDEX(ui32CurrentIndex)];
		ui32CurrentIndex--;
	}
}

#endif

static IMG_VOID _RGXMakeTimeCorrData(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_RGXDEV_INFO    *psDevInfo     = psDeviceNode->pvDevice;
	RGXFWIF_GPU_UTIL_FWCB *psGpuUtilFWCB = psDevInfo->psRGXFWIfGpuUtilFWCb;
	RGX_GPU_DVFS_TABLE    *psGpuDVFSTable = psDevInfo->psGpuDVFSTable;
	RGXFWIF_TIME_CORR     *psTimeCorr;
	IMG_UINT32            ui32NewSeqCount;
	IMG_UINT32            ui32CoreClockSpeed;
	IMG_UINT32            ui32Remainder;

	ui32CoreClockSpeed = psGpuDVFSTable->aui32DVFSClock[psGpuDVFSTable->ui32CurrentDVFSId];

	ui32NewSeqCount = psGpuUtilFWCB->ui32TimeCorrSeqCount + 1;
	psTimeCorr = &psGpuUtilFWCB->sTimeCorr[RGXFWIF_TIME_CORR_CURR_INDEX(ui32NewSeqCount)];

	psTimeCorr->ui64CRTimeStamp    = RGXReadHWTimerReg(psDevInfo);
	psTimeCorr->ui64OSTimeStamp    = OSClockns64();
	psTimeCorr->ui32CoreClockSpeed = ui32CoreClockSpeed;
	psTimeCorr->ui32CRDeltaToOSDeltaKNs =
	    RGXFWIF_GET_CRDELTA_TO_OSDELTA_K_NS(ui32CoreClockSpeed, ui32Remainder);

	/* Make sure the values are written to memory before updating the index of the current entry */
	OSWriteMemoryBarrier();

	/* Update the index of the current entry in the timer correlation array */
	psGpuUtilFWCB->ui32TimeCorrSeqCount = ui32NewSeqCount;

	PVR_DPF((PVR_DBG_MESSAGE,"RGXMakeTimeCorrData: Correlated OS timestamp %llu (ns) with CR timestamp %llu, GPU clock speed %uHz",
	         psTimeCorr->ui64OSTimeStamp, psTimeCorr->ui64CRTimeStamp, psTimeCorr->ui32CoreClockSpeed));
}


static IMG_VOID _RGXGPUFreqCalibrationPeriodStart(PVRSRV_DEVICE_NODE *psDeviceNode, RGX_GPU_DVFS_TABLE *psGpuDVFSTable)
{
	PVRSRV_RGXDEV_INFO *psDevInfo         = psDeviceNode->pvDevice;
	RGX_DATA           *psRGXData         = (RGX_DATA*)psDeviceNode->psDevConfig->hDevData;
	IMG_UINT32         ui32CoreClockSpeed = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;
	IMG_UINT32         ui32Index          = RGX_GPU_DVFS_GET_INDEX(ui32CoreClockSpeed);

	IMG_UINT64 ui64CRTimestamp = RGXReadHWTimerReg(psDevInfo);
	IMG_UINT64 ui64OSTimestamp = OSClockus64();

	psGpuDVFSTable->ui64CalibrationCRTimestamp = ui64CRTimestamp;
	psGpuDVFSTable->ui64CalibrationOSTimestamp = ui64OSTimestamp;

	/* Set the time needed to (re)calibrate the GPU frequency */
	if((psGpuDVFSTable->aui32DVFSClock[ui32Index] == 0) ||                /* We never met this frequency */
	   (psGpuDVFSTable->aui32DVFSClock[ui32Index] == ui32CoreClockSpeed)) /* We weren't able to calibrate this frequency previously */
	{
		psGpuDVFSTable->aui32DVFSClock[ui32Index] = ui32CoreClockSpeed;
		psGpuDVFSTable->ui32CalibrationPeriod     = RGX_GPU_DVFS_FIRST_CALIBRATION_TIME_US;

		PVR_DPF((PVR_DBG_MESSAGE, "RGXGPUFreqCalibrationStart: using uncalibrated GPU frequency %u", ui32CoreClockSpeed));
	}
	else if(psGpuDVFSTable->ui32CalibrationPeriod == RGX_GPU_DVFS_FIRST_CALIBRATION_TIME_US)
	{
		psGpuDVFSTable->ui32CalibrationPeriod = RGX_GPU_DVFS_TRANSITION_CALIBRATION_TIME_US;
	}
	else
	{
		psGpuDVFSTable->ui32CalibrationPeriod = RGX_GPU_DVFS_PERIODIC_CALIBRATION_TIME_US;
	}

	/* Update the index to the DVFS table */
	psGpuDVFSTable->ui32CurrentDVFSId = ui32Index;
}


static IMG_VOID _RGXGPUFreqCalibrationPeriodStop(PVRSRV_DEVICE_NODE *psDeviceNode, RGX_GPU_DVFS_TABLE *psGpuDVFSTable)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	IMG_UINT64 ui64CRTimestamp = RGXReadHWTimerReg(psDevInfo);
	IMG_UINT64 ui64OSTimestamp = OSClockus64();

	if(!psGpuDVFSTable->bAccumulatePeriod)
	{
		psGpuDVFSTable->ui64CalibrationCRTimediff = 0;
		psGpuDVFSTable->ui64CalibrationOSTimediff = 0;
	}

	psGpuDVFSTable->ui64CalibrationCRTimediff += ui64CRTimestamp - psGpuDVFSTable->ui64CalibrationCRTimestamp;
	psGpuDVFSTable->ui64CalibrationOSTimediff += ui64OSTimestamp - psGpuDVFSTable->ui64CalibrationOSTimestamp;
}


static IMG_UINT32 _RGXGPUFreqCalibrationCalculate(PVRSRV_DEVICE_NODE *psDeviceNode, RGX_GPU_DVFS_TABLE *psGpuDVFSTable)
{
	IMG_UINT32 ui32CalibratedClockSpeed;
	IMG_UINT32 ui32Remainder;

	ui32CalibratedClockSpeed =
	    RGXFWIF_GET_GPU_CLOCK_FREQUENCY_HZ(psGpuDVFSTable->ui64CalibrationCRTimediff, psGpuDVFSTable->ui64CalibrationOSTimediff, ui32Remainder);

	PVR_DPF((PVR_DBG_MESSAGE, "GPU frequency calibration: %u -> %u done over %llu us",
	         psGpuDVFSTable->aui32DVFSClock[psGpuDVFSTable->ui32CurrentDVFSId],
	         ui32CalibratedClockSpeed,
	         psGpuDVFSTable->ui64CalibrationOSTimediff));

	psGpuDVFSTable->aui32DVFSClock[psGpuDVFSTable->ui32CurrentDVFSId] = ui32CalibratedClockSpeed;

	return ui32CalibratedClockSpeed;
}


#if TSAI /* backport 1.9 */
static void _RGXGPUFreqCalibratePreClockSourceChange(IMG_HANDLE hDevHandle)
{
	PVRSRV_DEVICE_NODE  *psDeviceNode   = hDevHandle;
	PVRSRV_RGXDEV_INFO  *psDevInfo      = psDeviceNode->pvDevice;
	RGX_GPU_DVFS_TABLE  *psGpuDVFSTable = psDevInfo->psGpuDVFSTable;
	//PVRSRV_VZ_RETN_IF_MODE(DRIVER_MODE_GUEST);

	_RGXGPUFreqCalibrationPeriodStop(psDeviceNode, psGpuDVFSTable);

	if (psGpuDVFSTable->ui64CalibrationOSTimediff >= psGpuDVFSTable->ui32CalibrationPeriod)
	{
		_RGXGPUFreqCalibrationCalculate(psDeviceNode, psGpuDVFSTable);
	}
}


static void _RGXGPUFreqCalibratePostClockSourceChange(IMG_HANDLE hDevHandle)
{
	PVRSRV_DEVICE_NODE  *psDeviceNode      = hDevHandle;
	PVRSRV_RGXDEV_INFO  *psDevInfo         = psDeviceNode->pvDevice;
	RGX_GPU_DVFS_TABLE  *psGpuDVFSTable    = psDevInfo->psGpuDVFSTable;
	//PVRSRV_VZ_RETN_IF_MODE(DRIVER_MODE_GUEST);

	/* Frequency has not changed, accumulate the time diffs to get a better result */
	psGpuDVFSTable->bAccumulatePeriod = IMG_TRUE;

	_RGXGPUFreqCalibrationPeriodStart(psDeviceNode, psGpuDVFSTable);

	/* Update the timer correlation data */
	_RGXMakeTimeCorrData(psDeviceNode/*, IMG_TRUE*/);
}


#endif
/*
	RGXGPUFreqCalibratePrePowerState
*/
IMG_VOID RGXGPUFreqCalibratePrePowerState(IMG_HANDLE hDevHandle)
{
	PVRSRV_DEVICE_NODE  *psDeviceNode   = hDevHandle;
	PVRSRV_RGXDEV_INFO  *psDevInfo      = psDeviceNode->pvDevice;
	RGX_GPU_DVFS_TABLE  *psGpuDVFSTable = psDevInfo->psGpuDVFSTable;

	_RGXGPUFreqCalibrationPeriodStop(psDeviceNode, psGpuDVFSTable);

	if(psGpuDVFSTable->ui64CalibrationOSTimediff >= psGpuDVFSTable->ui32CalibrationPeriod)
	{
		_RGXGPUFreqCalibrationCalculate(psDeviceNode, psGpuDVFSTable);
	}
}


/*
	RGXGPUFreqCalibratePostPowerState
*/
IMG_VOID RGXGPUFreqCalibratePostPowerState(IMG_HANDLE hDevHandle)
{
	PVRSRV_DEVICE_NODE  *psDeviceNode      = hDevHandle;
	PVRSRV_RGXDEV_INFO  *psDevInfo         = psDeviceNode->pvDevice;
	RGX_GPU_DVFS_TABLE  *psGpuDVFSTable    = psDevInfo->psGpuDVFSTable;
	RGX_DATA            *psRGXData         = (RGX_DATA*)psDeviceNode->psDevConfig->hDevData;
	IMG_UINT32          ui32CoreClockSpeed = psRGXData->psRGXTimingInfo->ui32CoreClockSpeed;

	/* If the frequency hasn't changed then accumulate the time diffs to get a better result */
	psGpuDVFSTable->bAccumulatePeriod = (RGX_GPU_DVFS_GET_INDEX(ui32CoreClockSpeed) == psGpuDVFSTable->ui32CurrentDVFSId);

	_RGXGPUFreqCalibrationPeriodStart(psDeviceNode, psGpuDVFSTable);

	/* Update the timer correlation data */
	_RGXMakeTimeCorrData(psDeviceNode);
}


/*
	RGXGPUFreqCalibratePreClockSpeedChange
*/
IMG_VOID RGXGPUFreqCalibratePreClockSpeedChange(IMG_HANDLE hDevHandle)
{
	PVRSRV_DEVICE_NODE  *psDeviceNode   = hDevHandle;
	PVRSRV_RGXDEV_INFO  *psDevInfo      = psDeviceNode->pvDevice;
	RGX_GPU_DVFS_TABLE  *psGpuDVFSTable = psDevInfo->psGpuDVFSTable;

	_RGXGPUFreqCalibrationPeriodStop(psDeviceNode, psGpuDVFSTable);

	/* Wait until RGXPostClockSpeedChange() to do anything as the GPU frequency may be left
	 * unchanged (in that case we delay calibration/correlation to get a better result later) */
}


/*
	RGXGPUFreqCalibratePostClockSpeedChange
*/
IMG_UINT32 RGXGPUFreqCalibratePostClockSpeedChange(IMG_HANDLE hDevHandle, IMG_UINT32 ui32NewClockSpeed)
{
	PVRSRV_DEVICE_NODE  *psDeviceNode          = hDevHandle;
	PVRSRV_RGXDEV_INFO  *psDevInfo             = psDeviceNode->pvDevice;
	RGX_GPU_DVFS_TABLE  *psGpuDVFSTable        = psDevInfo->psGpuDVFSTable;
	IMG_UINT32          ui32ReturnedClockSpeed = ui32NewClockSpeed;

	if(RGX_GPU_DVFS_GET_INDEX(ui32NewClockSpeed) != psGpuDVFSTable->ui32CurrentDVFSId)
	{
		/* Only calibrate if the last period was long enough */
		if(psGpuDVFSTable->ui64CalibrationOSTimediff >= RGX_GPU_DVFS_TRANSITION_CALIBRATION_TIME_US)
		{
			ui32ReturnedClockSpeed = _RGXGPUFreqCalibrationCalculate(psDeviceNode, psGpuDVFSTable);
		}

		_RGXGPUFreqCalibrationPeriodStart(psDeviceNode, psGpuDVFSTable);

		/* Update the timer correlation data */
		_RGXMakeTimeCorrData(psDeviceNode);

		psGpuDVFSTable->bAccumulatePeriod = IMG_FALSE;
	}
	else
	{
		psGpuDVFSTable->bAccumulatePeriod = IMG_TRUE;
	}

	return ui32ReturnedClockSpeed;
}


/*
	RGXGPUFreqCalibrateCorrelatePeriodic
*/
IMG_VOID RGXGPUFreqCalibrateCorrelatePeriodic(IMG_HANDLE hDevHandle)
{
	PVRSRV_DEVICE_NODE     *psDeviceNode   = hDevHandle;
	PVRSRV_RGXDEV_INFO     *psDevInfo      = psDeviceNode->pvDevice;
	RGX_GPU_DVFS_TABLE     *psGpuDVFSTable = psDevInfo->psGpuDVFSTable;
	PVRSRV_DATA            *psPVRSRVData;
	IMG_UINT64             ui64TimeNow     = OSClockus64();
	PVRSRV_DEV_POWER_STATE ePowerState;

	/* Check if it's the right time to recalibrate the GPU clock frequency */
	if((ui64TimeNow - psGpuDVFSTable->ui64CalibrationOSTimestamp) < psGpuDVFSTable->ui32CalibrationPeriod) return;

	/* Try to acquire the powerlock, if not possible then don't wait */
	psPVRSRVData     = PVRSRVGetPVRSRVData();
	if (OSLockIsLocked(psPVRSRVData->hPowerLock)) return; /* Better to not wait here if possible */
	/* There's a chance that the powerlock could be taken here, it's not that bad even if not desirable
	   (TODO use OSTryLockAcquire, currently implemented under Linux only) */
	if (PVRSRVPowerLock() != PVRSRV_OK) return;

	/* If the GPU is off then we can't do anything */
	PVRSRVGetDevicePowerState(psDeviceNode->sDevId.ui32DeviceIndex, &ePowerState);
	if (ePowerState != PVRSRV_DEV_POWER_STATE_ON)
	{
		PVRSRVPowerUnlock();
		return;
	}

	/* All checks passed, we can calibrate and correlate */
	_RGXGPUFreqCalibrationPeriodStop(psDeviceNode, psGpuDVFSTable);
	_RGXGPUFreqCalibrationCalculate(psDeviceNode, psGpuDVFSTable);
	_RGXGPUFreqCalibrationPeriodStart(psDeviceNode, psGpuDVFSTable);
	_RGXMakeTimeCorrData(psDeviceNode);

	/* Force Accumulate Period to false to not trigger a periodic calibration over and over again */
	psGpuDVFSTable->bAccumulatePeriod = IMG_FALSE;

	PVRSRVPowerUnlock();
}


/******************************************************************************
 End of file (rgxtimecorr.c)
******************************************************************************/
