char gps_buffer[128];
uint8_t gps_offset=0;

char gpzda_buffer[128];
uint8_t gpzda_offset=0;
uint8_t gpzda_bytes=0;
uint8_t gpzda_checksum=0;
uint8_t gpzda_recieved_checksum=0;
boolean gpzda_incoming=FALSE;
boolean gpzda_waiting=FALSE;
boolean gpzda_checksum_error=0;

char gpgga_buffer[64];
uint8_t gpgga_offset=0;
uint8_t gpgga_bytes=0;
uint8_t gpgga_checksum=0;
uint8_t gpgga_recieved_checksum=0;
boolean gpgga_incoming=FALSE;
boolean gpgga_waiting=FALSE;
boolean gpgga_checksum_error=0;

boolean pps_waiting=FALSE;

#IFDEF PPS
#INT_EXT
void pps_interrupt(void)
{
	wallclock_inc_second();
	#IFDEF OUTPUT_PPS
	fprintf(COM1,".");
	#ENDIF
	pps_waiting=TRUE;
}
#ENDIF

void pps_feedback(void)
{
	remote_feedback();
	if(gps_fix==0)
	{
		fprintf(COM1,"NO FIX!\r\n");
	}
	else
	{
		if(gps_fix==1) fprintf(COM1,"GPS FIX");
		else if(gps_fix==2) fprintf(COM1,"DGPS FIX");
		if(satellite_count<80) fprintf(COM1," WTIH %u SATELLITES\r\n",satellite_count);
		else fprintf(COM1,"\r\n");
	}
	pps_waiting=FALSE;
}

#INT_EXT1
void check_fix(void)
{
	//gps_fix=input(PIN_B1);
}

#INT_RDA2
void gps_message(void)
{
	gps_buffer[gps_offset]=fgetc(COM2);
	#IFDEF OUTPUT_ALL_GPS
	fprintf(COM1,"%c",gps_buffer[gps_offset]);
	#ENDIF
	if(gpzda_incoming)
	{
		gpzda_buffer[gpzda_offset]=gps_buffer[gps_offset];
		if(gpzda_buffer[gpzda_offset]==0x0d)
		{
			gpzda_incoming=FALSE;
			gpzda_waiting=TRUE;
			gps_offset=0;
			memset(gps_buffer, 0, sizeof(gps_buffer));
		}
		else
		{
			gps_offset++;
			gpzda_offset++;
		}
	}
	else if(gpgga_incoming)
	{
		gpgga_buffer[gpgga_offset]=gps_buffer[gps_offset];
		if(gpgga_buffer[gpgga_offset]==0x0d)
		{
			gpgga_incoming=FALSE;
			gpgga_waiting=TRUE;
			gps_offset=0;
			memset(gps_buffer, 0, sizeof(gps_buffer));
		}
		else
		{
			gps_offset++;
			gpgga_offset++;
		}
	}
	else
	{
		if((gps_offset==0)&&(gps_buffer[gps_offset]=='$')) gps_offset++;
		else if((gps_offset==1)&&(gps_buffer[gps_offset]=='G')) gps_offset++;
		else if((gps_offset==2)&&(gps_buffer[gps_offset]=='P')) gps_offset++;
		else if((gps_offset==3)&&((gps_buffer[gps_offset]=='Z')||(gps_buffer[gps_offset]=='G'))) gps_offset++;
		else if((gps_offset==4)&&((gps_buffer[gps_offset]=='D')||(gps_buffer[gps_offset]=='G'))) gps_offset++;
		else if((gps_offset==5)&&(gps_buffer[gps_offset]=='A'))
		{
			gps_offset++;
			if(gps_buffer[gps_offset-3]=='Z')
			{
				gpzda_incoming=TRUE;
				gpzda_offset=gps_offset;
				uint8_t i=0;
				while(i<gpzda_offset)
				{
					gpzda_buffer[i]=gps_buffer[i];
 					i++;
				}
			}
			else if(gps_buffer[gps_offset-3]=='G')
			{
				gpgga_incoming=TRUE;
				gpgga_offset=gps_offset;
				uint8_t i=0;
				while(i<gpgga_offset)
				{
					gpgga_buffer[i]=gps_buffer[i];
 					i++;
				}
			}
		}
		else
		{
			gps_offset=0;
			memset(gps_buffer, 0, sizeof(gps_buffer));
		}
	}
}

