/*
  @file test_read_rmap.c
  @author Juan Manuel Gómez
  @brief Demostration RMAP packet to configure GR718 
  @details Demostrates how to configure GR718B using RMAP packets. The   
  configuration port is at SpaceWire Addr 0 and support RMAP
  target packet. The Register are memory mapped, and could be
  initalized through RMAP Write commands.
  @param No parammeters needed.
  @example ./test_rmap  
  @copyright jmgomez CSIC-IAA
*/

#include <stdio.h>
#include <stdlib.h>
#include "utility.h"
#include "star-dundee_types.h"
#include "star-api.h"
#include "cfg_api_router.h"
#include "cfg_api_mk2.h"
#include "cfg_api_mk2_types.h"
#include "cfg_api_brick_mk3.h"
#include "rmap_packet_library.h"

#define VERSION_INFO "star-system_test v2.0"

#include <errno.h>

#define STAR_INFINITE 30000

#define _TX_INTERFACE 1
#define _RX_INTERFACE 2
#define _PACKET_SIZE 64

#define _TX_BAUDRATE_MUL 2
#define _TX_BAUDRATE_DIV 4
#define _RX_BAUDRATE_MUL 2
#define _RX_BAUDRATE_DIV 4

#define _ADDRESS_PATH 2
#define _ADDRESS_PATH_SIZE 1

void printPacket( STAR_SPACEWIRE_PACKET * StreamItemPacket){
    unsigned char* pTxStreamData = NULL;
    STAR_SPACEWIRE_ADDRESS *pStreamItemAddress = NULL;
    unsigned int pTxStreamDataSize = 0, i;
    pTxStreamData = STAR_getPacketData(
                    (STAR_SPACEWIRE_PACKET *)StreamItemPacket,
                    &pTxStreamDataSize);	

    pStreamItemAddress =  STAR_getPacketAddress ( (STAR_SPACEWIRE_PACKET *)StreamItemPacket);
/*
    printf("Packet  Address Path [Size: %d]\n", pStreamItemAddress->pathLength);
    printf("===================\n");
    for(i=0; i < pStreamItemAddress->pathLength ; ++i){
    	printf( "\t0x%x" ,pStreamItemAddress->pPath[i]);
	if (!((i+1) % 8 ) ){
	    printf("\n");
	}
    }
*/
	    printf("\n");

    printf("Packet  Data\n");
    printf("===================\n");
    for ( i= 0; i < pTxStreamDataSize; ++i){
    	printf( "\t0x%x" ,pTxStreamData[i]);
	if (!((i+1) % 8 ) ){
	    printf("\n");
	}
    }

    STAR_destroyAddress(pStreamItemAddress);
    STAR_destroyPacketData(pTxStreamData);
}


unsigned long printRxPackets(STAR_TRANSFER_OPERATION * const pTransferOp)
{
  unsigned long i;
  unsigned int rxPacketCount;
  STAR_STREAM_ITEM *pRxStreamItem;

  /* Get the number of traffic items received */
  rxPacketCount = STAR_getTransferItemCount(pTransferOp);
  if (rxPacketCount == 0)
    {
      printf("No packets received.\n");
    }
  else
    {
      /* For each traffic item received */
      for (i = 0U; i < rxPacketCount; i++)
        {
	  /* Get the packet */
	  pRxStreamItem = STAR_getTransferItem(pTransferOp, i);
	  if ((pRxStreamItem == NULL) || (pRxStreamItem->itemType !=
					  STAR_STREAM_ITEM_TYPE_SPACEWIRE_PACKET) ||
	      (pRxStreamItem->item == NULL))
            {
	      printf("\nERROR received an unexpected traffic type, or empty traffic item in item %lu\n",
		     i);
            }
	  else
            {
	      printPacket( (STAR_SPACEWIRE_PACKET *)pRxStreamItem->item);
            }
        }
    }

  /* Return the error count */
  return rxPacketCount;
}


