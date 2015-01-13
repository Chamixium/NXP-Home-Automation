/*****************************************************************************
 *
 * MODULE:          JN-AN-1189
 *
 * COMPONENT:       app_sensor_state_machine.c
 *
 * DESCRIPTION:     ZHA Demo: Occupancy Sensor Key Press Behaviour (Implementation)
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
#include "dbg.h"
#include "app_events.h"
#include "app_buttons.h"
#include "zha_sensor_node.h"
#include "haEzFindAndBind.h"
#include "app_sensor_state_machine.h"
#include "os_gen.h"
#include "App_OccupancySensor.h"
#include "GenericBoard.h"
#include "AppHardwareApi.h"
#include "pwrm.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_SENSOR_STATE
    #define TRACE_SENSOR_STATE   TRUE
#else
    #define TRACE_SENSOR_STATE   FALSE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PUBLIC void vSensorStateMachine(te_TransitionCode eTransitionCode );
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern const uint8 u8MyEndpoint;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
uint8 u8PIRUnoccupiedToOccupiedCount = 0;
bool bSampleEvent = FALSE;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vSensorStateMachine
 *
 * DESCRIPTION:
 * The control state machine called form the button handler function.
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSensorStateMachine(te_TransitionCode eTransitionCode )
{
    DBG_vPrintf(TRACE_SENSOR_STATE,"\nIn vSensorStateMachine TransitionCode = %d -> ",eTransitionCode);
    switch(eTransitionCode)
    {
        /* Fall through for the button presses as there will be a delayed action*/
         case SW1_PRESSED:
         break;

         case COMM_BUTTON_PRESSED:
              DBG_vPrintf(TRACE_SENSOR_STATE,"eEZ_FindAndBind \n");
              /* Keep Alive for 6 secomds and Poll & tick timer */
              vLoadKeepAliveTime(KEEP_ALIVETIME);
              vWakeCallBack();
              /* Exclude These Clusters */
              eEZ_ExcludeClusterFromEZBinding(GENERAL_CLUSTER_ID_BASIC,TRUE);
              eEZ_ExcludeClusterFromEZBinding(GENERAL_CLUSTER_ID_BASIC,FALSE);
              eEZ_ExcludeClusterFromEZBinding(GENERAL_CLUSTER_ID_GROUPS,FALSE);
              eEZ_ExcludeClusterFromEZBinding(GENERAL_CLUSTER_ID_IDENTIFY,TRUE);
              eEZ_ExcludeClusterFromEZBinding(GENERAL_CLUSTER_ID_IDENTIFY,FALSE);
              vAPP_ZCL_DeviceSpecific_SetIdentifyTime(0xFF);
              eEZ_FindAndBind(u8MyEndpoint,E_EZ_INITIATOR);
          break;

          case COMM_BUTTON_RELEASED:
              DBG_vPrintf(TRACE_SENSOR_STATE," Exit Easy Mode \n");
              vAPP_ZCL_DeviceSpecific_IdentifyOff();
              vEZ_Exit(u8MyEndpoint);
          break;

          case SW1_RELEASED:
              DBG_vPrintf(TRACE_SENSOR_STATE," Start Transition to  Occupied State with PIR %d\n",u8PIRUnoccupiedToOccupiedCount);
              if(sSensor.sOccupancySensingServerCluster.u8Occupancy == 0x00)
              {
            	  if(u8PIRUnoccupiedToOccupiedCount == 0)
            	  {
            	        vStartWakeTimer(sSensor.sOccupancySensingServerCluster.u8PIRUnoccupiedToOccupiedDelay);
            	  }
				  u8PIRUnoccupiedToOccupiedCount++;
				  DBG_vPrintf(TRACE_SENSOR_STATE," Start Transition from  UnOccupied State with PIR %d \n",u8PIRUnoccupiedToOccupiedCount);
              }else
              {
            	  DBG_vPrintf(TRACE_SENSOR_STATE," Start Transition from  Occupied State to UnOccupied with PIR \n");
            	  vStartWakeTimer(sSensor.sOccupancySensingServerCluster.u16PIROccupiedToUnoccupiedDelay);
              }
          break;

          default :
              break;
    }
}

