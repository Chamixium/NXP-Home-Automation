/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          app_ota_client.c
 *
 * DESCRIPTION:        DK4 (DR1175/DR1199) OTA Client App (Implementation)
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

#include "dbg.h"
#include "pdm.h"
#include "PDM_IDs.h"

#include "os_gen.h"
#include "pdum_gen.h"

#include "ota.h"
#include "ha.h"

#include "app_timer_driver.h"
#include "app_ota_client.h"
#ifdef DimmableLight
    #include "zha_light_node.h"
#endif
#ifdef DimmerSwitch
    #include "zha_switch_node.h"
#endif
#include "app_common.h"
#include "Utilities.h"
#include "rnd_pub.h"
#ifdef CHECK_VBO_FOR_OTA_ACTIVITY
    /* On Chip Peripherals include */
    #include "AppHardwareApi.h"
#endif
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_APP_OTA
    #define TRACE_APP_OTA               FALSE
#else
    #define TRACE_APP_OTA               TRUE
#endif

#ifdef CHECK_VBO_FOR_OTA_ACTIVITY
    #define SYSCON_SYS_IM_ADDR  0x0200000cUL
    #define SYSCON_VBOCTRL_ADDR 0x02000044UL
#endif

#define OTA_CLIENT_EP 1
#define OTA_STARTUP_DELAY_IN_SEC 5
#define OTA_FIND_SERVER 1
#define OTA_FIND_SERVER_WAIT 2
#define OTA_FIND_SERVER_ADDRESS 3
#define OTA_FIND_SERVER_ADDRESS_WAIT 4
#define OTA_QUERYIMAGE 5
#define OTA_DL_PROGRESS 6

#define APP_IEEE_ADDR_RESPONSE 0x8001
#define APP_MATCH_DESCRIPTOR_RESPONSE 0x8006

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitAndDisplayKeys(void);
PRIVATE bool_t bMatchRecieved(void);

PRIVATE teZCL_Status eClientQueryNextImageRequest(
                    uint8 u8SourceEndpoint,
                    uint8 u8DestinationEndpoint,
                    uint16 u16DestinationAddress,
                    uint32 u32FileVersion,
                    uint16 u16HardwareVersion,
                    uint16 u16ImageType,
                    uint16 u16ManufacturerCode,
                    uint8 u8FieldControl);

PRIVATE ZPS_teStatus eSendOTAMatchDescriptor(uint16 u16ProfileId);
PRIVATE void vManagaeOTAState(uint32 u32OTAQueryTimeinSec);
PRIVATE void vGetIEEEAddress(uint8 u8Index);
PRIVATE void vCheckForOTAMatch( ZPS_tsAfEvent  * psStackEvent);
PRIVATE void vCheckForOTAServerIeeeAddress( ZPS_tsAfEvent  * psStackEvent);
PRIVATE void vOTAPersist(void);
PRIVATE void vRestetOTADiscovery(void);
PRIVATE void vManageDLProgressState(void);
#ifdef CLD_TIME
    PRIVATE teZCL_Status eOTAReadTimeFromServer(tsZCL_Address *psDestinationAddress,uint8 u8DestinationEndPointId );
#endif
PRIVATE uint8 u8VerifyLinkKey(tsOTA_CallBackMessage *psCallBackMessage);

#ifdef CHECK_VBO_FOR_OTA_ACTIVITY
    PRIVATE void vOTAActivity(teOTAActivity eOTAActivity);
    PRIVATE void vSetNextBOEvent(teBrownOutTripVoltage eBOTripVoltage,uint32 u32Mask);
    PRIVATE teOTAActivity eGetOTAActivity(void);
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PDM_tsRecordDescriptor OTA_PersistedDataPDDesc;
tsOTA_PersistedData        sOTA_PersistedData;

/* mac address */
PUBLIC  uint8 au8MacAddress[]  __attribute__ ((section (".ro_mac_address"))) = {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Pre-configured Link Key */
PUBLIC  uint8 s_au8LnkKeyArray[16] __attribute__ ((section (".ro_se_lnkKey"))) = {0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,
                                                                  0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39};

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint8 OTA_State=OTA_FIND_SERVER;
PRIVATE uint32 u32TimeOut=0;
PRIVATE tsDiscovedOTAServers sDiscovedOTAServers[MAX_SERVER_NODES];
PRIVATE uint32 u32OTAQueryTimeinSec=OTA_QUERY_TIME_IN_SEC;
PRIVATE uint8 u8Discovered;
PRIVATE uint8 u8Examined;
volatile PRIVATE uint8 au8MacAddressVolatile[8];
#ifdef CLD_TIME
    PRIVATE uint8 u8MaxReadReties=MAX_TIME_READ_RETRIES;
#endif

#ifdef CHECK_VBO_FOR_OTA_ACTIVITY
    PRIVATE teOTAActivity s_eOTAStartStop=APP_E_OTA_START;
#endif
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vAppInitOTA
 *
 * DESCRIPTION:
 * Initialises application OTA client related data structures and calls
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vAppInitOTA(void)
{
    tsNvmDefs sNvmDefs;
    teZCL_Status eZCL_Status;
    uint8 au8CAPublicKey[22];

    vInitAndDisplayKeys();

    #if (OTA_MAX_IMAGES_PER_ENDPOINT == 3)
        uint8 u8StartSector[3] = {0,2,4};
    #else
        #ifdef JENNIC_CHIP_FAMILY_JN516x
            uint8 u8StartSector[1] = {0};
        #else
            uint8 u8StartSector[2] = {0,3};
        #endif
    #endif

    eZCL_Status = eOTA_UpdateClientAttributes(OTA_CLIENT_EP); /* Initialise with the Ep = 1*/
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_APP_OTA, "eOTA_UpdateClientAttributes returned error 0x%x", eZCL_Status);
    }

    sNvmDefs.u32SectorSize = 64*1024; /* Sector Size = 64K*/
    sNvmDefs.u8FlashDeviceType = E_FL_CHIP_AUTO;
    vOTA_FlashInit(NULL,&sNvmDefs);

    #ifdef JENNIC_CHIP_FAMILY_JN516x
        eZCL_Status = eOTA_AllocateEndpointOTASpace(
        	OTA_CLIENT_EP,
            u8StartSector,
            OTA_MAX_IMAGES_PER_ENDPOINT,
            4,
            FALSE,
            au8CAPublicKey);
    #else
        eZCL_Status = eOTA_AllocateEndpointOTASpace(
        	OTA_CLIENT_EP,
            u8StartSector,
            OTA_MAX_IMAGES_PER_ENDPOINT,
            3,
            FALSE,
            au8CAPublicKey);
    #endif
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_APP_OTA, "eAllocateEndpointOTASpace returned error 0x%x", eZCL_Status);
    }
    else
    {
        #if TRACE_APP_OTA
            tsOTA_ImageHeader          sOTAHeader;
            eOTA_GetCurrentOtaHeader(OTA_CLIENT_EP,FALSE,&sOTAHeader);
            DBG_vPrintf(TRACE_APP_OTA,"\n\nCurrent Image Details \n");
            DBG_vPrintf(TRACE_APP_OTA,"File ID = 0x%08x\n",sOTAHeader.u32FileIdentifier);
            DBG_vPrintf(TRACE_APP_OTA,"Header Ver ID = 0x%04x\n",sOTAHeader.u16HeaderVersion);
            DBG_vPrintf(TRACE_APP_OTA,"Header Length ID = 0x%04x\n",sOTAHeader.u16HeaderLength);
            DBG_vPrintf(TRACE_APP_OTA,"Header Control Filed = 0x%04x\n",sOTAHeader.u16HeaderControlField);
            DBG_vPrintf(TRACE_APP_OTA,"Manufac Code = 0x%04x\n",sOTAHeader.u16ManufacturerCode);
            DBG_vPrintf(TRACE_APP_OTA,"Image Type = 0x%04x\n",sOTAHeader.u16ImageType);
            DBG_vPrintf(TRACE_APP_OTA,"File Ver = 0x%08x\n",sOTAHeader.u32FileVersion);
            DBG_vPrintf(TRACE_APP_OTA,"Stack Ver = 0x%04x\n",sOTAHeader.u16StackVersion);
            DBG_vPrintf(TRACE_APP_OTA,"Image Len = 0x%08x\n\n\n",sOTAHeader.u32TotalImage);
        #endif

        eZCL_Status = eOTA_RestoreClientData(OTA_CLIENT_EP,&sOTA_PersistedData,TRUE);

        DBG_vPrintf(TRACE_APP_OTA,"OTA PDM Status = %d \n",eZCL_Status);

        #ifdef CLD_TIME
            if(E_ZCL_SUCCESS == eZCL_Status)
            {
            	u8MaxReadReties = MAX_TIME_READ_RETRIES;
                OS_eStartSWTimer(App_TimeReadJitterTimer,APP_TIME_MS(500),NULL);
            }
        #endif
    }
