/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          zha_remote_node.c
 *
 * DESCRIPTION:        ZHA Demo : Stack <-> Occupancy Sensor App Interaction
 *                     (Implementation)
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <appapi.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "pwrm.h"
#include "zps_gen.h"
#include "rnd_pub.h"
#include "app_common.h"
#include "groups.h"
#include "PDM_IDs.h"
#include "app_timer_driver.h"
#include "zha_sensor_node.h"
#include "app_zcl_sensor_task.h"
#include "app_zbp_utilities.h"
#include "app_events.h"
#include "zcl_customcommand.h"
#include "app_buttons.h"
#include "GenericBoard.h"
#include "ha.h"
#include "haEzJoin.h"
#include "haEzFindAndBind.h"
#include "app_sensor_state_machine.h"
#include "zcl_common.h"
#include "app_reporting.h"
#include "haKeys.h"
#include "app_management_bind.h"
#include "PingParent.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_SENSOR_NODE
    #define DEBUG_SENSOR_NODE   TRUE
#else
    #define DEBUG_SENSOR_NODE   FALSE
#endif

#define bWakeUpFromSleep() bWatingToSleep()  /* For readability purpose */

#define ONE_SEC			 1
#define THREE_SEC		 3
#define TEN_SEC			 10

#define NUMBER_DEVICE_TO_BE_DISCOVERED 5
#define ZHA_MAX_REJOIN_ATTEMPTS 10

extern const uint8 u8MyEndpoint;



