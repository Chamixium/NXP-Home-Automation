/*****************************************************************************
 *
 * MODULE:          JN-AN-1189
 *
 * COMPONENT:       app_sensor_state_machine.h
 *
 * DESCRIPTION:     ZHA Demo: Occupancy sensor Press Behaviour (Interface)
 *
 *****************************************************************************
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
 ****************************************************************************/
#ifndef APP_SENSOR_STATE_MACHINE_H_
#define APP_SENSOR_STATE_MACHINE_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "zcl.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define BUTTON_DELAY_TIME_IN_MS 250

#define SW1        (1 << APP_BUTTONS_BUTTON_SW3)
#define SW2        (1 << APP_BUTTONS_BUTTON_SW2 )
#define SW3        (1 << APP_BUTTONS_BUTTON_SW1 )
#define SW4        (1 << APP_BUTTONS_BUTTON_SW4 )
#define COMM       (1 << APP_BUTTONS_BUTTON_1)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/* Kindly Maintain the order as the button numbers are assigned directly */

typedef enum{
    COMM_BUTTON_PRESSED,
    SW4_PRESSED,
    SW1_PRESSED,
    SW2_PRESSED,
    SW3_PRESSED,
    COMM_BUTTON_RELEASED,
    SW1_RELEASED,
    SW2_RELEASED,
    SW3_RELEASED,
    SW4_RELEASED,
    NUMBER_OF_TRANSITION_CODE
}te_TransitionCode;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vApp_ProcessKeyCombination(APP_tsEvent sButton);
void vAPP_HandlePIRStateTransition(void);
void vStartWakeTimer(uint16 u16Tick);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#endif /* APP_SENSOR_STATE_MACHINE_H_ */
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