#ifdef CHECK_VBO_FOR_OTA_ACTIVITY
    vInitVBOForOTA(APP_OTA_VBATT_LOW_THRES);
#endif

}


#ifdef CHECK_VBO_FOR_OTA_ACTIVITY
/****************************************************************************
 *
 * NAME: vInitVBOForOTA
 *
 * DESCRIPTION:
 * Initializes the VBO for the trip
 *
 * PARAMETERS:
 * teBrownOutTripVoltage Brown out voltage enumeration values
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vInitVBOForOTA(teBrownOutTripVoltage eBOTripValue)
{
    /* WORKAROUND for [lpsw4269]Set trip voltage, enable SVM & disable reset on BO */
    *(uint32 *)SYSCON_VBOCTRL_ADDR = eBOTripValue |1;

    /* Enable Rising Event mask (VDD below BO trip voltage) */
    *(uint32 *)SYSCON_SYS_IM_ADDR |= E_AHI_SYSCTRL_VREM_MASK;


    if(bAHI_BrownOutStatus())
    {
    	/* Depending on the voltage do a start or stop */
    	DBG_vPrintf(TRACE_APP_OTA,"BrownOut at Init\n");
    	vOTAActivity(APP_E_OTA_STOP);
    }
    else
    {
    	vOTAActivity(APP_E_OTA_START);
    	DBG_vPrintf(TRACE_APP_OTA,"Start at Init\n");
    }

}
/****************************************************************************
 *
 * NAME: vCbSystemControllerOTAVoltageCheck
 *
 * DESCRIPTION:
 * Checks the interrupt source and sets the next VBO trip voltage
 *
 * PARAMETERS:
 * u32DeviceId      The device that interrupted
 * u32ItemBitmap    The bit mask of the interrupt
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vCbSystemControllerOTAVoltageCheck(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
    /* Disable both interrupts */
    *(uint32 *)SYSCON_SYS_IM_ADDR &= ~(E_AHI_SYSCTRL_VREM_MASK|E_AHI_SYSCTRL_VFEM_MASK);

    /* Validate interrupt source when brown out state entered */
    if (E_AHI_DEVICE_SYSCTRL == u32DeviceId)
    {
        /*Voltage is Low, hence setting voltage for the rising edge */
        if(E_AHI_SYSCTRL_VREM_MASK & u32ItemBitmap)
        {
            /*Stop OTA activity until next interrupt*/
            vSetNextBOEvent(APP_OTA_VBATT_HI_THRES,E_AHI_SYSCTRL_VFEM_MASK);
            /* OTA activity Stop */
            vOTAActivity(APP_E_OTA_STOP);
        }
        /* Voltage is reported high, hence set the falling edge  */
        else if (E_AHI_SYSCTRL_VFEM_MASK & u32ItemBitmap)
        {
            /*Start OTA activity until next interrupt*/
            vSetNextBOEvent(APP_OTA_VBATT_LOW_THRES,E_AHI_SYSCTRL_VREM_MASK);
            /* OTA activity Start/Stop */
            vOTAActivity(APP_E_OTA_START);
        }
    }
}
#endif
/****************************************************************************
 *
 * NAME: vLoadOTAPersistedData
 *
 * DESCRIPTION:
 * Loads back OTA persisted data from PDM in application at start up.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadOTAPersistedData(void)
{
    /*Restore OTA Persistent Data*/
    PDM_teStatus eStatusOTAReload;
    eStatusOTAReload = PDM_eLoadRecord(&OTA_PersistedDataPDDesc,
                       PDM_ID_OTA_DATA,
                       &sOTA_PersistedData,
                       sizeof(tsOTA_PersistedData),
                       FALSE);

    DBG_vPrintf(TRACE_APP_OTA,"eStatusOTAReload=%d\n",eStatusOTAReload);

    #ifndef CLD_TIME
            if (eStatusOTAReload== PDM_E_STATUS_RECOVERED)
            {
                /*Make Block request time 1 as time cluster is not present*/
                if(sOTA_PersistedData.u32RequestBlockRequestTime !=0)
                {
                    sOTA_PersistedData.u32RequestBlockRequestTime=1;
                }
                /*Make retries 0*/
                sOTA_PersistedData.u8Retry=0;
            }
    #endif
}
/****************************************************************************
 *
 * NAME: vHandleZDPReqResForOTA
 *
 * DESCRIPTION:
 * Handles the stack event for OTA discovery. Called from the OS Task
 * upon a stack event.
 *
 * INPUT:
 * ZPS_tsAfEvent  * psStackEvent Pointer to stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleZDPReqResForOTA(ZPS_tsAfEvent  * psStackEvent)
{
    vCheckForOTAMatch(psStackEvent);
    vCheckForOTAServerIeeeAddress(psStackEvent);
}
/****************************************************************************
 *
 * NAME: vCheckforOTAOTAUpgrade
 *
 * DESCRIPTION:
 * Timely checks for the OTA upgrade when the device state is running.
 * This is called from a timer ticking at rate of 1sec
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vRunAppOTAStateMachine(void)
{
    #ifdef CHECK_VBO_FOR_OTA_ACTIVITY
    DBG_vPrintf(TRACE_APP_OTA,"OTA Activity = %d \n",eGetOTAActivity());
    if(APP_E_OTA_STOP != eGetOTAActivity())
    #endif
    {
        if( E_RUNNING == eGetNodeState())
        {
             /*Increment Second timer */
            u32OTAQueryTimeinSec++;
            if(u32OTAQueryTimeinSec > OTA_STARTUP_DELAY_IN_SEC )
                vManagaeOTAState(u32OTAQueryTimeinSec);
        }
        else
        {
        #ifndef CLD_TIME
            if(sOTA_PersistedData.sAttributes.u8ImageUpgradeStatus == E_CLD_OTA_STATUS_NORMAL)
        #endif
                u32OTAQueryTimeinSec=0;
        }
    }
}