/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PUBLIC void vStopAllTimers(void);
PUBLIC void vStopTimer(OS_thSWTimer hSwTimer);
PRIVATE void APP_vInitLeds(void);
PRIVATE void vHandleAppEvent( APP_tsEvent sAppEvent );
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO);
PRIVATE void vHandleJoinAndRejoinNWK( ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction  );
PRIVATE void app_vRestartNode (void);
PRIVATE void app_vStartNodeFactoryNew(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

PUBLIC PDM_tsRecordDescriptor sDevicePDDesc;
PUBLIC  tsDeviceDesc           sDeviceDesc;
uint8 u8SleepCount = (APP_LONG_SLEEP_DURATION_IN_SEC - 1);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

#ifdef SLEEP_ENABLE
    PRIVATE bool bDataPending=FALSE;
    PRIVATE uint8 u8KeepAliveTime = KEEP_ALIVETIME;
    PRIVATE pwrm_tsWakeTimerEvent    sWake;
#endif

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vInitialiseNode
 *
 * DESCRIPTION:
 * Initialises the application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseNode(void)
{
    PDM_teStatus eStatusReportReload;
    DBG_vPrintf(DEBUG_SENSOR_NODE, "\nAPP_vInitialiseNode*");

    APP_vInitLeds();

    /*Initialise the application buttons*/
    /* Initialise buttons; if a button is held down as the device is reset, delete the device
     * context from flash
     */
    APP_bButtonInitialise();

    /*In case of a deep sleep device any button wake up would cause a PDM delete , only check for DIO8
     * pressed for deleting the context */
    vDeletePDMOnButtonPress(APP_BUTTONS_BUTTON_1);

    /* Restore any report data that is previously saved to flash */
    eStatusReportReload = eRestoreReports();

    /* Restore any application data previously saved to flash */
    PDM_eLoadRecord(&sDevicePDDesc, PDM_ID_APP_REMOTE_CONTROL, &sDeviceDesc,
            sizeof(tsDeviceDesc), FALSE);

    /* Initialise ZBPro stack */
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
    DBG_vPrintf(DEBUG_SENSOR_NODE, "Set Sec state\n");

    /* Resister the call back for the mgmt bind server */
    vAppRegisterManagementBindServer();
    vEZ_RestoreDefaultAIBChMask();
    /* Initialize ZBPro stack */
    ZPS_eAplAfInit();

    DBG_vPrintf(DEBUG_SENSOR_NODE, "ZPS_eAplAfInit\n");
    /*Set Save default channel mask as it is going to be manipulated */
    vEZ_SetDefaultAIBChMask();

    APP_ZCL_vInitialise();

    /*Load the reports from the PDM or the default ones depending on the PDM load record status*/
    if(eStatusReportReload !=PDM_E_STATUS_RECOVERED )
    {
        /*Load Defaults if the data was not correct*/
        vLoadDefaultConfigForReportable();
    }
    /*Make the reportable attributes */
    vMakeSupportedAttributesReportable();

    /* If the device state has been restored from flash, re-start the stack
     * and set the application running again.
     */
    if (sDeviceDesc.eNodeState == E_RUNNING)
    {
        app_vRestartNode();
    }
    else
    {
        app_vStartNodeFactoryNew();
    }

    #ifdef PDM_EEPROM
        vDisplayPDMUsage();
    #endif

    OS_eActivateTask(APP_ZHA_Sensor_Task);
}

/****************************************************************************
 *
 * NAME: APP_ZHA_Sensor_Task
 *
 * DESCRIPTION:
 * Task that handles the application related functionality
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ZHA_Sensor_Task)
{
    APP_tsEvent sAppEvent;
    ZPS_tsAfEvent sStackEvent;
    sStackEvent.eType = ZPS_EVENT_NONE;
    sAppEvent.eType = APP_E_EVENT_NONE;
    /*Collect the application events*/
    if (OS_eCollectMessage(APP_msgEvents, &sAppEvent) == OS_E_OK)
    {

    }
    /*Collect stack Events */
    else if ( OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent) == OS_E_OK)
    {
        #ifdef DEBUG_SENSOR_NODE
            vDisplayStackEvent(sStackEvent);
        #endif

        /* Mgmt Bind server called from the application */
        vHandleManagementBindEvents(&sStackEvent);

        vPingRecv(&sStackEvent);
    }

    /* Handle events depending on node state */
    switch (sDeviceDesc.eNodeState)
    {
        case E_STARTUP:
        	vHandleJoinAndRejoinNWK(&sStackEvent,E_EZ_JOIN);
            break;

        case E_REJOINING:
        	vHandleJoinAndRejoinNWK(&sStackEvent,E_EZ_REJOIN);
        	DBG_vPrintf(DEBUG_SENSOR_NODE, "In E_REJOIN - Kick off Tick Timer \n");
        	OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
        	vHandleAppEvent( sAppEvent );
        	break;

        case E_RUNNING:
            DBG_vPrintf(DEBUG_SENSOR_NODE, "E_RUNNING\r\n");
            if (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN)
            {
                DBG_vPrintf(DEBUG_SENSOR_NODE, "ZPS_EVENT_NWK_FAILED_TO_JOIN\n");
                vStopAllTimers();
                OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
                vStartStopTimer( APP_JoinTimer, APP_TIME_MS(1000),(uint8*)&(sDeviceDesc.eNodeState),E_REJOINING );
                DBG_vPrintf(DEBUG_SENSOR_NODE, "failed join running %02x\n",sStackEvent.uEvent.sNwkJoinFailedEvent.u8Status );
            }
            /* Mgmt Leave Received */
            else if(  ZPS_EVENT_NWK_LEAVE_INDICATION == sStackEvent.eType )
            {
                if( sStackEvent.uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0 )
                {
                    DBG_vPrintf(DEBUG_SENSOR_NODE, "ZDO Leave\n" );
    				PDM_vDelete();
    				vAHI_SwReset();
                }
            }
            #ifdef SLEEP_ENABLE
                else if (ZPS_EVENT_NWK_POLL_CONFIRM == sStackEvent.eType)
                {
                    if (MAC_ENUM_SUCCESS == sStackEvent.uEvent.sNwkPollConfirmEvent.u8Status)
                    {
                        bDataPending = TRUE;
                    }
                    else if (MAC_ENUM_NO_DATA == sStackEvent.uEvent.sNwkPollConfirmEvent.u8Status)
                    {
                        bDataPending = FALSE;
                    }
                }
            #endif
            vHandleAppEvent( sAppEvent );
            vEZ_EZModeNWKFindAndBindHandler(&sStackEvent);
            /* Update status of LED D2 for Occupancy  if in running state */
           	vGenericLEDSetOutput(1,sSensor.sOccupancySensingServerCluster.u8Occupancy);
            break;
        default:
            break;
    }

    /*
     * Global clean up to make sure any PDUs have been freed
     */
    if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
}

