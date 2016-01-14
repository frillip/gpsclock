void init_display(void);
void update_display0(char);
void update_display1(char);
void toggle_colon(void);

#define DISP_BRIGHTEST 0xFF

boolean colon_state=FALSE;
boolean display_mode=1;
boolean mode_switch=FALSE;
boolean toggle_waiting=FALSE;
uint8_t display_brightness=DISP_BRIGHTEST;

void update_display0(void)
{
	output_low(DISP0_SS);
	#asm nop #endasm

	if(display_mode==0)		// HHMMSSss
	{
		// Hours
		spi_write(time.hour/10);
		spi_write(time.hour%10);
	
		// Minutes
		spi_write(time.minute/10);
		spi_write(time.minute%10);
	}
	else if(display_mode==1)		// YYYYMMDD
	{
		// Years
		spi_write(time.year/1000);
		spi_write((time.year%1000)/100);
		spi_write((time.year%100)/10);
		spi_write(time.year%10);
	}
	// Deselect display
	#asm nop #endasm
	output_high(DISP0_SS);
}

void update_display1(void)
{
	output_low(DISP1_SS);
	#asm nop #endasm

	if(display_mode==0)		// HHMMSSss
	{
		// Seconds
		spi_write(time.second/10);
		spi_write(time.second%10);
	
		// Hundreths of a second
		spi_write(time.second_100/10);
		spi_write(time.second_100%10);
	}
	else if(display_mode==1)		// YYYYMMDD
	{
		// Months
		spi_write(time.month/10);
		spi_write(time.month%10);
		// Days
		spi_write(time.day/10);
		spi_write(time.day%10);
	}
	// Deselect display
	#asm nop #endasm
	output_high(DISP1_SS);
}

void init_display(void)
{
	// Set SS high
	output_high(DISP0_SS);
	output_high(DISP1_SS);
	// SPI at 250kHz
	setup_spi(SPI_MASTER|SPI_L_TO_H|SPI_XMIT_L_TO_H|SPI_CLK_T2);
	// Wait for both displays to boot
	delay_ms(100);

	// Reset display0, turn on colon
	output_low(DISP0_SS);
	output_low(DISP1_SS);
	#asm nop #endasm
	spi_write(0x82);
	spi_write(0x00);
	// reset
	spi_write(0x76);
	// dots
	spi_write(0x77);
	// colon
	spi_write(0x02);
	// max brightness
	spi_write(0x7A);
	spi_write(DISP_BRIGHTEST);
	#asm nop #endasm
	output_high(DISP0_SS);
	output_high(DISP1_SS);

	// fill with initial time (force)
	output_low(DISP0_SS);
	#asm nop #endasm
	spi_write(0x76);
	#asm nop #endasm
	output_high(DISP0_SS);
	delay_us(10);
	update_display0();

	output_low(DISP1_SS);
	#asm nop #endasm
	spi_write(0x76);
	#asm nop #endasm
	output_high(DISP1_SS);
	delay_us(10);
	update_display1();
}

void toggle_colon(void)
{
	if(display_mode==0)
	{
		colon_state = ! colon_state;
		// Display0
		output_low(DISP0_SS);
		#asm nop #endasm
		// Dots
		spi_write(0x77);
		// Colon or no colon
		spi_write(((uint8_t)colon_state<<1)|((uint8_t)colon_state<<3));
		// Deselect display
		#asm nop #endasm
		output_high(DISP0_SS);
	
		// Display1
		output_low(DISP1_SS);
		#asm nop #endasm
		// Dots
		spi_write(0x77);
		// Colon or no colon
		spi_write((uint8_t)colon_state<<1);
		// Deselect display
		#asm nop #endasm
		output_high(DISP1_SS);
	}
	if(display_mode==1)
	{
		// Display0
		output_low(DISP0_SS);
		#asm nop #endasm
		// Dots
		spi_write(0x77);
		// Colon or no colon
		spi_write(0x08);
		// Deselect display
		#asm nop #endasm
		output_high(DISP0_SS);
	
		// Display1
		output_low(DISP1_SS);
		#asm nop #endasm
		// Dots
		spi_write(0x77);
		// Colon or no colon
		spi_write(0x02);
		// Deselect display
		#asm nop #endasm
		output_high(DISP1_SS);
	}
}

void update_brightness(void)
{
	output_low(DISP0_SS);
	output_low(DISP1_SS);
	#asm nop #endasm
	spi_write(0x7A);
	spi_write(display_brightness);
	#asm nop #endasm
	output_high(DISP0_SS);
	output_high(DISP1_SS);
}