/****************************************************************************
 *
 * NAME: vHandleAppOtaClient
 *
 * DESCRIPTION:
 * Handles the OTA Cluster Client events.
 * This is called from the EndPoint call back in the application
 * when an OTA event occurs.
 *
 * INPUT:
 * tsOTA_CallBackMessage *psCallBackMessage Pointer to cluster callback message
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleAppOtaClient(tsOTA_CallBackMessage *psCallBackMessage)
{

    DBG_vPrintf(TRACE_APP_OTA, "OTA Event Id = %d\n",psCallBackMessage->eEventId);

    if(psCallBackMessage->eEventId == E_CLD_OTA_COMMAND_QUERY_NEXT_IMAGE_RESPONSE )
    {
        DBG_vPrintf(TRACE_APP_OTA,"\n\n\nQuery Next Image Response \n");
        DBG_vPrintf(TRACE_APP_OTA,"Image Size 0x%08x\n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u32ImageSize);
        DBG_vPrintf(TRACE_APP_OTA,"File Ver  0x%08x \n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u32FileVersion);
        DBG_vPrintf(TRACE_APP_OTA,"Manufacture Code 0x%04x  \n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u16ManufacturerCode);
        DBG_vPrintf(TRACE_APP_OTA,"Image Type 0x%04x\n\n\n",psCallBackMessage->uMessage.sQueryImageResponsePayload.u16ImageType);
    }

    if(psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_VERIFY_IMAGE_VERSION )
    {
        DBG_vPrintf(TRACE_APP_OTA,"\n\n\nVerify the version \n");
        DBG_vPrintf(TRACE_APP_OTA,"Current Ver = 0x%08x\n",psCallBackMessage->uMessage.sImageVersionVerify.u32CurrentImageVersion );
        DBG_vPrintf(TRACE_APP_OTA,"Current Ver = 0x%08x\n",psCallBackMessage->uMessage.sImageVersionVerify.u32NotifiedImageVersion);

        if (psCallBackMessage->uMessage.sImageVersionVerify.u32CurrentImageVersion ==
            psCallBackMessage->uMessage.sImageVersionVerify.u32NotifiedImageVersion )
        {
                psCallBackMessage->uMessage.sImageVersionVerify.eImageVersionVerifyStatus =E_ZCL_FAIL;
                OTA_State = OTA_FIND_SERVER;
                vRestetOTADiscovery();
        }
        #ifdef CLD_TIME
            else
            {
                /* Looks like we are going to upgrade get the time from server */
            	u8MaxReadReties = MAX_TIME_READ_RETRIES;
                OS_eStartSWTimer(App_TimeReadJitterTimer,APP_TIME_MS(500),NULL);
            }
        #endif
    }

    if(psCallBackMessage->eEventId == E_CLD_OTA_COMMAND_UPGRADE_END_RESPONSE )
    {
        DBG_vPrintf(TRACE_APP_OTA,"\n\n\nUpgrade End Response \n");
        DBG_vPrintf(TRACE_APP_OTA,"Upgrade Time : 0x%08x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime);
        DBG_vPrintf(TRACE_APP_OTA,"Current Time : 0x%08x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime);
        DBG_vPrintf(TRACE_APP_OTA,"File Version : 0x%08x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u32FileVersion);
        DBG_vPrintf(TRACE_APP_OTA,"Image Type   : 0x%04x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u16ImageType);
        DBG_vPrintf(TRACE_APP_OTA,"Manufacturer : 0x%04x\n",psCallBackMessage->uMessage.sUpgradeResponsePayload.u16ManufacturerCode);
        /* If no time is defined */
        #ifndef CLD_TIME
            if(psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime > psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime)
            {
                psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime -= psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime;
            }
            else
            {
                /* If upgrade time is in past , upgrade in one second*/
                psCallBackMessage->uMessage.sUpgradeResponsePayload.u32UpgradeTime = 1;
            }
            psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime = 0;
        #else
            #ifdef OVER_RIDE_UTC_FROM_OTA
                vZCL_SetUTCTime(psCallBackMessage->uMessage.sUpgradeResponsePayload.u32CurrentTime);
            #endif
        #endif
    }

    if(psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_SAVE_CONTEXT )
    {
        DBG_vPrintf(TRACE_APP_OTA,"\nSave Context\n");
        vOTAPersist();
    }
    if(psCallBackMessage->eEventId == E_CLD_OTA_INTERNAL_COMMAND_SWITCH_TO_UPGRADE_DOWNGRADE )
    {
        DBG_vPrintf(TRACE_APP_OTA,"\nSwitching to New Image\n");
    }
    if(psCallBackMessage->eEventId ==  E_CLD_OTA_INTERNAL_COMMAND_OTA_DL_ABORTED)
    {
        DBG_vPrintf(TRACE_APP_OTA,"DL complete INvalid Image\n\n");
    }
    if(psCallBackMessage->eEventId ==  E_CLD_OTA_INTERNAL_COMMAND_RESET_TO_UPGRADE)
    {
        DBG_vPrintf(TRACE_APP_OTA,"E_CLD_OTA_INTERNAL_COMMAND_RESET_TO_UPGRADE\n\n");
        //vDumpFlashData(0,200000);
    }
    if(psCallBackMessage->eEventId ==  E_CLD_OTA_COMMAND_UPGRADE_END_RESPONSE)
    {
        DBG_vPrintf(TRACE_APP_OTA,"E_CLD_OTA_COMMAND_UPGRADE_END_RESPONSE\n\n");
    }
    if(psCallBackMessage->eEventId == E_CLD_OTA_COMMAND_BLOCK_RESPONSE)
    {
        //DBG_vPrintf(TRUE,"Block Resp Status = %d\n",psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status);
        if(psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status == OTA_STATUS_SUCCESS)
        {
            psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status = u8VerifyLinkKey(psCallBackMessage);
            if(OTA_STATUS_ABORT == psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status)
            {
                /* Send an Abort from Application, the ZCL is not doing it.*/
                tsOTA_UpgradeEndRequestPayload sUpgradeEndRequestPayload;

                sUpgradeEndRequestPayload.u32FileVersion = psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u32FileVersion;
                sUpgradeEndRequestPayload.u16ImageType = psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u16ImageType;
                sUpgradeEndRequestPayload.u16ManufacturerCode = psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u16ManufacturerCode;
                sUpgradeEndRequestPayload.u8Status = psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status;

                eOTA_ClientUpgradeEndRequest(
                         OTA_CLIENT_EP,                                         //                                  uint8                       u8SourceEndpoint,
                         psCallBackMessage->sPersistedData.u8DstEndpoint,       //                                  uint8                       u8DestinationEndpoint,
                         &psCallBackMessage->sPersistedData.sDestinationAddress,//                                  tsZCL_Address              *psDestinationAddress,
                         &sUpgradeEndRequestPayload);
            }
        }
        #ifdef DEEP_SLEEP_ENABLE
            vLoadDeepSleepTimer(DEEP_SLEEP_TIME);
        #endif
        #ifdef CHECK_VBO_FOR_OTA_ACTIVITY
        if(APP_E_OTA_STOP == eGetOTAActivity())
        {
                psCallBackMessage->uMessage.sImageBlockResponsePayload.u8Status = OTA_STATUS_WAIT_FOR_DATA;
                psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sWaitForData.u32CurrentTime = 0;
                psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sWaitForData.u32RequestTime = 0xFFFF0000;
        }
        #endif
    }
    if(psCallBackMessage->eEventId ==  E_CLD_OTA_INTERNAL_COMMAND_OTA_DL_ABORTED)
    {
        DBG_vPrintf(TRACE_APP_OTA,"E_CLD_OTA_INTERNAL_COMMAND_OTA_DL_ABORTED\n\n");
    }
    u32TimeOut = 0;
}

/****************************************************************************
 *
 * NAME: vDumpFlashData
 *
 * DESCRIPTION:
 * Dumps the OTA downloaded data on debug terminal at a chunk of 16 bytes
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vDumpFlashData(uint32 u32FlashByteLocation, uint32 u32EndLocation)
{
    uint8 au8Data[16];

    for (;u32FlashByteLocation<=u32EndLocation;u32FlashByteLocation +=16 )
    {
        bAHI_FullFlashRead(u32FlashByteLocation,16,au8Data);

        uint8 u8Len;
        DBG_vPrintf(TRUE,"0x%08x : ",u32FlashByteLocation);
        for (u8Len=0;u8Len<16;u8Len++)
        {
            DBG_vPrintf(TRUE,"%02x ",au8Data[u8Len]);
        }
        DBG_vPrintf(TRUE,"\n",au8Data[u8Len]);

        volatile uint32 u32Delay=10000;
        for(u32Delay=10000;u32Delay>0;u32Delay--);

        vAHI_WatchdogRestart();
    }
}
/****************************************************************************
 *
 * NAME: vIncrementTimeOut
 *
 * DESCRIPTION:
 * Increment Timeout value by the u8TimeInSec to compensate for any sleep
 *
 * PARAMETERS:
 * uint8 u8TimeInSec Time in seconds to be incremented
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vIncrementTimeOut(uint8 u8TimeInSec)
{
    if(u32OTAQueryTimeinSec != 0)
    {
        u32OTAQueryTimeinSec += u8TimeInSec;
    }
    if(u32TimeOut != 0)
    {
        u32TimeOut += u8TimeInSec;
    }
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
#ifdef CHECK_VBO_FOR_OTA_ACTIVITY
/****************************************************************************
 *
 * NAME: vSetNextBOEvent
 *
 * DESCRIPTION:
 * Sets the voltage and rising/falling event to occur
 *
 * PARAMETERS:
 * eBOTripVoltage       Trip Voltage
 * u32Mask              Rising or falling mask
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetNextBOEvent(teBrownOutTripVoltage eBOTripVoltage,uint32 u32Mask)
{
    /* Set trip voltage to indicate start activity, enable SVM & disable reset on BO */
    *(uint32 *)SYSCON_VBOCTRL_ADDR =  eBOTripVoltage|1;

    /*Set the falling edge and reset the rising edge */
    *(uint32 *)SYSCON_SYS_IM_ADDR |= u32Mask;
}

/****************************************************************************
 *
 * NAME: vOTAActivity
 *
 * DESCRIPTION:
 * Manipulates the block request time to based on the voltage
 *
 * PARAMETERS:
 * teBrownOutTripVoltage Brown out voltage enumeration values
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vOTAActivity(teOTAActivity eOTAStartStop)
{
    s_eOTAStartStop = eOTAStartStop;
    tsOTA_PersistedData * psOTAPersistedData = psGetOTACallBackPersistdata();

    if(psOTAPersistedData->sAttributes.u8ImageUpgradeStatus != E_CLD_OTA_STATUS_NORMAL)
    {
        switch(eOTAStartStop)
        {
            case APP_E_OTA_STOP:
                psOTAPersistedData->u32RequestBlockRequestTime = 0xFFFFFFF0;
                DBG_vPrintf(TRACE_APP_OTA,"Stopped OTA Block Request by Setting time to 0x%08x \n",psOTAPersistedData->u32RequestBlockRequestTime);
                break;
            case APP_E_OTA_START:
                psOTAPersistedData->u32RequestBlockRequestTime =  1;
                /*Make retries 0*/
                psOTAPersistedData->u8Retry=0;
                DBG_vPrintf(TRACE_APP_OTA,"Resumed OTA Block Request by Setting time to 0x%08x \n",psOTAPersistedData->u32RequestBlockRequestTime);
                break;
            default :
                break;
        }
    }
}
/****************************************************************************
 *
 * NAME: eGetOTAActivity
 *
 * DESCRIPTION:
 * Gets the activity of OTA
 *
 * PARAMETERS:
 * void
 *
 * RETURNS:
 * OTA to start or to stop
 *
 ****************************************************************************/
