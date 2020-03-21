#ifndef DISPLAY_H_   /* Include guard */
#define DISPLAY_H_

#include <stdint.h>
#include "types.h"

void delay(int cyc);
uint8_t spi_send_recv(uint8_t data);
void spi_init();
void display_wakeup();
void upplystPixel(int x, int y);
void ritaPaddel(Paddle p);
void ritaBoll(Ball b);
void rensaSkarm();
void ritaPoang(Paddle p1, Paddle p2);
void rita(Paddle p1, Paddle p2, Ball ball);

#endif // DISPLAY_H_
