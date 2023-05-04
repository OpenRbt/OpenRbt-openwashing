#ifndef DIA_SCREEN_H
#define DIA_SCREEN_H
#include <stdio.h>

#define BPP 4
#define DEPTH 32

#define NO_ERROR 0
#define NULL_PASSED 1
#define IMG_ERROR 2
#define SDL_INIT_ERROR 3
#include "dia_screen.h"

DiaScreen::DiaScreen(int resX, int resY, int hideCursor, int fullScreen) {
    
    if (hideCursor) {
        SDL_Cursor* cursor;
        int32_t cursorData[2] = {0,0};
        cursor = SDL_CreateCursor((uint8_t *)cursorData, (uint8_t *)cursorData, 8, 8, 4, 4);
        SDL_SetCursor(cursor);
    }
	delay(100);
    int additionalOption = 0;
    if (fullScreen) {
        additionalOption = SDL_FULLSCREEN;
    }    
	if (!(Canvas = SDL_SetVideoMode(resX, resY, DEPTH, additionalOption|SDL_NOFRAME|SDL_HWSURFACE))) {
        printf("Cant set your videomode, attempt #2, without fullscreen\n");
    	if (!(Canvas = SDL_SetVideoMode(resX, resY, DEPTH, SDL_NOFRAME|SDL_HWSURFACE))) {
            SDL_Quit();
            InitializedOk = SDL_INIT_ERROR;
            return;
        }
    }
    printf("video mode is set properly \n"); fflush(stdout);
	delay(100);
	InitializedOk = 1;
}

void DiaScreen_DrawPage1(DiaScreen * screen, int num) {
    printf("src1_s\n");
    printf("src2_b2\n");
    SDL_Flip(screen->Canvas);
    printf("src2_b3\n");
}

void DiaScreen::FlipFrame() {
    printf("frame ... ");
    SDL_Flip(Canvas);
    printf(" ... flipped \n");
}
void DiaScreen::FillBackground(Uint8 r, Uint8 g, Uint8 b) {
	SDL_FillRect(Canvas, NULL, SDL_MapRGB(Canvas->format, r, g, b));
}

DiaScreen::~DiaScreen() {
    SDL_Quit();
}

#endif