PRIVATE teOTAActivity eGetOTAActivity(void)
{
    return s_eOTAStartStop;
}
#endif
/****************************************************************************
 *
 * NAME: u8VerifyLinkKey
 *
 * DESCRIPTION:
 * Verifies link key once first 1K data is downloaded and saved in flash
 *
 * RETURNS:
 * staus of the verification
 *
 ****************************************************************************/
PRIVATE uint8 u8VerifyLinkKey(tsOTA_CallBackMessage *psCallBackMessage)
{
    extern uint32 enc_start;
    static bool_t bKeyVerified=FALSE;

    //DBG_vPrintf(TRUE,"Block Resp Offset = %d \n",psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u32FileOffset);
    //DBG_vPrintf(TRUE,"Flash_start = 0x%08x\n",&flash_start);
    //DBG_vPrintf(TRUE,"LinkKey Location = 0x%08x\n",(uint32)&FlsLinkKey);

    /*Assumption : First 1 K downloaded and saved to external flash*/
    if(
            (bKeyVerified == FALSE) &&
            (psCallBackMessage->uMessage.sImageBlockResponsePayload.uMessage.sBlockPayloadSuccess.u32FileOffset > APP_OTA_OFFSET_WRITEN_BEFORE_LINKKEY_VERIFICATION )
       )
    {
        bKeyVerified = TRUE;

        int i;
        uint8 au8DownloadedLnkKey[0x10];
        uint8 au8Key[0x10];

        uint32 u32LnkKeyLocation = (uint32)&FlsLinkKey - (uint32)&flash_start;
        DBG_vPrintf(TRACE_APP_OTA,"Link Key Offset in External Flash = 0x%08x\n",u32LnkKeyLocation);
        bAHI_FullFlashRead(u32LnkKeyLocation,0x10,au8DownloadedLnkKey);

        /*Get a copy of the Lnk Key*/
        for(i=0;i<16;i++)
        {
            au8Key[i] = s_au8LnkKeyArray[i];
        }

        uint32 u32CustomerSettings = *((uint32*)FL_INDEX_SECTOR_CUSTOMER_SETTINGS);
        bool_t bEncExternalFlash = (u32CustomerSettings & FL_INDEX_SECTOR_ENC_EXT_FLASH)?FALSE:TRUE;

        if(bEncExternalFlash)
        {
            uint8 au8Iv[0x10];
            uint8 au8DataOut[0x10];

            uint32 u32IVLocation = 0x10;
            tsReg128 sKey;

            /*Get the downloaded IV from Ext Flash */
            bAHI_FullFlashRead(u32IVLocation,0x10,au8Iv);
            DBG_vPrintf(TRACE_APP_OTA,"The Plain IV :\n");

            for (i=0;i<0x10;i++)
                DBG_vPrintf(TRACE_APP_OTA,"au8Iv[%d]: 0x%02x\n",i,au8Iv[i]);

            DBG_vPrintf(TRACE_APP_OTA,"The Enc Offset = 0x%08x\n",((uint32)&enc_start));
            uint32 u32LnkKeyFromEncStart=  (u32LnkKeyLocation -(uint32)&enc_start);
            DBG_vPrintf(TRACE_APP_OTA,"The The Total Bytes between Enc Offset and LnkKey = 0x%08x\n", u32LnkKeyFromEncStart );
            uint8 u8IVIncrement = (uint8)(u32LnkKeyFromEncStart/16);
            DBG_vPrintf(TRACE_APP_OTA,"The IV should be increased by = 0x%08x\n", u8IVIncrement);

            /*Increase IV*/
            au8Iv[15] = au8Iv[15]+u8IVIncrement;

            /*Get the EFUSE keys*/
            uint32 *pu32KeyPtr = (uint32*)FL_INDEX_SECTOR_ENC_KEY_ADDRESS;

            sKey.u32register0 = *pu32KeyPtr;
            sKey.u32register1 = *(pu32KeyPtr+1);
            sKey.u32register2 = *(pu32KeyPtr+2);
            sKey.u32register3 = *(pu32KeyPtr+3);

            DBG_vPrintf(TRACE_APP_OTA,"The Key is :\n");
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register0: 0x%08x\n",sKey.u32register0);
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register1: 0x%08x\n",sKey.u32register1);
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register2: 0x%08x\n",sKey.u32register2);
            DBG_vPrintf(TRACE_APP_OTA,"sKey.u32register3: 0x%08x\n",sKey.u32register3);

            /*Encrypt the IV*/
            bACI_ECBencodeStripe(&sKey,TRUE,(tsReg128 *)au8Iv,(tsReg128 *)au8DataOut);

            /* Encrypt the Internal Key */
            for(i=0;i<16;i++)
            {
                au8Key[i] = au8Key[i] ^ au8DataOut[i];
            }
        }

        /*Comparing the Keys*/
        for(i=0;i<0x10;i++)
        {
            DBG_vPrintf(TRACE_APP_OTA,"Internal Key[%d] = %02x Downloaded Key[%d] = 0x%02x \n",i,au8Key[i],i,au8DownloadedLnkKey[i]);

            /*Compare to see if they match else its an invalid image*/
            if(au8Key[i]!=au8DownloadedLnkKey[i])
            {
                DBG_vPrintf(TRACE_APP_OTA,"Key Mismatch, Abort DownLoad\n");
                bKeyVerified=FALSE;
                return OTA_STATUS_ABORT;
            }
        }
    }
    return OTA_STATUS_SUCCESS;
}