/****************************************************************************
 *
 * NAME: vHandleJoinAndRejoinNWK
 *
 * DESCRIPTION:
 * Handles the Start UP events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleJoinAndRejoinNWK( ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction  )
{
    teEZ_State ezState;
    /*Call The EZ mode Handler passing the events*/
    vEZ_EZModeNWKJoinHandler(pZPSevent,eJoinAction);
    ezState = eEZ_GetJoinState();
    DBG_vPrintf(DEBUG_SENSOR_NODE, "EZ_STATE\%x r\n", ezState);
    if(ezState == E_EZ_DEVICE_IN_NETWORK)
    {
        DBG_vPrintf(DEBUG_SENSOR_NODE, "HA EZMode EVT: E_EZ_DEVICE_IN_NETWORK \n");
        vStartStopTimer( APP_JoinTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_RUNNING );
        PDM_vSaveRecord( &sDevicePDDesc);
        PDM_vSave();
        /* Start 1 seconds polling */
        OS_eStartSWTimer(APP_PollTimer, POLL_TIME, NULL);
        OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
    }
}
#ifdef SLEEP_ENABLE
/****************************************************************************
 *
 * NAME: vLoadKeepAliveTime
 *
 * DESCRIPTION:
 * Loads the keep alive timer based on the right conditions.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadKeepAliveTime(uint8 u8TimeInSec)
{
    u8KeepAliveTime=u8TimeInSec;
    DBG_vPrintf(DEBUG_SENSOR_NODE,"In Fn vLoadKeepAliveTime--- Dev State = %d\n",sDeviceDesc.eNodeState);
    OS_eStartSWTimer(APP_PollTimer,POLL_TIME, NULL);
    OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
    if(sDeviceDesc.eNodeState == E_REJOINING)
    {
    	vStartStopTimer( APP_JoinTimer, APP_TIME_MS(1000),(uint8*)&(sDeviceDesc.eNodeState),E_REJOINING );
    	DBG_vPrintf(DEBUG_SENSOR_NODE,"Loaded the Keep Alive Timer\n");
    }
}

/****************************************************************************
 *
 * NAME: bWatingToSleep
 *
 * DESCRIPTION:
 * Gets the status if the module is waiting for sleep.
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC bool bWatingToSleep(void)
{
    if (0 == u8KeepAliveTime)
        return TRUE;
    else
        return FALSE;
}

/****************************************************************************
 *
 * NAME: vUpdateKeepAliveTimer
 *
 * DESCRIPTION:
 * Updates the Keep Alive time at 1 sec call from the tick timer that served ZCL as well.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vUpdateKeepAliveTimer(void)
{

	DBG_vPrintf(DEBUG_SENSOR_NODE, "u8KeepAliveTime %d u8SleepCount %d\n",u8KeepAliveTime,u8SleepCount);
    if( u8KeepAliveTime > 0 )
    {
        u8KeepAliveTime--;
        if(u8SleepCount > 0)
        {
        	u8SleepCount--;
        	DBG_vPrintf(DEBUG_SENSOR_NODE, "u8SleepCount %d\n",u8SleepCount);
        }else
        {
        	u8SleepCount = (APP_LONG_SLEEP_DURATION_IN_SEC - 1);
        }
    }
    else
    {
		vStopAllTimers();
		DBG_vPrintf(DEBUG_SENSOR_NODE, "PWRM_u16GetActivityCount %d with sleep count %d\n",PWRM_u16GetActivityCount(),u8SleepCount);
		PWRM_teStatus eStatus=PWRM_eScheduleActivity(&sWake, u8SleepCount*32000 , vWakeCallBack);
		DBG_vPrintf(DEBUG_SENSOR_NODE, "PWRM_u16GetActivityCount status = %d\n",eStatus);
    }

}
/****************************************************************************
 *
 * NAME: vWakeCallBack
 *
 * DESCRIPTION:
 * Wake up call back called upon wake up by the schedule activity event.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vWakeCallBack(void)
{
	DBG_vPrintf(DEBUG_SENSOR_NODE, "\nvWakeCallBack\n");

	/*Start Polling*/
    if(OS_eGetSWTimerStatus(APP_PollTimer) != OS_E_SWTIMER_RUNNING )
    	OS_eStartSWTimer(APP_PollTimer,POLL_TIME, NULL);

    /*Start the APP_TickTimer to continue the ZCL tasks */
    if(OS_eGetSWTimerStatus(APP_TickTimer) != OS_E_SWTIMER_RUNNING )
    	OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);

}

