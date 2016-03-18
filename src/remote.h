void remote_init(void);	
void remote_command(void);
void utc_feedback(void);
void local_feedback(void);

uint8_t command_offset=0;
uint8_t command_timeout=0;
char command[128];
char command_buffer[128];

boolean command_incoming=FALSE;
boolean command_waiting=FALSE;
boolean command_complete=FALSE;

void clear_cmd_buffers()
{
	command_timeout=0;
	command_waiting=FALSE;
	command_complete=FALSE;
	memset(command, 0, sizeof(command_buffer));
	memset(command, 0, sizeof(command));
	command_offset=0;
}

void utc_feedback(void)
{
	if(!command_timeout)
	{
		fprintf(COM1,"%04lu-%02u-%02uT%02u:%02u:%02u",utc.year,utc.month,utc.day,utc.hour,utc.minute,utc.second);
		if(timezone.minus_flag) fprintf(COM1,"-");
		else fprintf(COM1,"+");
		fprintf(COM1,"%02u:%02u\r\n",timezone.hour,timezone.minute);
	}
}

void local_feedback(void)
{
	if(!command_timeout) fprintf(COM1,"%04lu-%02u-%02uT%02u:%02u:%02u\r\n",local.year,local.month,local.day,local.hour,local.minute,utc.second);
}

void local_time_feedback(void)
{
	if(!command_timeout) fprintf(COM1,"%02u:%02u:%02u\r\n",local.hour,local.minute,utc.second);
}

void local_date_feedback(void)
{
	if(!command_timeout) fprintf(COM1,"%04lu-%02u-%02u\r\n",local.year,local.month,local.day);
}

void brightness_feedback(void)
{
	if(!command_timeout) fprintf(COM1,"%02X\r\n",display_brightness);
}

uint8_t strcmp(unsigned char *s1, unsigned char *s2)
{
   for (; *s1 == *s2; s1++, s2++)
      if (*s1 == '\0')
         return(1);
   return(0);
}

uint8_t strncmp(unsigned char *s1, unsigned char *s2, uint8_t n)
{
   for (; n > 0; s1++, s2++, n--)
      if (*s1 != *s2)
         return(0);
      else if (*s1 == '\0')
         return(1);
   return(1);
}

#INT_RDA
void remote_command(void)
{
	command[command_offset]=fgetc(COM1);
	command_timeout=250;
	if(command[command_offset]==0xff)
	{
		delay_us(100);
		reset_cpu();
	}
	if(CMD_ECHO)fprintf(COM1,"%c", command[command_offset]);
	if(command[command_offset]==0x0d)
	{
		fprintf(COM1,"\n");
		if(!command_incoming) clear_cmd_buffers();
	}
	/*
	if(command[command_offset]==0x08)
	{
		if(command_offset)
		{
			command_offset--;
			if(command_offset<3) command_incoming=false;
			return;
		}
		else clear_cmd_buffers();
	}
	*/
	if(command_incoming)
	{
		command_buffer[command_offset-3]=command[command_offset];
		if((command[command_offset]==0x00)||(command[command_offset]==0x0d))
		{
			command_incoming=FALSE;
			command_waiting=TRUE;
			command[command_offset]=0x00;
			command_buffer[command_offset-3]=command[command_offset];
			memset(command, 0, sizeof(command));
			command_offset=0;
			return;
		}
		command_offset++;
		if(command_offset==128)
		{
			clear_cmd_buffers();
		}
	}
	else
	{
		if((command_offset==0)&&(command[0]=='A'))
		{
			command_offset++;
		}
		else if((command_offset==1)&&(command[1]=='T'))
		{
			command_offset++;
		}
		else if ((command_offset==2)&&(command[2]=='+'))
		{
			command_offset++;
			command_incoming=TRUE;
		}
		else command_offset=0;
	}
}