/****************************************************************************
 *
 * NAME: vInitAndDisplayKeys
 *
 * DESCRIPTION:
 * Initialize Keys and displays the content
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitAndDisplayKeys(void)
{
    #ifndef JENNIC_CHIP_FAMILY_JN514x
        /*ZPS_vSetOverrideLocalMacAddress((uint64 *)&au8MacAddress);*/
        volatile uint8 u8Index;
        for(u8Index=0;u8Index<8;u8Index++)
            au8MacAddressVolatile[u8Index]=au8MacAddress[u8Index];
    #endif

    #if TRACE_APP_OTA==TRUE
        uint8 i;
        DBG_vPrintf(TRACE_APP_OTA, "MAC Address at address = %08x\n\n\n",au8MacAddress);
        for (i =0;i<8;i++)
        {
            DBG_vPrintf(TRACE_APP_OTA, "%02x ",au8MacAddress[i] );
        }

        DBG_vPrintf(TRACE_APP_OTA, "\n\n\n Link Key ");
        for (i =0;i<16;i++)
        {
            DBG_vPrintf(TRACE_APP_OTA, "%02x ",s_au8LnkKeyArray[i] );
        }
        DBG_vPrintf(TRACE_APP_OTA, "\n\n\n");
    #endif
}

/****************************************************************************
 *
 * NAME: vCheckForOTAMatch
 *
 * DESCRIPTION:
 * Checks for the OTA cluster match during OTA server discovery, if a match
 * found it will make an entry in the local discovery table.This table will be
 * used to query image requests by the client.
 *
 *
 * INPUT:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCheckForOTAMatch( ZPS_tsAfEvent  * psStackEvent)
{
    if (APP_MATCH_DESCRIPTOR_RESPONSE == psStackEvent->uEvent.sApsZdpEvent.u16ClusterId)
    {
        if(
                (!psStackEvent->uEvent.sApsZdpEvent.uZdpData.sMatchDescRsp.u8Status)
                #ifdef IGNORE_COORDINATOR_AS_OTA_SERVER
                    &&
                    (0x0000 != psStackEvent->uEvent.sApsZdpEvent.uZdpData.sMatchDescRsp.u16NwkAddrOfInterest)
                #endif
                )
        {
            uint8 i;
            sDiscovedOTAServers[u8Discovered].u16NwkAddrOfServer= psStackEvent->uEvent.sApsZdpEvent.uZdpData.sMatchDescRsp.u16NwkAddrOfInterest;
            DBG_vPrintf(TRACE_APP_OTA,"\n\nNwk Address oF server = %04x\n",sDiscovedOTAServers[u8Discovered].u16NwkAddrOfServer);

            sDiscovedOTAServers[u8Discovered].u8MatchLength = psStackEvent->uEvent.sApsZdpEvent.uZdpData.sMatchDescRsp.u8MatchLength;
            DBG_vPrintf(TRACE_APP_OTA,"Number of OTA Server EPs = %d\n",sDiscovedOTAServers[u8Discovered].u8MatchLength);

            for( i=0; i<sDiscovedOTAServers[u8Discovered].u8MatchLength && i<MAX_SERVER_EPs ;i++)
            {
                /*sDiscovedOTAServers[u8Discovered].u8MatchList[i] = psStackEvent->uEvent.sApsZdpEvent.uZdpData.sMatchDescRsp.pu8MatchList[i];*/
                sDiscovedOTAServers[u8Discovered].u8MatchList[i] = psStackEvent->uEvent.sApsZdpEvent.uLists.au8Data[i];
                DBG_vPrintf(TRACE_APP_OTA,"OTA Server EP# = %d\n",sDiscovedOTAServers[u8Discovered].u8MatchList[i]);
            }
            vGetIEEEAddress(u8Discovered);
            u8Discovered++;
        }
    }
}
/****************************************************************************
 *
 * NAME: vCheckForOTAServerIeeeAddress
 *
 * DESCRIPTION:
 * Handles IEEE address look up query query
 * Makes an entry in the application OTA discovery table. Later this is used
 * for by the OTA requests.
 *
 * This function is called from the application OTA handler with stack event
 * as input.
 *
 *
 * INPUT:
 * ZPS_tsAfEvent  * psStackEvent   Pointer to the stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCheckForOTAServerIeeeAddress( ZPS_tsAfEvent  * psStackEvent)
{
    if (APP_IEEE_ADDR_RESPONSE == psStackEvent->uEvent.sApsZdpEvent.u16ClusterId)
    {
        if(!psStackEvent->uEvent.sApsZdpEvent.uZdpData.sIeeeAddrRsp.u8Status)
        {
            uint8 i;
            for( i=0; i<u8Discovered ;i++)
            {
                if( sDiscovedOTAServers[i].u16NwkAddrOfServer ==
                    psStackEvent->uEvent.sApsZdpEvent.uZdpData.sIeeeAddrRsp.u16NwkAddrRemoteDev)
                {
                    /*Make an entry in the OTA server tables*/
                    sDiscovedOTAServers[i].u64IeeeAddrOfServer = psStackEvent->uEvent.sApsZdpEvent.uZdpData.sIeeeAddrRsp.u64IeeeAddrRemoteDev;
                    DBG_vPrintf(TRACE_APP_OTA,"Entry Added NWK Addr 0x%04x IEEE Addr 0x%016x",
                            sDiscovedOTAServers[i].u16NwkAddrOfServer,sDiscovedOTAServers[i].u64IeeeAddrOfServer);
                    ZPS_eAplZdoAddAddrMapEntry( sDiscovedOTAServers[i].u16NwkAddrOfServer,sDiscovedOTAServers[i].u64IeeeAddrOfServer);
                    sDiscovedOTAServers[i].bValid = TRUE;
                }
            }
        }
    }
}
/****************************************************************************
 *
 * NAME: vGetIEEEAddress
 *
 * DESCRIPTION:
 * Finds an IEEE address on the local node by calling Stack API, if no entries
 * found it request the IEEE look up on air.
 *
 *
 * INPUT:
 * uint8 u8Index   Index to the discovery table point to the NWK address of
 *                 the discovered server
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vGetIEEEAddress(uint8 u8Index)
{
    /* Always query for address Over the air - Uncomment if required for local look up first*/
    #ifdef LOCAL_ADDRESS_LOOK_UP
    /* See if there is a local address map exists  */
    uint64 u64IeeeAdress = ZPS_u64AplZdoLookupIeeeAddr(sDiscovedOTAServers[u8Index].u16NwkAddrOfServer);
    if( u64IeeeAdress != 0x0000000000000000 || u64IeeeAdress != 0xFFFFFFFFFFFFFFFF )
    {
        /*Valid address found, setting up the OTA server address */
        sDiscovedOTAServers[u8Index].u64IeeeAddrOfServer = u64IeeeAdress;
    }
    else
    #endif
    {
        /* If there is no address map existing, then do a look up */
        PDUM_thAPduInstance hAPduInst;
        hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

        if (hAPduInst == PDUM_INVALID_HANDLE)
        {
            DBG_vPrintf(TRACE_APP_OTA, "IEEE Address Request - PDUM_INVALID_HANDLE\n");
        }
        else
        {
            ZPS_tuAddress uDstAddr;
            bool bExtAddr;
            uint8 u8SeqNumber;
            ZPS_teStatus eStatus;
            ZPS_tsAplZdpIeeeAddrReq sZdpIeeeAddrReq;

            uDstAddr.u16Addr = sDiscovedOTAServers[u8Examined].u16NwkAddrOfServer;
            bExtAddr = FALSE;
            sZdpIeeeAddrReq.u16NwkAddrOfInterest=sDiscovedOTAServers[u8Examined].u16NwkAddrOfServer;
            sZdpIeeeAddrReq.u8RequestType =0;
            sZdpIeeeAddrReq.u8StartIndex =0;

            eStatus= ZPS_eAplZdpIeeeAddrRequest(
                                            hAPduInst,
                                            uDstAddr,
                                            bExtAddr,
                                            &u8SeqNumber,
                                            &sZdpIeeeAddrReq);
            if (eStatus)
            {
                PDUM_eAPduFreeAPduInstance(hAPduInst);
                DBG_vPrintf(TRACE_APP_OTA, "Address Request failed: 0x%02x\n", eStatus);
            }
        }
    }
}
/****************************************************************************
 *
 * NAME: bMatchRecieved
 *
 * DESCRIPTION:
 * Validation function for the match and sets the valid field true in the
 * discovery table if there is valid OTA Server found in the network
 *
 *
 * INPUT:
 *
 * RETURNS:
 * TRUE/FALSE
 *
 ****************************************************************************/