/****************************************************************************
 *
 * NAME: vManageWakeUponSysControlISR
 *
 * DESCRIPTION:
 * Called from SysControl ISR to process the wake up conditions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vManageWakeUponSysControlISR(teInterruptType eInterruptType)
{
	int Count = 0;
	tsZCL_CallBackEvent sCallBackEvent;

	/*In any case this could be a wake up from timer interrupt or from buttons
	 * press
	 * */

	if(TRUE == bWakeUpFromSleep())
	{
		/*Only called if the module is coming out of sleep */
		 if(eInterruptType == E_INTERRUPT_WAKE_TIMER_EXPIRY)
		 {
			if(sDeviceDesc.eNodeState == E_REJOINING)
			{
				/* for rejoining keep alive for 10 seconds */
				vLoadKeepAliveTime(TEN_SEC);
			}
			else
			{
				/* detect parent loss with poll tries */
				vLoadKeepAliveTime(THREE_SEC);
				/* Provide the sleep ticks to ZCL to trigger report */
				for(Count = 0; Count <u8SleepCount ; Count++)
				{
					sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
					vZCL_EventHandler(&sCallBackEvent);
				}
			}
			u8SleepCount = (APP_LONG_SLEEP_DURATION_IN_SEC - 1);
			vWakeCallBack();
		    /* Activate the SleepTask, that would start the SW timer and polling would continue
		     * */
		    OS_eActivateTask(APP_SleepTask);
		}
	}
	DBG_vPrintf(DEBUG_SENSOR_NODE,"vISR_SystemController on WakeUP \n\n");
}
#endif

/****************************************************************************
 *
 * NAME: vDeletePDMOnButtonPress
 *
 * DESCRIPTION:
 * PDM context clearing on button press
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO)
{
    bool_t bDeleteRecords = FALSE;
    uint32 u32Buttons = u32AHI_DioReadInput() & (1 << u8ButtonDIO);
    if (u32Buttons == 0)
    {
        bDeleteRecords = TRUE;
    }
    else
    {
        bDeleteRecords = FALSE;
    }
    /* If required, at this point delete the network context from flash, perhaps upon some condition
     * For example, check if a button is being held down at reset, and if so request the Persistent
     * Data Manager to delete all its records:
     * e.g. bDeleteRecords = vCheckButtons();
     * Alternatively, always call PDM_vDelete() if context saving is not required.
     */
    if(bDeleteRecords)
    {
        DBG_vPrintf(DEBUG_SENSOR_NODE,"Deleting the PDM\n");
        PDM_vDelete();
    }
}

/****************************************************************************
 *
 * NAME: vHandleAppEvent
 *
 * DESCRIPTION:
 * Function to handle the app event - buttons
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleAppEvent( APP_tsEvent sAppEvent )
{
    switch(sAppEvent.eType)
    {
        case APP_E_EVENT_BUTTON_DOWN:
        case APP_E_EVENT_BUTTON_UP:
        	vApp_ProcessKeyCombination(sAppEvent);
			/*Reset the channel mask to last used so that the
			 * rejoining joining will be attempted
			 * */
			if(sDeviceDesc.eNodeState == E_REJOINING )
			{
				vEZ_ReJoinOnLastKnownCh();
			}
        break;
        default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: APP_vInitLeds
 *
 * DESCRIPTION:
 * Initialises LED's
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vInitLeds(void)
{
    vGenericLEDInit();
}

