/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          App_OccupancySensor.c
 *
 * DESCRIPTION:        ZHA Demo Occupancy Sensor -Implementation
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
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "zps_gen.h"
#include "App_OccupancySensor.h"
#include "GenericBoard.h"
#include "dbg.h"
#include <string.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
const uint8 u8MyEndpoint = OCCUPANCYSENSOR_SENSOR_ENDPOINT;
tsHA_OccupancySensorDevice sSensor;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: eApp_HA_RegisterEndpoint
 *
 * DESCRIPTION:
 * Register ZHA endpoints
 *
 * PARAMETER
 * Type                        Name                  Descirption
 * tfpZCL_ZCLCallBackFunction  fptr                  Pointer to ZCL Callback function
 *
 * RETURNS:
 * teZCL_Status
 *
 ****************************************************************************/
teZCL_Status eApp_HA_RegisterEndpoint(tfpZCL_ZCLCallBackFunction fptr)
{
    return eHA_RegisterOccupancySensorEndPoint(OCCUPANCYSENSOR_SENSOR_ENDPOINT,
                                              fptr,
                                              &sSensor);
}

/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZCL Device Specific initialization
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
void vAPP_ZCL_DeviceSpecific_Init()
{
    /* Initialise the strings in Basic */
    memcpy(sSensor.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sSensor.sBasicServerCluster.au8ModelIdentifier, "ZHA-OccupancySensor", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sSensor.sBasicServerCluster.au8DateCode, "20140604", CLD_BAS_DATE_SIZE);
    memcpy(sSensor.sBasicServerCluster.au8SWBuildID, "4000-0001", CLD_BAS_SW_BUILD_SIZE);
    /* Initialise the strings in Occupancy Cluster */
    sSensor.sOccupancySensingServerCluster.eOccupancySensorType = E_CLD_OS_SENSORT_TYPE_PIR;
    sSensor.sOccupancySensingServerCluster.u8Occupancy = 0;
    sSensor.sOccupancySensingServerCluster.u8PIRUnoccupiedToOccupiedThreshold = OCCUPANCY_SENSOR_UNOCCUPIED_TO_OCCUPIED_THRESHOLD;
    sSensor.sOccupancySensingServerCluster.u8PIRUnoccupiedToOccupiedDelay = OCCUPANCY_SENSOR_UNOCCUPIED_TO_OCCUPIED_DELAY;
    sSensor.sOccupancySensingServerCluster.u16PIROccupiedToUnoccupiedDelay = OCCUPANCY_SENSOR_OCCUPIED_TO_UNOCCUPIED_DELAY;
}

/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_Identify
 *
 * DESCRIPTION:
 * ZCL Device Specific initialization
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vAPP_ZCL_DeviceSpecific_SetIdentifyTime(uint16 u16Time)
{
    sSensor.sIdentifyServerCluster.u16IdentifyTime=u16Time;
}
/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_UpdateIdentify
 *
 * DESCRIPTION:
 * ZCL Device Specific Identify Updates
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vAPP_ZCL_DeviceSpecific_UpdateIdentify(void)
{
    vGenericLEDSetOutput(2, sSensor.sIdentifyServerCluster.u16IdentifyTime%2);
}
/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_IdentifyOff
 *
 * DESCRIPTION:
 * ZCL Device Specific stop idnetify
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vAPP_ZCL_DeviceSpecific_IdentifyOff(void)
{
    vAPP_ZCL_DeviceSpecific_SetIdentifyTime(0);
    vGenericLEDSetOutput(2, 0);
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