PRIVATE bool_t bMatchRecieved(void)
{
    uint8 i;

    for (i =0; i < u8Discovered;i++)
    {
        if(sDiscovedOTAServers[i].bValid)
            return TRUE;
    }
    return FALSE;
}
/****************************************************************************
 *
 * NAME: eClientQueryNextImageRequest
 *
 * DESCRIPTION:
 * Query nest image request application wrapper.
 *
 *
 * INPUT:
 *  uint8 u8SourceEndpoint
 *  uint8 u8DestinationEndpoint,
 *  uint16 u16DestinationAddress,
 *  uint32 u32FileVersion,
 *  uint16 u16HardwareVersion,
 *  uint16 u16ImageType,
 *  uint16 u16ManufacturerCode,
 *  uint8 u8FieldControl
 *
 * RETURNS:
 * ZCL status of the call
 *
 ****************************************************************************/
PRIVATE teZCL_Status eClientQueryNextImageRequest(
                    uint8 u8SourceEndpoint,
                    uint8 u8DestinationEndpoint,
                    uint16 u16DestinationAddress,
                    uint32 u32FileVersion,
                    uint16 u16HardwareVersion,
                    uint16 u16ImageType,
                    uint16 u16ManufacturerCode,
                    uint8 u8FieldControl)
{
    tsZCL_Address sAddress;
    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = u16DestinationAddress;

    tsOTA_QueryImageRequest sRequest;
    sRequest.u32CurrentFileVersion = u32FileVersion;
    sRequest.u16HardwareVersion = u16HardwareVersion;
    sRequest.u16ImageType = u16ImageType;
    sRequest.u16ManufacturerCode = u16ManufacturerCode;
    sRequest.u8FieldControl = u8FieldControl;


    return eOTA_ClientQueryNextImageRequest(
            u8SourceEndpoint,
            u8DestinationEndpoint,
            &sAddress,
            &sRequest);
}
/****************************************************************************
 *
 * NAME: eSendOTAMatchDescriptor
 *
 * DESCRIPTION:
 * Sends the OTA match descriptor for OTA server discovery as a broadcast.
 *
 *
 * INPUT:
 *  uint16 u16ProfileId Profile Identifier
 *
 * RETURNS:
 * ZPS status of the call
 *
 ****************************************************************************/
