/*****************************************************************************
 *
 * MODULE:          JN-AN-1189
 *
 * COMPONENT:       app_sensor_state_machine.c
 *
 * DESCRIPTION:     ZHA Demo: Light Sensor Key Press Behaviour (Implementation)
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
#include "App_LightSensor.h"
#include "TSL2550.h"
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
uint8 u8PIROcccupancySample = 0;
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
         case COMM_BUTTON_PRESSED:
              DBG_vPrintf(TRACE_SENSOR_STATE,"eEZ_FindAndBind \n");
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
 * NAME: APP_LightSensorTask
 *
 * DESCRIPTION:
 * Sampling Task for the light sensor
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_LightSensorTask)
{
    bool_t bStatus;
    uint16 u16ALSResult;

    bStatus = bTSL2550_StartRead(TSL2550_CHANNEL_0);
    DBG_vPrintf(TRACE_SENSOR_STATE,"\nStart Read Status =%d",bStatus);

    u16ALSResult = u16TSL2550_ReadResult();
    DBG_vPrintf(TRACE_SENSOR_STATE,"\nResult = %d",u16ALSResult);

    if(u16ALSResult > (LIGHT_SENSOR_MINIMUM_MEASURED_VALUE - 1))
        sSensor.sIlluminanceMeasurementServerCluster.u16MeasuredValue = u16ALSResult;
    else
        sSensor.sIlluminanceMeasurementServerCluster.u16MeasuredValue = LIGHT_SENSOR_MINIMUM_MEASURED_VALUE;

    /* Start sample timer so that you keep on sampling if KEEPALIVE_TIME is too high*/
    OS_eContinueSWTimer(APP_LightSensorSampleTimer,APP_TIME_MS(1000 * LIGHT_SENSOR_SAMPLING_TIME_IN_SECONDS),NULL);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
