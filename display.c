#include "display.h"

#include <stdint.h>
#include <pic32mx.h>
#include "assets.h"
#include "types.h"

#define DISPLAY_VDD PORTFbits.RF6
#define DISPLAY_VBATT PORTFbits.RF5
#define DISPLAY_COMMAND_DATA PORTFbits.RF4
#define DISPLAY_RESET PORTGbits.RG9

#define DISPLAY_VDD_PORT PORTF
#define DISPLAY_VDD_MASK 0x40
#define DISPLAY_VBATT_PORT PORTF
#define DISPLAY_VBATT_MASK 0x20
#define DISPLAY_COMMAND_DATA_PORT PORTF
#define DISPLAY_COMMAND_DATA_MASK 0x10
#define DISPLAY_RESET_PORT PORTG
#define DISPLAY_RESET_MASK 0x200

#define PADDLE_HEIGHT   8     // PADDLE_HEIGHT = 8
uint8_t game[128*4] = {0};

/*
 * Simple delay
 * (tagen från LAB3, mapp "time4io", fil "mipslabfunc.c", motsvarar funktionen "quicksleep")
 */
void delay(int cyc) {
    int i;
    for(i = cyc; i > 0; i--);
}

/*
 * SPI send/receive
 * (tagen från LAB3, mapp "time4io", fil "mipslabfunc.c", motsvarar funktionen "spi_send_recv")
 */
uint8_t spi_send_recv(uint8_t data) {
    while(!(SPI2STAT & 0x08));
    SPI2BUF = data;
    while(!(SPI2STAT & 0x01));
    return SPI2BUF;
}

/*
 * Enable SPI
 * (tagen från LAB3, mapp "time4io", fil "mipslabmain.c", motsvarar funktionen "main")
 */
void spi_init() {
    /* Set up peripheral bus clock */
    OSCCON &= ~0x180000;
    OSCCON |= 0x080000;

    /* Set up output pins */
    AD1PCFG = 0xFFFF;
    ODCE = 0x0;
    TRISECLR = 0xFF;
    PORTE = 0x0;

    /* Output pins for display signals */
    PORTF = 0xFFFF;
    PORTG = (1 << 9);
    ODCF = 0x0;
    ODCG = 0x0;
    TRISFCLR = 0x70;
    TRISGCLR = 0x200;

    /* Set up input pins */
    TRISDSET = (1 << 8);
    TRISFSET = (1 << 1);

    /* Set up SPI as master */
    SPI2CON = 0;
    SPI2BRG = 4;

    /* Clear SPIROV*/
    SPI2STATCLR &= ~0x40;
    /* Set CKP = 1, MSTEN = 1; */
        SPI2CON |= 0x60;

    /* Turn on SPI */
    SPI2CONSET = 0x8000;
}

/*
 * Display startup procedure
 * (tagen från LAB3, mapp "time4io", fil "mipslabfunc.c", motsvarar funktionen "display_init")
 */
void display_wakeup() {
    DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
    delay(10);
    DISPLAY_VDD_PORT &= ~DISPLAY_VDD_MASK;
    delay(1000000);

    spi_send_recv(0xAE);
    DISPLAY_RESET_PORT &= ~DISPLAY_RESET_MASK;
    delay(10);
    DISPLAY_RESET_PORT |= DISPLAY_RESET_MASK;
    delay(10);

    spi_send_recv(0x8D);
    spi_send_recv(0x14);

    spi_send_recv(0xD9);
    spi_send_recv(0xF1);

    DISPLAY_VBATT_PORT &= ~DISPLAY_VBATT_MASK;
    delay(10000000);

    spi_send_recv(0xA1);
    spi_send_recv(0xC8);

    spi_send_recv(0xDA);
    spi_send_recv(0x20);

    spi_send_recv(0xAF);
}

/*
 * Draw single pixel on display by
 * converting coordinates to SPI
 * compatible format.
 */
void upplystPixel(int x, int y) {
    short offset = 0;
    if (y > 0) { offset = y / 8; }
    game[offset * 128 + x] |= 1 << (y - offset * 8);
}

/*
 * Draw paddle
 */
void ritaPaddel(Paddle p) {
    int i;
    for (i = 0; i < PADDLE_HEIGHT; i++) {
        upplystPixel(p.x, p.y + i);
    }
}

/*
 * Draw ball
 */
void ritaBoll(Ball b) {
    upplystPixel(b.x, b.y);
}

/*
 * Reset display
 */
void rensaSkarm() {
    int i;
    for (i = 0; i < sizeof(game); i++) { game[i] = 0; }
}

/*
 * Print current score
 */
void ritaPoang(Paddle p1, Paddle p2) {
    int i, x = 52;
    p1.score = p1.score % 128;
    p2.score = p2.score % 128;
    char c = p1.score + '0';

    for (i = 0; i < 8; i++) {
        game[x + i] = font[c * 8 + i];
    }

    x = 60;
    c = ':';
    for (i = 0; i < 8; i++) {
        game[x + i] = font[c * 8 + i];
    }

    x = 68;
    c = p2.score + '0';
    for (i = 0; i < 8; i++) {
        game[x + i] = font[c * 8 + i];
    }
}

/**
 * Renders the full screen
 * (tagen från LAB3, mapp "time4io", fil "mipslabfunc.c", motsvarar funktionen "display_image")
 */
void rendreraSkarm(uint8_t arr[]) {
    int i, j;

    for(i = 0; i < 4; i++) {
        DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
        spi_send_recv(0x22);
        spi_send_recv(i);

        spi_send_recv(0 & 0xF);
        spi_send_recv(0x10 | ((0 >> 4) & 0xF));

        DISPLAY_COMMAND_DATA_PORT |= DISPLAY_COMMAND_DATA_MASK;

        for(j = 0; j < 128; j++)
            spi_send_recv(arr[i*128 + j]);
    }
}

/*
 * Print the menu
 */
void rendreraMeny(int selected) {
    rensaSkarm();
    int offset = (((128 / 8) - menuLength) / 2) * 8;

    int i, j, f;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < menuLength; j++) {
            for (f = 0; f < 8; f++) {
                if (selected == i && menu[i][j] != ' ') {
                    game[offset + i * 128 + j * 8 + f] = ~font[menu[i][j] * 8 + f];
                }else {
                    game[offset + i * 128 + j * 8 + f] = font[menu[i][j] * 8 + f];
                }
            }
        }
    }

    rendreraSkarm(game);
}

/*
 * Send the next frame to the display
 */
void rita(Paddle p1, Paddle p2, Ball ball) {
    int i, j;

    rensaSkarm();
    ritaPaddel(p1);
    ritaPaddel(p2);
    ritaBoll(ball);
    ritaPoang(p1, p2);

    rendreraSkarm(game);
}

/*
 * Starting screen
 */
void ritaLogo() {
    rendreraSkarm(logo);
}

void ritaThanks(){
    rendreraSkarm(thanks);
}


/*
 * Ending Screen
 */
void ritaAvslut(Paddle p1, Paddle p2) {
    rensaSkarm();
    int i;
    for (i = 0; i < sizeof(game); i++) {
      if (p1.score > p2.score)
        game[i] = youwin[i];
      else if (p1.score < p2.score)
        game[i] = gameover[i];
    }

    rendreraSkarm(game);
}
