#include "../src/usp.h"

int getPacket(struct dg_payload *pload, struct dg_payload *data_temp_buff, int seq_num, int windowSize)
{
	int i;
	for(i = 0; i < windowSize; i++)
	{
		if(data_temp_buff[i].seq_number == seq_num)
		{
			pload->ack = data_temp_buff[i].ack;
			strcpy(pload->buff, data_temp_buff[i].buff);
			pload->type = data_temp_buff[i].type;
			pload->seq_number = data_temp_buff[i].seq_number;
			pload->ts = data_temp_buff[i].ts;
			pload->windowSize = data_temp_buff[i].windowSize;
			data_temp_buff[i].type = USED;
			return 1;
		}
	}
	return 0;
}

int storePacket(struct dg_payload pload, struct dg_payload* data_temp_buff, int windowSize)
{
	int i;
	bool isPresent = false;
	for(i = 0; i < windowSize; i++)
	{
		if(pload.seq_number == data_temp_buff[i].seq_number)
		{
			isPresent = true;
			if(DEBUG)
				printf("Already present in buffer %d \n", data_temp_buff[i].seq_number);
			break;
		}
	}

	for(i = 0; i < windowSize && !isPresent; i++)
	{
		if(data_temp_buff[i].type == USED)
		{
			memset(data_temp_buff[i].buff,0,PACKET_SIZE);
			strcpy(data_temp_buff[i].buff, pload.buff);
			data_temp_buff[i].seq_number = pload.seq_number;
			data_temp_buff[i].type = pload.type;
			if(DEBUG)
				printf("Stored buffer %d \n", data_temp_buff[i].seq_number);
			return 1;
		}
	}

	return 0;
}

int isNewPacketPresent(struct dg_payload* data_temp_buff, int windowSize)
{
	int i;
	for(i = 0; i < windowSize; i++)
	{
		if(data_temp_buff[i].type != USED)
		{
			return 1;
		}
	}
	return 0;
}

void initializeTempBuff(struct dg_payload* data_temp_buff, int windowSize)
{
	int i;
	for(i = 0; i < windowSize; i++)
	{
		data_temp_buff[i].type = USED;
	}
}

int getUsedTempBuffSize(struct dg_payload* data_temp_buff, int windowSize)
{
	int count = 0;
	int i;
	for(i = 0; i < windowSize; i++)
	{
		if(data_temp_buff[i].type != USED)
		{
			count++;
		}
	}
	if(DEBUG)
		printTempBuff(data_temp_buff, windowSize);
	return count;
}

void printTempBuff(struct dg_payload* data_temp_buff, int windowSize)
{
	int count = 0;
	int i;
	printf("Temp buff has : ");
	for(i = 0; i < windowSize; i++)
	{
		if(data_temp_buff[i].type != USED)
		{
			count++;
			printf("%d ", data_temp_buff[i].seq_number);
		}
	}
	printf("\n");
}

char* getAckType(uint32_t ackType){
	printf("Ack type : %u\n", ackType);
	switch(ackType)
	{
		case ACK:
			return "ACK";
		case FIN_ACK:
			return "FIN_ACK";
		case WINDOW_PROBE:
			return "WINDOW_PROBE";
	}
}