PRIVATE ZPS_teStatus eSendOTAMatchDescriptor(uint16 u16ProfileId)
{
    uint16 au16InClusters[]={OTA_CLUSTER_ID};
    uint8 u8TransactionSequenceNumber;
    ZPS_tuAddress uDestinationAddress;
    ZPS_tsAplZdpMatchDescReq sMatch;

    sMatch.u16ProfileId = u16ProfileId;
    sMatch.u8NumInClusters=sizeof(au16InClusters)/sizeof(uint16);
    sMatch.pu16InClusterList=au16InClusters;
    sMatch.pu16OutClusterList=NULL;
    sMatch.u8NumOutClusters=0;
    sMatch.u16NwkAddrOfInterest=0xFFFD;

    uDestinationAddress.u16Addr = 0xFFFD;

    PDUM_thAPduInstance hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

    if (hAPduInst == PDUM_INVALID_HANDLE)
    {
        DBG_vPrintf(TRACE_APP_OTA, "Allocate PDU ERR:\n");
        return (ZPS_teStatus)PDUM_E_INVALID_HANDLE;
    }

    ZPS_teStatus eStatus = ZPS_eAplZdpMatchDescRequest(
                            hAPduInst,
                            uDestinationAddress,
                            FALSE,
                            &u8TransactionSequenceNumber,
                            &sMatch);

    if (eStatus)
    {
        PDUM_eAPduFreeAPduInstance(hAPduInst);
        DBG_vPrintf(TRACE_APP_OTA, "Match ERR: 0x%x", eStatus);
    }

    return eStatus;
}
/****************************************************************************
 *
 * NAME: vManagaeOTAState
 *
 * DESCRIPTION:
 * Simple State Machine to move the OTA state from Discovery to Download.
 * It also implements a simple mechanism of time out.
 *
 * INPUT:
 * uint32 u32OTAQueryTime
 *
 * RETURNS:
 *
 *
 ****************************************************************************/