void process_gpzda(void)
{
	gpzda_waiting=FALSE;
	// Calculate checksum
	uint8_t i=0;
	gpzda_checksum=0x00;
	#IFDEF OUTPUT_NMEA
	fprintf(COM1,"%c",gpzda_buffer[i]);
	#ENDIF
	while(i<gpzda_offset-4)
	{
		i++;
		#IFDEF OUTPUT_NMEA
		fprintf(COM1,"%c",gpzda_buffer[i]); 
		#ENDIF
		gpzda_checksum^=gpzda_buffer[i];
	}
	#IFDEF OUTPUT_NMEA
	fprintf(COM1,"*%X\r\n",gpzda_checksum);
	#ENDIF

	time.hour=(((uint8_t)gpzda_buffer[7]-48)*10)+((uint8_t)gpzda_buffer[8]-48);
	time.minute=(((uint8_t)gpzda_buffer[9]-48)*10)+((uint8_t)gpzda_buffer[10]-48);
	time.second=(((uint8_t)gpzda_buffer[11]-48)*10)+((uint8_t)gpzda_buffer[12]-48);
	if((gps_fix==0)||(satellite_count<4)) time.second_100=(((uint8_t)gpzda_buffer[14]-48)*10)+((uint8_t)gpzda_buffer[15]-48);
	time.day=(((uint8_t)gpzda_buffer[18]-48)*10)+((uint8_t)gpzda_buffer[19]-48);
	time.month=(((uint8_t)gpzda_buffer[21]-48)*10)+((uint8_t)gpzda_buffer[22]-48);
	time.year=(((uint16_t)gpzda_buffer[24]-48)*1000)+(((uint16_t)gpzda_buffer[25]-48)*100)+(((uint16_t)gpzda_buffer[26]-48)*10)+((uint16_t)gpzda_buffer[27]-48);	

	gpzda_offset=0;
	memset(gpzda_buffer, 0, sizeof(gpzda_buffer));
}

void process_gpgga(void)
{
	gpgga_waiting=FALSE;
	// Calculate checksum
	uint8_t i=0;
	gpgga_checksum=0x00;
	#IFDEF OUTPUT_NMEA
	fprintf(COM1,"%c",gpgga_buffer[i]);
	#ENDIF
	while(i<gpgga_offset-4)
	{
		i++;
		#IFDEF OUTPUT_NMEA
		fprintf(COM1,"%c",gpgga_buffer[i]);
		#ENDIF
		gpgga_checksum^=gpgga_buffer[i];
	}
	#IFDEF OUTPUT_NMEA
	fprintf(COM1,"*%X\r\n",gpgga_checksum);
	#ENDIF

	i=0;
	uint8_t gpgga_field=0;
	while(i<gpgga_offset-4)
	{
		if(gpgga_buffer[i]==',') gpgga_field++;
		i++;
		if(gpgga_field==6) break;
	}

	gps_fix=((uint8_t)gpgga_buffer[i]-48);

	if(gps_fix==0)
	{
		fprintf(COM1,"NO FIX!\r\n");
		satellite_count=0;
	}
	else
	{
		if(((uint8_t)gpgga_buffer[i+2]-48)<10)
		{
			satellite_count=(((uint8_t)gpgga_buffer[i+2]-48)*10)+((uint8_t)gpgga_buffer[i+3]-48);
		}
		else
		{
			satellite_count=0xFF;
		}
	}

	gpgga_offset=0;
	memset(gpgga_buffer, 0, sizeof(gpgga_buffer));
}