/****************************************************************************
 *
 * NAME: vStopAllTimers
 *
 * DESCRIPTION:
 * Stops all the timers
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStopAllTimers(void)
{
	vStopTimer(APP_PollTimer);
	vStopTimer(APP_TickTimer);
	vStopTimer(APP_JoinTimer);
	vStopTimer(APP_BackOffTimer);
	vStopTimer(App_EZFindAndBindTimer);
}

/****************************************************************************
 *
 * NAME: vStopTimer
 *
 * DESCRIPTION:
 * Stops the timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStopTimer(OS_thSWTimer hSwTimer)
{
	if(OS_eGetSWTimerStatus(hSwTimer) != OS_E_SWTIMER_STOPPED)
		OS_eStopSWTimer(hSwTimer);
}

/****************************************************************************
 *
 * NAME: vStartTimer
 *
 * DESCRIPTION:
 * Starts the timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartTimer(OS_thSWTimer hSwTimer,uint32 u32Time)
{
	if(OS_eGetSWTimerStatus(hSwTimer) != OS_E_SWTIMER_STOPPED )
		OS_eStopSWTimer(hSwTimer);
	OS_eStartSWTimer(hSwTimer, u32Time, NULL);
}

/****************************************************************************
 *
 * NAME: APP_PollTask
 *
 * DESCRIPTION:
 * Poll Task for the polling as well it triggers the rejoin in case of pool failure
 * It also manages sleep timing.
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_PollTask)
{
    ZPS_teStatus u8PStatus;

    OS_eStopSWTimer(APP_PollTimer);
    if(!bWatingToSleep())
    {
    	OS_eStartSWTimer(APP_PollTimer, POLL_TIME, NULL);

		u8PStatus = ZPS_eAplZdoPoll();
		if( u8PStatus )
		{
			DBG_vPrintf(DEBUG_SENSOR_NODE, "\nPoll Failed \n", u8PStatus );
		}
    }


}

/****************************************************************************
 *
 * NAME: vEZ_EZModeCb
 *
 * DESCRIPTION:
 * EZ Mode call back
 *
 * PARAMETERS:
 * tsEZ_FindAndBindEvent * psCallBackEvent  call back event.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vEZ_EZModeCb(tsEZ_FindAndBindEvent * psCallBackEvent)
{
    switch (psCallBackEvent->eEventType)
    {
        /*EZ Mode Events */

        case E_EZ_BIND_CREATED_FOR_TARGET:
        {
            uint8 u8Seq;
            tsZCL_Address sAddress;
            tsCLD_Identify_IdentifyRequestPayload sPayload;

            sPayload.u16IdentifyTime = 0;
            sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
            sAddress.uAddress.u16DestinationAddress = psCallBackEvent->u16TargetAddress;

            eCLD_IdentifyCommandIdentifyRequestSend(
									psCallBackEvent->u8InitiatorEp,
									psCallBackEvent->u8TargetEp,
                                    &sAddress,
                                    &u8Seq,
                                    &sPayload);
        	/*Make the target stop identifying that is just bound
             * This helps in adding next target */
            DBG_vPrintf(DEBUG_SENSOR_NODE, " APP : E_HA_EZ_BIND_CREATED_FOR_TARGET\n");


        }
            break;

        case E_EZ_GROUP_CREATED_FOR_TARGET:
        {
            uint8 u8Seq;
            tsZCL_Address sAddress;
            tsCLD_Identify_IdentifyRequestPayload sPayload;

            sPayload.u16IdentifyTime = 0;
            sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
            sAddress.uAddress.u16DestinationAddress = psCallBackEvent->u16TargetAddress;

            eCLD_IdentifyCommandIdentifyRequestSend(
									psCallBackEvent->u8InitiatorEp,
									psCallBackEvent->u8TargetEp,
                                    &sAddress,
                                    &u8Seq,
                                    &sPayload);
        	/*Make the target stop identifying that is just grouped
             * This helps in adding next target */
            DBG_vPrintf(DEBUG_SENSOR_NODE, " APP : E_EZ_GROUP_CREATED_FOR_TARGET\n");

        }
            break;

        case E_EZ_TIMEOUT:
            /**/
            break;
        default:
            DBG_vPrintf(DEBUG_SENSOR_NODE, "Invalid event type\r\n");
            break;
    }
}
/****************************************************************************
 *
 * NAME: app_vRestartNode
 *
 * DESCRIPTION:
 * Start the Restart the ZigBee Stack after a context restore from
 * the EEPROM/Flash
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vRestartNode (void)
{
    ZPS_tsNwkNib * thisNib;

    /* Get the NWK Handle to have the NWK address from local node and use it as GroupId*/
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    /* The node is in running state indicates that
     * the EZ Mode state is as E_EZ_DEVICE_IN_NETWORK*/
    eEZ_UpdateEZState(E_EZ_DEVICE_IN_NETWORK);

    DBG_vPrintf(DEBUG_SENSOR_NODE, "\nNon Factory New Start");

    PDM_vSave();

    /* Start 1 seconds polling */
    OS_eStartSWTimer(APP_PollTimer,POLL_TIME, NULL);
    /*Rejoin NWK when coming from reset.*/
	ZPS_eAplZdoRejoinNetwork();
}


/****************************************************************************
 *
 * NAME: app_vStartNodeFactoryNew
 *
 * DESCRIPTION:
 * Start the ZigBee Stack for the first ever Time.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vStartNodeFactoryNew(void)
{
    eEZ_UpdateEZState(E_EZ_START);

    /* Stay awake for joining */
    DBG_vPrintf(DEBUG_SENSOR_NODE, "\nFactory New Start");
    vStartStopTimer( APP_JoinTimer, APP_TIME_MS(1000),(uint8*)&(sDeviceDesc.eNodeState),E_STARTUP );
}

/****************************************************************************
 *
 * NAME: APP_SleepTask
 *
 * DESCRIPTION:
 * Os Task activated by the wake up event to manage sleep
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

OS_TASK(APP_SleepTask)
{
#ifdef SLEEP_ENABLE
    DBG_vPrintf(DEBUG_SENSOR_NODE, "SleepTask\n");
	vIncrementPingTime(61);
    if(bPingParent())
    {
		OS_eStartSWTimer(App_PingTimer,APP_TIME_MS(2000),NULL);
    }
#endif
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
