/****************************************************************************
 *
 *                 THIS IS A GENERATED FILE. DO NOT EDIT!
 *
 * MODULE:         OS
 *
 * COMPONENT:      os_gen.h
 *
 * DATE:           Mon Jan 12 23:19:09 2015
 *
 * AUTHOR:         Jennic RTOS Configuration Tool
 *
 * DESCRIPTION:    RTOS Application Configuration
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd. 2009 All rights reserved
 *
 ****************************************************************************/

#ifndef _OS_GEN_H
#define _OS_GEN_H

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define OS_STRICT_CHECKS

/* Module ZBPro */

/* Mutex Handles */
#define mutexZPS ((OS_thMutex)&os_Mutex_mutexZPS)
#define mutexPDUM ((OS_thMutex)&os_Mutex_mutexPDUM)
#define mutexMAC ((OS_thMutex)&os_Mutex_mutexMAC)
#define hSpiMutex ((OS_thMutex)&os_Mutex_hSpiMutex)
#define HA ((OS_thMutex)&os_Mutex_HA)

/* Message Handles */
#define zps_msgMlmeDcfmInd ((OS_thMessage)&os_Message_zps_msgMlmeDcfmInd)
#define zps_msgTimeEvents ((OS_thMessage)&os_Message_zps_msgTimeEvents)
#define zps_msgMcpsDcfmInd ((OS_thMessage)&os_Message_zps_msgMcpsDcfmInd)
#define zps_msgMlmeDcfmInd_C_Type MAC_tsMlmeVsDcfmInd
#define zps_msgTimeEvents_C_Type zps_tsTimeEvent
#define zps_msgMcpsDcfmInd_C_Type MAC_tsMcpsVsDcfmInd

/* Module JN_AN_1189_ZigBee_HomeAutomation_Demo */

/* Task Handles */
#define APP_PingTimerTask ((OS_thTask)&os_Task_APP_PingTimerTask)

/* Cooperative Task Handles */
#define APP_ButtonsScanTask ((OS_thTask)&os_Task_APP_ButtonsScanTask)
#define APP_ZHA_Sensor_Task ((OS_thTask)&os_Task_APP_ZHA_Sensor_Task)
#define APP_PollTask ((OS_thTask)&os_Task_APP_PollTask)
#define ZCL_Task ((OS_thTask)&os_Task_ZCL_Task)
#define zps_taskZPS ((OS_thTask)&os_Task_zps_taskZPS)
#define APP_SleepTask ((OS_thTask)&os_Task_APP_SleepTask)

/* Message Handles */
#define APP_msgZpsEvents ((OS_thMessage)&os_Message_APP_msgZpsEvents)
#define APP_msgEvents ((OS_thMessage)&os_Message_APP_msgEvents)
#define APP_msgZpsEvents_ZCL ((OS_thMessage)&os_Message_APP_msgZpsEvents_ZCL)
#define APP_msgZpsEvents_C_Type ZPS_tsAfEvent
#define APP_msgEvents_C_Type APP_tsEvent
#define APP_msgZpsEvents_ZCL_C_Type ZPS_tsAfEvent

/* Timer Handles */
#define APP_cntrTickTimer ((OS_thHWCounter)&os_HWCounter_APP_cntrTickTimer)
#define APP_ButtonsScanTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_ButtonsScanTimer)
#define APP_TickTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_TickTimer)
#define APP_PollTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_PollTimer)
#define APP_JoinTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_JoinTimer)
#define APP_BackOffTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_APP_BackOffTimer)
#define App_EZFindAndBindTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_App_EZFindAndBindTimer)
#define App_PingTimer ((OS_thSWTimer)&os_SWTimer_APP_cntrTickTimer_App_PingTimer)

/* Module Exceptions */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef void (*OS_tprISR)(void);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/* Mutex Handles */
PUBLIC OS_thMutex os_Mutex_mutexZPS;
PUBLIC OS_thMutex os_Mutex_mutexPDUM;
PUBLIC OS_thMutex os_Mutex_mutexMAC;
PUBLIC OS_thMutex os_Mutex_hSpiMutex;
PUBLIC OS_thMutex os_Mutex_HA;

/* Message Handles */
PUBLIC OS_thMessage os_Message_zps_msgMlmeDcfmInd;
PUBLIC OS_thMessage os_Message_zps_msgTimeEvents;
PUBLIC OS_thMessage os_Message_zps_msgMcpsDcfmInd;

/* Task Handles */
PUBLIC OS_thTask os_Task_APP_PingTimerTask;

/* Cooperative Task Handles */
PUBLIC OS_thTask os_Task_APP_ButtonsScanTask;
PUBLIC OS_thTask os_Task_APP_ZHA_Sensor_Task;
PUBLIC OS_thTask os_Task_APP_PollTask;
PUBLIC OS_thTask os_Task_ZCL_Task;
PUBLIC OS_thTask os_Task_zps_taskZPS;
PUBLIC OS_thTask os_Task_APP_SleepTask;

/* Message Handles */
PUBLIC OS_thMessage os_Message_APP_msgZpsEvents;
PUBLIC OS_thMessage os_Message_APP_msgEvents;
PUBLIC OS_thMessage os_Message_APP_msgZpsEvents_ZCL;

/* Timer Handles */
PUBLIC OS_thHWCounter os_HWCounter_APP_cntrTickTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_ButtonsScanTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_TickTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_PollTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_JoinTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_APP_BackOffTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_App_EZFindAndBindTimer;
PUBLIC OS_thSWTimer os_SWTimer_APP_cntrTickTimer_App_PingTimer;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void OS_vStart(void (*)(void), void (*)(void), void (*)(OS_teStatus , void *));
PUBLIC OS_tprISR OS_prGetActiveISR(void);

PUBLIC bool os_bAPP_cbSetTickTimerCompare(uint32 );
PUBLIC uint32 os_u32APP_cbGetTickTimer(void);
PUBLIC void os_vAPP_cbEnableTickTimer(void);
PUBLIC void os_vAPP_cbDisableTickTimer(void);

#endif
