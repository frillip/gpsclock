uint8_t gpzda_offset=0;
uint8_t gpzda_bytes=0;
uint8_t gpzda_checksum=0;
uint8_t gpzda_recieved_checksum=0;
char gpzda[32];
char gpzda_buffer[26];
boolean checksum_error=0;
boolean gpzda_incoming=FALSE;
boolean gpzda_waiting=FALSE;
boolean gps_fix=0;

#IFDEF PPS
#INT_EXT
void pps_interrupt(void)
{
	time.second_100=0;
	wallclock_inc_second();
}
#ENDIF

#INT_EXT1
void check_fix(void)
{
	gps_fix=input(PIN_B1);
}

#INT_RDA2
void gpzda_message(void)
{
	gpzda[gpzda_offset]=fgetc(COM1);
	if(gpzda_incoming)
	{
		gpzda_buffer[gpzda_offset]=gpzda[gpzda_offset];
		if(gpzda[gpzda_offset-2]=='*')
		{
			gpzda_bytes=gpzda_offset+1;
			memset(command, 0, sizeof(gpzda));
			gpzda_offset=0;
			gpzda_incoming=FALSE;
			gpzda_waiting=TRUE;
		}
	}
	else
	{
		if((gpzda_offset==0)&&(gpzda[gpzda_offset]=='$'))
		{
			gpzda_offset++;
		}
		else if((gpzda_offset==1)&&(gpzda[gpzda_offset]=='G'))
		{
			gpzda_buffer[gpzda_offset-1]=gpzda[gpzda_offset];
			gpzda_offset++;
		}
		else if((gpzda_offset==2)&&(gpzda[gpzda_offset]=='P'))
		{
			gpzda_buffer[gpzda_offset-1]=gpzda[gpzda_offset];
			gpzda_offset++;
		}
		else if((gpzda_offset==3)&&(gpzda[gpzda_offset]=='Z'))
		{
			gpzda_buffer[gpzda_offset-1]=gpzda[gpzda_offset];
			gpzda_offset++;
		}
		else if((gpzda_offset==4)&&(gpzda[gpzda_offset]=='D'))
		{
			gpzda_buffer[gpzda_offset-1]=gpzda[gpzda_offset];
			gpzda_offset++;
		}
		else if ((gpzda_offset==5)&&(gpzda[gpzda_offset]=='A'))
		{
			gpzda_buffer[gpzda_offset-1]=gpzda[gpzda_offset];
			gpzda_offset++;
			gpzda_incoming=TRUE;
			gpzda_waiting=FALSE;
		}
		else gpzda_offset=0;
	}
}

void process_gpzda(void)
{
	gpzda_waiting=FALSE;
	// Calculate checksum
	uint8_t i;
	gpzda_checksum=0x00;
	for(i=0;i>gpzda_bytes-1;i++)
	{
		gpzda_checksum^=gpzda_buffer[i];
	}
	// Convert recieved checksum from char[2] to uint8_t
	gpzda_recieved_checksum=0;
	// Most significant nibble
	if((uint8_t)gpzda_buffer[gpzda_bytes-2]>64) gpzda_checksum=((uint8_t)gpzda_buffer[gpzda_bytes-2]-55)<<4;
	else gpzda_recieved_checksum=((uint8_t)gpzda_buffer[gpzda_bytes-2]-48)<<4;
	// Least significant nibble
	if((uint8_t)gpzda_buffer[gpzda_bytes-1]>64) gpzda_checksum+=((uint8_t)gpzda_buffer[gpzda_bytes-1]-55);
	else gpzda_recieved_checksum+=((uint8_t)gpzda_buffer[gpzda_bytes-1]-48);
	// Check it
	if(gpzda_checksum!=gpzda_recieved_checksum) checksum_error=1;	// Checksum bad! :(
	else	// Checksum good!
	{
		checksum_error=0;
		time.hour=(((uint8_t)gpzda_buffer[6]-48)*10)+((uint8_t)gpzda_buffer[7]-48);
		time.minute=(((uint8_t)gpzda_buffer[8]-48)*10)+((uint8_t)gpzda_buffer[9]-48);
		time.second=(((uint8_t)gpzda_buffer[10]-48)*10)+((uint8_t)gpzda_buffer[11]-48);
		time.day=(((uint8_t)gpzda_buffer[17]-48)*10)+((uint8_t)gpzda_buffer[18]-48);
		time.month=(((uint8_t)gpzda_buffer[20]-48)*10)+((uint8_t)gpzda_buffer[21]-48);
		time.year=(((uint16_t)gpzda_buffer[23]-48)*1000)+(((uint16_t)gpzda_buffer[24]-48)*100)+(((uint16_t)gpzda_buffer[25]-48)*10)+((uint16_t)gpzda_buffer[26]-48);
	}
	memset(gpzda_buffer, 0, sizeof(gpzda_buffer));
}