/****************************************************************************
 *
 * NAME: vApp_ProcessKeyCombination
 *
 * DESCRIPTION:
 * Interpretes the button press and calls the state machine.
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vApp_ProcessKeyCombination(APP_tsEvent sButton)
{
    te_TransitionCode eTransitionCode=NUMBER_OF_TRANSITION_CODE;
    DBG_vPrintf(TRACE_SENSOR_STATE, "\nButton Event = %d",sButton.eType);
    switch(sButton.eType)
    {
        case APP_E_EVENT_BUTTON_DOWN:
            DBG_vPrintf(TRACE_SENSOR_STATE, "\nButton Number= %d",sButton.uEvent.sButton.u8Button);
            DBG_vPrintf(TRACE_SENSOR_STATE, "\nDIO State    = %08x\n",sButton.uEvent.sButton.u32DIOState);

            eTransitionCode=sButton.uEvent.sButton.u8Button;

            DBG_vPrintf(TRACE_SENSOR_STATE, "\nTransition Code = %d\n",eTransitionCode);
            vSensorStateMachine(eTransitionCode);
            break;

        case APP_E_EVENT_BUTTON_UP:
            DBG_vPrintf(TRACE_SENSOR_STATE, "\nButton Number= %d",sButton.uEvent.sButton.u8Button);
            DBG_vPrintf(TRACE_SENSOR_STATE, "\nDIO State    = %08x\n",sButton.uEvent.sButton.u32DIOState);
            switch (sButton.uEvent.sButton.u8Button)
            {
                case 0:
                    eTransitionCode=COMM_BUTTON_RELEASED;
                    break;
                case 1:
                    eTransitionCode=SW4_RELEASED;
                    break;
                case 2:
                    eTransitionCode=SW1_RELEASED;
                    break;
                case 3:
                    eTransitionCode=SW2_RELEASED;
                    break;
                case 4:
                    eTransitionCode=SW3_RELEASED;
                    break;
                default :
                    break;
            }
            vSensorStateMachine(eTransitionCode);
            break;
        default :
            break;
    }
}

/****************************************************************************
 *
 * NAME: vAPP_HandlePIRStateTransition
 *
 * DESCRIPTION:
 * Function that handles the state transition
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vAPP_HandlePIRStateTransition(void)
{
	tsZCL_CallBackEvent sCallBackEvent;
    if(sSensor.sOccupancySensingServerCluster.u8Occupancy == E_CLD_OS_OCCUPIED)
    {
		DBG_vPrintf(TRACE_SENSOR_STATE,"\n UnOccupied State \n");
		sSensor.sOccupancySensingServerCluster.u8Occupancy = 0x00;
		vGenericLEDSetOutput(1, FALSE);
		/* Provide ZCL tick to send report out: Called once as HA_MIN_REPORT_INT is 1 second */
		sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
		vZCL_EventHandler(&sCallBackEvent);
		/* Stop wake timer 1 for report to reload the new counts */
		PWRM_vWakeInterruptCallback();
		u8SleepCount = (APP_LONG_SLEEP_DURATION_IN_SEC - 1);
    }else if(u8PIRUnoccupiedToOccupiedCount >= sSensor.sOccupancySensingServerCluster.u8PIRUnoccupiedToOccupiedThreshold)
	{
	  DBG_vPrintf(TRACE_SENSOR_STATE,"\n Occupied State \n");
	  sSensor.sOccupancySensingServerCluster.u8Occupancy = E_CLD_OS_OCCUPIED;
	  vGenericLEDSetOutput(1, TRUE);
	  /* Provide ZCL tick to send report out: Called once as HA_MIN_REPORT_INT is 1 second  */
	  sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
	  vZCL_EventHandler(&sCallBackEvent);
	  /* Stop wake timer 1 for report to reload the new counts */
	  PWRM_vWakeInterruptCallback();
	  u8SleepCount = (APP_LONG_SLEEP_DURATION_IN_SEC - 1);
	  /* Start Transition to Unoccupied state */
	  vStartWakeTimer(sSensor.sOccupancySensingServerCluster.u16PIROccupiedToUnoccupiedDelay);
	}
	  u8PIRUnoccupiedToOccupiedCount = 0;
}

/****************************************************************************
 *
 * NAME: vStartWakeTimer
 *
 * DESCRIPTION:
 * Starts wake timer 0
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vStartWakeTimer(uint16 u16Tick)
{
      uint32 u32Ticks = 0;
      uint64 u64AdjustedTicks = 0;
      /* Disable Wake timer interrupt to use for calibration */
      vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, FALSE);
      uint32 u32CalibrationValue = u32AHI_WakeTimerCalibrate();
      (void)u8AHI_WakeTimerFiredStatus();
      /* Enable timer to use for sleeping */
      vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);
      u64AdjustedTicks = u16Tick * 32600 ;
      u64AdjustedTicks = u64AdjustedTicks * 10000 / u32CalibrationValue;
	  if (u64AdjustedTicks > 0xffffffff) {
	  // overflowed os limit to max uint32
		  u32Ticks = 0xfffffff;
	  } else {
		  u32Ticks = (uint32)u64AdjustedTicks;
	  }
      DBG_vPrintf(TRACE_SENSOR_STATE, "WakeTimer : u32CalibrationValue = %d u32Ticks %08x \n", u32CalibrationValue,u32Ticks);
      vAHI_WakeTimerStop(E_AHI_WAKE_TIMER_0);
      vAHI_WakeTimerStart(E_AHI_WAKE_TIMER_0, u32Ticks);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
