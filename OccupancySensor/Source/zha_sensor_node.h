/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          zha_sensor_node.c
 *
 * DESCRIPTION:        ZHA Demo : Stack <-> Occupancy Sensor App Interaction
 *                     (Interface)
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

#ifndef APP_SENSOR_NODE_H_
#define APP_SENSOR_NODE_H_

#include "app_common.h"
#include "zcl_customcommand.h"
#ifdef PDM_EEPROM
 #include "app_pdm.h"
#endif
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define APP_LONG_SLEEP_DURATION_IN_SEC 60
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void APP_vInitialiseNode(void);
PUBLIC void vStartTimer(OS_thSWTimer hSwTimer,uint32 u32Time);
PUBLIC void vStopTimer(OS_thSWTimer hSwTimer);
#ifdef SLEEP_ENABLE
    PUBLIC void vUpdateKeepAliveTimer(void);
    PUBLIC void vLoadKeepAliveTime(uint8 u8TimeInSec);
    PUBLIC bool bWatingToSleep(void);
    PUBLIC void vWakeCallBack(void);
    PUBLIC uint32 u32CalibrateWakeTimer(uint8 u8CountInSeconds);
#endif

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern uint8 u8SleepCount;
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_SENSOR_NDOE_H_*/
