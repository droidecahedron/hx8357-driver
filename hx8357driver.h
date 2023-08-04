

#ifndef ONETOONEI2CDRIVER_H_
#define ONETOONEI2CDRIVER_H_


#include <F2837xD_device.h>


#define HX8357D_WIDTH  320
#define HX8357D_HEIGHT 480

void initbigLCD(void);

void LCDdrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height,uint16_t color);

void HX8357D_writecommand8(uint16_t value);
void HX8357D_writedata8(uint16_t value);

#endif /* ONETOONEI2CDRIVER_H_ */