void process_command(void)
{
	command_waiting=FALSE;
	command_complete=FALSE;
	command_timeout=0;

	if(strcmp(command_buffer,"RESET"))
	{
		fprintf(COM1, "Resetting...\r\n");
		write_eeprom(EEPROM_RESET,0x42);			// Write reset flag
		write_eeprom(EEPROM_YEAR,(utc.year>>8));
		write_eeprom(EEPROM_YEAR+1,(utc.year&0x00FF));
		write_eeprom(EEPROM_MONTH,utc.month);
		write_eeprom(EEPROM_DAY,utc.day);
		write_eeprom(EEPROM_HOUR,utc.hour);
		write_eeprom(EEPROM_MINUTE,utc.minute);
		write_eeprom(EEPROM_SECOND,utc.second);	// Write current time to EEPROM
		delay_ms(10);
		reset_cpu();
	}

	else if(strncmp(command_buffer,"UTC",3))
	{
		utc_feedback();
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"LOCAL",5))
	{
		local_feedback();
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"TIME",4))
	{
		local_time_feedback();
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"DATE",4))
	{
		local_date_feedback();
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"BRIGHT",6))
	{
		if((command_buffer[8])&&(((uint8_t)command_buffer[6]-48)<3))display_brightness=(((uint8_t)command_buffer[6]-48)*100)+(((uint8_t)command_buffer[7]-48)*10)+((uint8_t)command_buffer[8]-48);
		else if(command_buffer[7])display_brightness=(((uint8_t)command_buffer[7]-48)*10)+((uint8_t)command_buffer[8]-48);
		else if(command_buffer[6])display_brightness=(uint8_t)command_buffer[6]-48;
		update_brightness();
		brightness_feedback();
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"TIMEZONE",8))
	{
		if(command_buffer[12])
		{
			if(command_buffer[8]=='-')timezone.minus_flag=TRUE;
			else timezone.minus_flag=FALSE;
			timezone.hour=(((uint8_t)command_buffer[9]-48)*10)+((uint8_t)command_buffer[10]-48);
			if(timezone.hour>14)timezone.hour=0;
			timezone.minute=(((uint8_t)command_buffer[11]-48)*10)+((uint8_t)command_buffer[12]-48);
			if(timezone.minute>45)timezone.minute=0;
			if((!timezone.hour)&&(!timezone.minute)) timezone.minus_flag=FALSE;
			write_eeprom(EEPROM_TZ_HOUR,timezone.hour);
			write_eeprom(EEPROM_TZ_MINUTE,timezone.minute);
		}
		calc_local_time();
		utc_feedback();
		local_feedback();
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"PPS",3))
	{
		OUTPUT_PPS=!OUTPUT_PPS;
		fprintf(COM1,"PPS output ");
		if(OUTPUT_PPS) fprintf(COM1,"enabled: ");
		else fprintf(COM1,"disabled: ");
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"FEEDBACK",8))
	{
		OUTPUT_FEEDBACK=!OUTPUT_FEEDBACK;
		fprintf(COM1,"Feedback output ");
		if(OUTPUT_FEEDBACK) fprintf(COM1,"enabled: ");
		else fprintf(COM1,"disabled: ");
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"NMEA",4))
	{
		OUTPUT_NMEA=!OUTPUT_NMEA;
		fprintf(COM1,"NMEA output ");
		if(OUTPUT_NMEA) fprintf(COM1,"enabled: ");
		else fprintf(COM1,"disabled: ");
		command_complete=TRUE;
	}

	else if(strncmp(command_buffer,"ALLGPS",6))
	{
		OUTPUT_ALL_GPS=!OUTPUT_ALL_GPS;
		fprintf(COM1,"GPS output ");
		if(OUTPUT_ALL_GPS) fprintf(COM1,"enabled: ");
		else fprintf(COM1,"disabled: ");
		command_complete=TRUE;
	}

	if(command_complete) fprintf(COM1,"OK\r\n\r\n");
	clear_cmd_buffers();
}