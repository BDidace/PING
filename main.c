#include <pic32mx.h>
#include "types.h"
#include "helpers.h"
#include "assets.h"
#include "display.h"
#include "freqmap.h"

#define GAME_SPEED          20
#define GAME_WIN_SCORE      3
#define MAX_X               128
#define MAX_Y               32
#define PADDLE_HEIGHT       8
#define CONTROLLER_SPEED    40

#define STATE_START     0
#define STATE_PONG      1
#define STATE_END       2
#define STATE_MENU      3
#define STATE_CREDIT    4

#define MENU_MULTI      1
#define MENU_CPUBAS     0

int gameState = STATE_START;
int menuState = MENU_MULTI;
Paddle p1, p2;
Ball ball;

/*
 * One frame of the game
 */
void avancera() {
    ball.x = (ball.x + ball.speedX);
    ball.y = (ball.y + ball.speedY);

    // vertical collision detection
    if (ball.y <= 0) {
        ball.y = 0;
        ball.speedY *= (-1);
    }else if (ball.y >= MAX_Y - 1) {
        ball.y = MAX_Y - 1;
        ball.speedY *= (-1);
    }

    // horizontal collision detection
    if (ball.x <= 0) {
        // score for p2?
        if (ball.y < p1.y || ball.y > p1.y + PADDLE_HEIGHT - 1) { p2.score += 1; }

        ball.x = 0;
        ball.speedX *= (-1);
    }else if (ball.x >= MAX_X - 1) {
        // score for p1?
        if (ball.y < p2.y || ball.y > p2.y + PADDLE_HEIGHT - 1) { p1.score += 1; }

        ball.x = MAX_X - 1;
        ball.speedX *= (-1);
    }
}

/*
 * Set up game configuration
 * (Placeringen av paddles och bollen när spelet sätts igång)
 */
void init_game() {
    p1.x = 0;
    p1.y = 12;
    p1.score = 0;

    p2.x = 127;
    p2.y = 23;
    p2.score = 0;

    ball.x = 61;
    ball.y = 15;
    ball.speedX = 2;
    ball.speedY = 1;
}

/*
 * Initialise
 */
int main(void) {
    spi_init();
    display_wakeup();

    ritaLogo();
    init_game();

    // setup hardware
    enableButtons();
    enableTimer2(31250, 0x1B, 0x111, 1);

    enableMultiVectorMode();
    enable_interrupt();

    for(;;) ;
    return 0;
}

int counter = GAME_SPEED;

/**
 * Linear mapping from [0,1023] to valid paddle position
 */
int translateToScreen(int val) {
    return val > 0 ? ((MAX_Y - PADDLE_HEIGHT) * val) / 1024 : 0;
}

void updateraPaddel() {

    if (isButtonPressed(3) && (p1.y < 24)){
      Paddle p;
      p1.y+=5;
    }
    else if (isButtonPressed(2) && (p1.y > 0)){
      Paddle p;
      p1.y-=5;
    }
}

/**
 * Updaterar menyn
 */
void updateraMeny() {
    int PressStart;

    menuState = ((1 * PressStart) / 1024);
}

int direction = 0;
int targetCoord = 0;
/**
 *  Genererar en CPU motståndare
 */
void updateraCPU() {
    int max;
    max = MAX_Y - PADDLE_HEIGHT;
    switch(menuState) {
        case MENU_CPUBAS:
            if (ball.y == 0) {
                direction = 1;
            } else if (ball.y == MAX_Y - 1) {
                direction = -1;
            }

            if (direction == 1 && p2.y < 23 && ball.x > 100) {      // OM bollen rör sig nedåt OCH paddeln är på den övre delen av skärmen OCH bollen är över andra planhalvan
                p2.y++;                                             // DÅ rör sig paddeln nedåt.
            } else if (direction == -1 && p2.y > 0 && ball.x > 100) { // OM bollen rör sig uppåt OCH paddeln är någonstans på skärmen OCH bollen är över andra planhalva
                p2.y--;                                                // DÅ rör sig paddeln uppåt.
            }
            break;

    }
}

/**
 * ISR Interrupt handler for timer 2
 */
void timer2_interrupt_handler(void) {
    IFSCLR(0) = 0x100;
    counter--;

    if (counter != 0) { return; }
    counter = GAME_SPEED;
    updateraPaddel();

    switch (gameState) {
        case STATE_MENU:
            updateraMeny();
            rendreraMeny(menuState);
            if (isButtonPressed(4)) {
                init_game();
                gameState = STATE_PONG;
                rita(p1, p2, ball);
            }
            break;
        case STATE_PONG:
            avancera();

            // cpu player movement
            if (menuState == MENU_CPUBAS) {
                updateraCPU();
            }

            // check for game abort
            if (isButtonPressed(4)) {
                gameState = STATE_MENU;
                rendreraMeny(menuState);
            }

            rita(p1, p2, ball);

            // game end?
            if (p1.score >= GAME_WIN_SCORE || p2.score >= GAME_WIN_SCORE) {
                gameState = STATE_END;
                ritaAvslut(p1, p2);
            }
            break;
        case STATE_START:
            if (isButtonPressed(4)) {
                gameState = STATE_MENU;
                rendreraMeny(menuState);
            }
            break;
        case STATE_CREDIT:
            if (isButtonPressed(4)) {
                gameState = STATE_START;
                ritaLogo();
            }
            break;
        case STATE_END:
            if (isButtonPressed(4)) {
                gameState = STATE_CREDIT;
                ritaThanks();
            }
            break;

    }
}

/**
 * ISR Interrupt handler for timer 3
 */
void timer3_interrupt_handler(void) {}

/**
 * ISR general interrupt handler
 */
void core_interrupt_handler(void) {}
