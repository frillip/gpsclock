void init_display(void);
void update_display0(char);
void update_display1(char);
void toggle_colon(void);

#define DISP_BRIGHTEST 0x00

boolean colon_state=FALSE;
boolean display_mode=0;

void update_display0(void)
{
	output_low(DISP0_SS);
	#asm nop #endasm

	// Hours (1-2 digit)
	spi_write(time.hour/10);
	spi_write(time.hour%10);

	// Minutes
	spi_write(time.minute/10);
	spi_write(time.minute%10);

	// Deselect display
	#asm nop #endasm
	output_high(DISP0_SS);
}

void update_display1(void)
{
	output_low(DISP1_SS);
	#asm nop #endasm

	// Seconds (1-2 digit)
	spi_write(time.second/10);
	spi_write(time.second%10);

	// Hundreths of a second
	spi_write(time.second_100/10);
	spi_write(time.second_100%10);

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
	#asm nop #endasm
	// reset
	spi_write(0x76);
	// dots
	spi_write(0x77);
	// colon
	spi_write(0x10);
	// max brightness
	spi_write(0x7A);
	spi_write(DISP_BRIGHTEST);
	#asm nop #endasm
	output_high(DISP0_SS);

	// eset display1, turn on colon
	output_low(DISP1_SS);
	#asm nop #endasm
	// reset
	spi_write(0x76);
	// dots
	spi_write(0x77);
	// colon
	spi_write(0x10);
	// max brightness
	spi_write(0x7A);
	spi_write(DISP_BRIGHTEST);
	#asm nop #endasm
	output_high(DISP1_SS);

	// Fill with initial time (force)
	update_display0();
	update_display1();
}

void toggle_colon(void)
{
	colon_state = ! colon_state;

	// Display0
	output_low(DISP0_SS);
	#asm nop #endasm
	// Dots
	spi_write(0x77);
	// Colon or no colon
	spi_write((uint8_t)colon_state<<4);
	// Deselect display
	#asm nop #endasm
	output_high(DISP0_SS);

	// Display1
	output_low(DISP1_SS);
	#asm nop #endasm
	// Dots
	spi_write(0x77);
	// Colon or no colon
	spi_write((uint8_t)colon_state<<4);
	// Deselect display
	#asm nop #endasm
	output_high(DISP1_SS);
}

