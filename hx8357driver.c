#include <F28x_Project.h>

#include <stdbool.h>
#include <stdio.h>


#include "hx8357driver.h"

/******** Adjust these for your board/mcu! *********/
// Controls will be on different port
#define LCD_RST_DIR     GpioCtrlRegs.GPCDIR.bit.GPIO95
#define LCD_RST_SET     GpioDataRegs.GPCSET.bit.GPIO95
#define LCD_RST_CLEAR     GpioDataRegs.GPCCLEAR.bit.GPIO95

#define LCD_RD_DIR      GpioCtrlRegs.GPEDIR.bit.GPIO139
#define LCD_RD_SET      GpioDataRegs.GPESET.bit.GPIO139
#define LCD_RD_CLEAR      GpioDataRegs.GPECLEAR.bit.GPIO139

#define LCD_WR_DIR      GpioCtrlRegs.GPBDIR.bit.GPIO56
#define LCD_WR_SET      GpioDataRegs.GPBSET.bit.GPIO56
#define LCD_WR_CLEAR      GpioDataRegs.GPBCLEAR.bit.GPIO56

#define LCD_CD_DIR      GpioCtrlRegs.GPDDIR.bit.GPIO97
#define LCD_CD_DATA      GpioDataRegs.GPDSET.bit.GPIO97
#define LCD_CD_CMD      GpioDataRegs.GPDCLEAR.bit.GPIO97

#define LCD_CS_DIR      GpioCtrlRegs.GPCDIR.bit.GPIO94
#define LCD_CS_SET      GpioDataRegs.GPCSET.bit.GPIO94
#define LCD_CS_CLEAR      GpioDataRegs.GPCCLEAR.bit.GPIO94

// Data is GPIO[7:0]
#define LCD_DATA_IN     GpioCtrlRegs.GPADIR.all &= ~(0xFF)
#define LCD_DATA_OUT    GpioCtrlRegs.GPADIR.all |= (0xFF)
/***************************************************/


// HX8357-D Commands
#define HX8357D_CMD_SWRESET     0x01
#define HX8357D_CMD_SLPOUT      0x11
#define HX8357D_CMD_DISPON      0x29
#define HX8357D_CMD_COLMOD      0x3A
#define HX8357D_CMD_SETOSC      0xB0
#define HX8357D_CMD_SETEXC      0xB9
#define HX8357D_CMD_SETPANEL    0xCC

// defines for setting address window
#define HX8357D_CMD_CASET       0x2A
#define HX8357D_CMD_PASET       0x2B
#define HX8357D_CMD_RAMWR       0x2C



void initbigLCD(void)
{
    //*****GPIO setting*****//
    EALLOW;
    LCD_RST_SET = 1;
    LCD_RST_DIR = 0;
    LCD_CS_DIR = 1;
    LCD_CS_SET = 1;
    LCD_CD_DIR = 1;
    LCD_CD_DATA = 1;
    LCD_RD_DIR = 1;
    LCD_RD_SET = 1;
    LCD_WR_DIR = 1;
    LCD_WR_SET = 1;
    EDIS;

    DELAY_US(100e3);


    //****init sequence****//
    // writecommand8/writedata8 control pin directions.

    // start with a software reset.
    HX8357D_writecommand8(HX8357D_CMD_SWRESET);
    DELAY_US(6000); // 5ms startup delay + 1ms safety margin.

    // enable extended commands - required for SETOSC and SETPANEL commands.
    HX8357D_writecommand8(HX8357D_CMD_SETEXC);
    HX8357D_writedata8(0xFF);
    HX8357D_writedata8(0x83);
    HX8357D_writedata8(0x57);
    DELAY_US(2500);

    // set internal oscillator to 100 Hz for normal mode
    HX8357D_writecommand8(HX8357D_CMD_SETOSC);
    HX8357D_writedata8(0x6E);

    // set panel to BGR
    HX8357D_writecommand8(HX8357D_CMD_SETPANEL);
    HX8357D_writedata8(0x05);

    // set pixel format to 16 bit
    HX8357D_writecommand8(HX8357D_CMD_COLMOD);
    HX8357D_writedata8(0x55);

    // sleep out
    HX8357D_writecommand8(HX8357D_CMD_SLPOUT);
    DELAY_US(6000); // 5ms delay + 1 ms safety margin

    // display on
    HX8357D_writecommand8(HX8357D_CMD_DISPON);

}


    // sets the lcd address window. this is relevant to rect/screen fills and h/v lines.
    // then fills address window with color.
void LCDdrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    //send starting point first, then ending point. (page 144)

    // column address set
    HX8357D_writecommand8(HX8357D_CMD_CASET);
    HX8357D_writedata8(x>>8);
    HX8357D_writedata8(x);
    HX8357D_writedata8((x+width-1)>>8);
    HX8357D_writedata8((x+width-1));

    // row address set
    HX8357D_writecommand8(HX8357D_CMD_PASET);
    HX8357D_writedata8(y>>8);
    HX8357D_writedata8(y);
    HX8357D_writedata8((y+height-1)>>8);
    HX8357D_writedata8((y+height-1));

    // write to RAM now.
    HX8357D_writecommand8(HX8357D_CMD_RAMWR);
    // to find the dimensions we need to fill, we subtract the addresses
    // to find the actual size of the rectangle.
    uint32_t xsize = width;
    uint32_t ysize = height;
    uint32_t area = xsize*ysize;
    for(uint32_t i=0; i<area; i++)
    {
        HX8357D_writedata8(color >> 8);
        HX8357D_writedata8(color >> 0);
    }

}




//////////////////////////////////////////////////////////
//          INTERFACE (WRITE COMMAND/DATA) FXNs         //


void HX8357D_writecommand8(uint16_t value)
{
    // Set CD low for command
    // drive chip select low
    // set direction of data bus to output
    EALLOW;
    LCD_CD_CMD  = 1;
    LCD_CS_CLEAR = 1;
    LCD_DATA_OUT;

    // put the data onto the data bus.
    GpioDataRegs.GPASET.all = value & 0xFF;    // sets bits
    GpioDataRegs.GPACLEAR.all = 0xFF ^ value; // clear the bits. via XOR

    // clear the write signal, set the write signal, set chip select.
    LCD_WR_CLEAR = 1;
    LCD_WR_SET = 1;
    LCD_CS_SET = 1;

}


void HX8357D_writedata8(uint16_t value)
{

    // Set CD high for command
    // drive chip select low
    // set direction of data bus to output
    EALLOW;
    LCD_CD_DATA  = 1;
    LCD_CS_CLEAR = 1;
    LCD_DATA_OUT;

    // put the data onto the data bus.
    GpioDataRegs.GPASET.all = value & 0xFF;    // sets bits
    GpioDataRegs.GPACLEAR.all = 0xFF ^ value; // clears the bits with XOR

    // clear the write signal, set the write signal, set chip select. (timing diagram, page19)
    LCD_WR_CLEAR = 1;
    LCD_WR_SET = 1;
    LCD_CS_SET = 1;

}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