/******************************************************************/
/*                                                                */
/*****************************************************************/
int __cdecl main(int argc, char *argv[]){

  STAR_DEVICE_ID* devices;
  STAR_DEVICE_ID deviceId;
  U32 deviceCount = 0U, i;

  STAR_CHANNEL_MASK channelMask;
  STAR_CFG_MK2_HARDWARE_INFO hardwareInfo;
  char versionStr[STAR_CFG_MK2_VERSION_STR_MAX_LEN];
  char buildDateStr[STAR_CFG_MK2_BUILD_DATE_STR_MAX_LEN];

  unsigned int txChannelNumber, rxChannelNumber;
  STAR_CHANNEL_ID rxChannelId = 0U, txChannelId = 0U;
  STAR_CFG_MK2_BASE_TRANSMIT_CLOCK clockRateParams;


  /***************************************************************/
  /*        Configuration                                        */
  /*                                                             */
  /* Channel 1 = Transmit interface                              */
  /* Channel 2 = Receive  interface                              */
  /* BaudRate  = 100 Mbps (100*2/4)*2                            */
  /* Packet Size = 1 KB                                          */
  /***************************************************************/

  txChannelNumber = _TX_INTERFACE;
  rxChannelNumber = _RX_INTERFACE;
  clockRateParams.multiplier = _TX_BAUDRATE_MUL;
  clockRateParams.divisor = _TX_BAUDRATE_DIV;
  

  /* Get the list of devices of the specified type */
  devices = STAR_getDeviceListForType(STAR_DEVICE_TXRX_SUPPORTED , &deviceCount);
  if (devices == NULL){
    puts("No device found\n");	
    return 0;
  } 

  /* We are working with the first device, what if I have more? */
  deviceId = devices[0];	
  if (deviceId == STAR_DEVICE_UNKNOWN){
    puts("Error dispositivo desconocido.\n");
    return 0;
  }

  /* Get hardware info*/
  CFG_MK2_getHardwareInfo(deviceId, &hardwareInfo);
  CFG_MK2_hardwareInfoToString(hardwareInfo, versionStr, buildDateStr);
  /* Display the hardware info*/
  printf("\nVersion: %s", versionStr);
  printf("\nBuildDate: %s", buildDateStr);	
  // Flashes the LEDS.
  if (CFG_MK2_identify(deviceId) == 0)
    {
      puts("\nERROR: Unable to identify device");
      return 0U;
    }

  /* Get the channels present on the device  */
  /* We need at least 2 Channels (0x00000007) */
  channelMask = STAR_getDeviceChannels(deviceId);
  if (channelMask != 7U)
    {
      puts("The device does not have channel 1 and 2 available.\n");
      return 0U;
    }

  int status_link = 0;
  status_link = CFG_BRICK_MK3_setBaseTransmitClock(deviceId, txChannelNumber, clockRateParams);	
  if (status_link == 0){
    puts("Error setting the Tx link speed.\n");	
  }

  status_link = CFG_BRICK_MK3_setBaseTransmitClock(deviceId, rxChannelNumber, clockRateParams );
  if (status_link == 0){
    puts("Error setting the RX link speed.\n");	
  }

  /* Open channel 1 for Transmit*/
  txChannelId = STAR_openChannelToLocalDevice(deviceId, STAR_CHANNEL_DIRECTION_INOUT,
					      txChannelNumber, TRUE);
  if (txChannelNumber == 0U)
    {
      puts("\nERROR: Unable to open TX channel 1\n");
      return 0;
    }
  /* Open channel 2 for Receipt*/
  rxChannelId = STAR_openChannelToLocalDevice(deviceId, STAR_CHANNEL_DIRECTION_IN,
					      rxChannelNumber, TRUE);
  if (rxChannelId == 0U)
    {
      puts("\nERROR: Unable to open RX channel 2\n");
      return 0;
    }

  puts("Channels Opened.\n");	
	
  /*****************************************************************/
  /*    Allocate memory for the transmit buffer and construct      */
  /*    the packet to transmit.                                    */
  /*****************************************************************/
  unsigned long byteSize = 0;

  void *pFillPacket, *pBuildPacket;
  unsigned long fillPacketLenCalculated, fillPacketLen;  
  unsigned long buildPacketLen;
  U8 pTarget[] = {0,254};
  U8 pReply[] = {254};
  U8 pData[] = {0x00, 0x14, 0x02, 0x2E};
  char status;

  byteSize = _PACKET_SIZE;

				       
  /* Calculate the length of a write command packet, with 4 bytes of data */
  fillPacketLenCalculated = RMAP_CalculateReadCommandPacketLength(2, 1,1);
  printf("The Read command packet will have a length of %ld bytes\n",
	 fillPacketLenCalculated);
  /* Allocate memory for the packet */
  pFillPacket = malloc(fillPacketLenCalculated);
  if (!pFillPacket)
    {
      puts("Couldn't allocate the memory for the command packet");
      return;
    }

  /* Fill the memory with the packet */
  /*

    status = RMAP_FillReadCommandPacket(pTarget, 2, pReply, 1, 0, 0x20, 0,
        0x106, 0, 4, &fillPacketLen, NULL, 1, (U8 *)pFillPacket,
        fillPacketLenCalculated);
   */
  status = RMAP_FillReadCommandPacket(pTarget, 2, pReply, 1, 0, 0x00,
				      0, 0x198, 0, 4, &fillPacketLen, NULL, 1, (U8 *)pFillPacket,
				       fillPacketLenCalculated);
  if (!status)
    {
      puts("Couldn't fill the write command packet");
      free(pFillPacket);
      return;
    }
  
  STAR_TRANSFER_OPERATION *pTxTransferOp = NULL, *pRxTransferOp = NULL;
  STAR_STREAM_ITEM *pTxStreamItem = NULL;
   
  /* Create the receive operations. */
  pRxTransferOp = STAR_createRxOperation(1, STAR_RECEIVE_PACKETS);
  if (pRxTransferOp == NULL)
    {
      puts("\nERROR: Unable to create receive operation");
      return 0;
    }


  /* Create the packet to be transmitted */
  //  pTxStreamItem = STAR_createPacket(NULL, (U8 *)pBuildPacket, buildPacketLen,
  //				    STAR_EOP_TYPE_EOP);

  pTxStreamItem = STAR_createPacket(NULL, (U8 *)pFillPacket, fillPacketLen,
				    STAR_EOP_TYPE_EOP);

  if (pTxStreamItem == NULL)
    {
      puts("\nERROR: Unable to create the packet to be transmitted");
      return 0;
    }

  /* Create the transmit transfer operation for the packet */
  pTxTransferOp = STAR_createTxOperation(&pTxStreamItem, 1U);
  if (pTxTransferOp == NULL)
    {
      puts("\nERROR: Unable to create the transfer operation to be transmitted");
      return 0;
    }

  /***************************************************************/
  /*    Submit the operations                                    */
  /*                                                             */
  /***************************************************************/

  /* Submit the transmit operation */
  if (STAR_submitTransferOperation(txChannelId, pTxTransferOp) == 0)
    {
      printf("\nERROR occurred during transmit.  Test failed.\n");
      return 0;
    }

  //Wait the operations to finish.
  STAR_TRANSFER_STATUS rxStatus, txStatus;

  /* Wait on the transmit operation completing */
  txStatus = STAR_waitOnTransferOperationCompletion(pTxTransferOp,
						    STAR_INFINITE);
  if(txStatus != STAR_TRANSFER_STATUS_COMPLETE)
    {
      printf("\nERROR occurred during transmit.  Test failed.\n");
      return 0;
    }


  /* Submit the receive operation */
  if (STAR_submitTransferOperation(txChannelId, pRxTransferOp) == 0)
    {
      printf("\nERROR occurred during receive.  Test Tfailed.\n");
      return 0;
    }


  /* Wait on the receive operation completing */
  rxStatus = STAR_waitOnTransferOperationCompletion(pRxTransferOp,
						    STAR_INFINITE);
  if (rxStatus != STAR_TRANSFER_STATUS_COMPLETE)
    {
      printf("\nERROR occurred during receive.  Test failed.\n");
      return 0;
    }

  printRxPackets((STAR_TRANSFER_OPERATION *)  pRxTransferOp);


  /****************************************************************/
  /*    Free the resource                                         */
  /*                                                              */
  /****************************************************************/

  /* Dispose of the transfer operations */
  if (pTxTransferOp != NULL)
    {
      STAR_disposeTransferOperation(pTxTransferOp);
    }

  if (pRxTransferOp != NULL)
    {
      STAR_disposeTransferOperation(pRxTransferOp);
    }


  if (pFillPacket != NULL){
    RMAP_FreeBuffer(pFillPacket);
    }
  /* Close the channels */
  if (rxChannelId != 0U)
    {
      STAR_closeChannel(rxChannelId);
    }

  if (txChannelId != 0U)
    {
      STAR_closeChannel(txChannelId);
    }

  return 0;
}