PRIVATE void vManagaeOTAState(uint32 u32OTAQueryTime)
{
    /* Get the persisted state of the OTA  */

    switch(OTA_State)
    {
        case OTA_FIND_SERVER:
        {
            if(u32OTAQueryTime > OTA_QUERY_TIME_IN_SEC )
            {
                u32OTAQueryTimeinSec=0;
                ZPS_teStatus eStatus;
                eStatus = eSendOTAMatchDescriptor(HA_PROFILE_ID);
                if ( eStatus)
                {
                    u32TimeOut=0;
                    OTA_State = OTA_FIND_SERVER;
                    vRestetOTADiscovery();
                    DBG_vPrintf(TRACE_APP_OTA, "Send Match Error 0x%02x\n",eStatus);
                }
                else
                {
                    /*Wait for the discovery to complete */
                    u32TimeOut=0;
                    OTA_State = OTA_FIND_SERVER_WAIT;
                }
            }
        }
        break;

        case OTA_FIND_SERVER_WAIT:
        {
            u32TimeOut++;
            if( bMatchRecieved() )
            {
                u32TimeOut=0;
                OTA_State = OTA_QUERYIMAGE;
            }
            else if(u32TimeOut > OTA_DISCOVERY_TIMEOUT_IN_SEC )
            {
                u32TimeOut=0;
                OTA_State = OTA_FIND_SERVER;
                vRestetOTADiscovery();
            }
        }
        break;

        case OTA_QUERYIMAGE:
        {
            uint8 i;
            uint8 j;
            for(i=0; i < u8Discovered; i++)
            {
                if(sDiscovedOTAServers[i].bValid)
                {

                    tsOTA_ImageHeader          sOTAHeader;
                    eOTA_GetCurrentOtaHeader(OTA_CLIENT_EP,FALSE,&sOTAHeader);
                    DBG_vPrintf(TRACE_APP_OTA,"\n\nFile ID = 0x%08x\n",sOTAHeader.u32FileIdentifier);
                    DBG_vPrintf(TRACE_APP_OTA,"Header Ver ID = 0x%04x\n",sOTAHeader.u16HeaderVersion);
                    DBG_vPrintf(TRACE_APP_OTA,"Header Length ID = 0x%04x\n",sOTAHeader.u16HeaderLength);
                    DBG_vPrintf(TRACE_APP_OTA,"Header Control Filed = 0x%04x\n",sOTAHeader.u16HeaderControlField);
                    DBG_vPrintf(TRACE_APP_OTA,"Manufac Code = 0x%04x\n",sOTAHeader.u16ManufacturerCode);
                    DBG_vPrintf(TRACE_APP_OTA,"Image Type = 0x%04x\n",sOTAHeader.u16ImageType);
                    DBG_vPrintf(TRACE_APP_OTA,"File Ver = 0x%08x\n",sOTAHeader.u32FileVersion);
                    DBG_vPrintf(TRACE_APP_OTA,"Stack Ver = 0x%04x\n",sOTAHeader.u16StackVersion);
                    DBG_vPrintf(TRACE_APP_OTA,"Image Len = 0x%08x\n\n",sOTAHeader.u32TotalImage);

                    /*Set server address */
                    eOTA_SetServerAddress(
                                        OTA_CLIENT_EP,
                                        sDiscovedOTAServers[i].u64IeeeAddrOfServer,
                                        sDiscovedOTAServers[i].u16NwkAddrOfServer
                                        );

                    for (j=0; j< sDiscovedOTAServers[i].u8MatchLength;j++)
                    {
                        eClientQueryNextImageRequest(
                                OTA_CLIENT_EP,
                                sDiscovedOTAServers[i].u8MatchList[j],
                                sDiscovedOTAServers[i].u16NwkAddrOfServer,
                                sOTAHeader.u32FileVersion+1,
                                0,
                                sOTAHeader.u16ImageType,
                                sOTAHeader.u16ManufacturerCode,
                                0
                                );
                    }
                    OTA_State = OTA_DL_PROGRESS;
                }
            }
        }
        break;
        case OTA_DL_PROGRESS:
        {
            vManageDLProgressState();
        }
        break;
        default :
            break;
    }
}
/****************************************************************************
 *
 * NAME: vManageDLProgressState
 *
 * DESCRIPTION:
 * Manages the DL progress state, mainly to get the state machine out of it
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vManageDLProgressState(void)
{
    uint32 u32RandComponent;
    u32TimeOut++;
    u32RandComponent = RND_u32GetRand(RAND_TIMEOUT_MIN_IN_SEC,RAND_TIMEOUT_MAX_IN_SEC);
    if(u32TimeOut > (OTA_DL_IN_PROGRESS_TIME_IN_SEC + u32RandComponent))
    {
        u32TimeOut=0;
        OTA_State = OTA_FIND_SERVER;
        vRestetOTADiscovery();
    }
}

/****************************************************************************
 *
 * NAME: vRestetOTADiscovery
 *
 * DESCRIPTION:
 * Resets OTA discovery so that a fresh discovery is possible
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vRestetOTADiscovery(void)
{
    memset(sDiscovedOTAServers,0,sizeof(sDiscovedOTAServers));
    u8Discovered=0;
    u8Examined=0;
}

/****************************************************************************
 *
 * NAME: vOTAPersist
 *
 * DESCRIPTION:
 * Persists OTA data when called by the OTA client ebvent, this is required to
 * restore the down load in case of power failure.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PRIVATE void vOTAPersist(void)
{
    sOTA_PersistedData = sGetOTACallBackPersistdata();
    PDM_vSaveRecord(&OTA_PersistedDataPDDesc);
}

#ifdef CLD_TIME
/****************************************************************************
 *
 * NAME: eOTAReadTimeFromServer
 *
 * DESCRIPTION:
 * Reads the time attribute from the server
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE teZCL_Status eOTAReadTimeFromServer(tsZCL_Address *psDestinationAddress,uint8 u8DestinationEndPointId )
{
    teZCL_Status               eZCL_Status;
    uint8                      u8TransactionSequenceNumber;
    uint16                     au16AttributeRequestList[] = {E_CLD_TIME_ATTR_ID_TIME};

    eZCL_Status = eZCL_SendReadAttributesRequest(
                    OTA_CLIENT_EP,                              //uint8                       u8SourceEndPointId,
                    u8DestinationEndPointId,                    //uint8                       u8DestinationEndPointId,
                    GENERAL_CLUSTER_ID_TIME,                    //uint16                      u16ClusterId,
                    FALSE,                                      //bool_t                      bDirectionIsServerToClient,
                    psDestinationAddress,                       //tsZCL_Address              *psDestinationAddress,
                    &u8TransactionSequenceNumber,               //uint8                      *pu8TransactionSequenceNumber,
                    1,                                          //uint8                       u8NumberOfAttributesInRequest,
                    FALSE,                                      //bool_t                      bIsManufacturerSpecific,
                    0x1037,                                     //uint16                      u16ManufacturerCode,
                    au16AttributeRequestList);                  //uint16                     *pu16AttributeRequestList);
    DBG_vPrintf(TRACE_APP_OTA,"eZCL_SendReadAttributesRequest = %d \n",eZCL_Status);
    return eZCL_Status;
}
#endif

/****************************************************************************
 *
 * NAME: App_TimeReadJitterTask
 *
 * DESCRIPTION:
 * OS Task activated through the timer App_TimeReadJitterTimer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(App_TimeReadJitterTask)
{
#ifdef CLD_TIME
    teZCL_Status eZCL_Status;
    uint16 u16JitterTimeInMS=1000;
    u8MaxReadReties--;
    OS_eStopSWTimer(App_TimeReadJitterTimer);
    if( !bZCL_GetTimeHasBeenSynchronised() && u8MaxReadReties )
    {
        eZCL_Status = eOTAReadTimeFromServer(&(sOTA_PersistedData.sDestinationAddress),sOTA_PersistedData.u8DstEndpoint);
        DBG_vPrintf(TRACE_APP_OTA,"eOTAReadTimeFromServer = %d \n",eZCL_Status);
        u16JitterTimeInMS = (uint16)RND_u32GetRand(1000,3000);
        OS_eStartSWTimer(App_TimeReadJitterTimer,APP_TIME_MS(u16JitterTimeInMS),NULL);
    }
    else
    {
        if(!bZCL_GetTimeHasBeenSynchronised())
        {
            /* No time with OTA , hence we should make the next block request as soon as possible.*/
            DBG_vPrintf(TRACE_APP_OTA,"Reties are over Time not Sync\n");

            switch(sOTA_PersistedData.sAttributes.u8ImageUpgradeStatus)
            {
                case E_CLD_OTA_STATUS_COUNT_DOWN:
                case E_CLD_OTA_STATUS_WAIT_TO_UPGRADE:
                case E_CLD_OTA_STATUS_DL_IN_PROGRESS:
                    {
                        /* Get the pointer to Cluster custom data struct */
                        tsOTA_PersistedData * psOTAPersistedData = psGetOTACallBackPersistdata();

                        if(psOTAPersistedData->u32RequestBlockRequestTime !=0)
                        {
                            /*Set the request time in past to resume the download immediately*/
                            DBG_vPrintf(TRACE_APP_OTA,"Before reset of Block Request time =%d\n",psOTAPersistedData->u32RequestBlockRequestTime);
                            psOTAPersistedData->u32RequestBlockRequestTime=1;
                            psOTAPersistedData->u8Retry = 0;
                            DBG_vPrintf(TRACE_APP_OTA,"Reset Block Request time =%d\n",psOTAPersistedData->u32RequestBlockRequestTime);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
#endif
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
