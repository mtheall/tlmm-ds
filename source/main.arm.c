#include <nds.h>
#include <stdio.h>
#include "tlmm.h"

static const char *progs[] = {
  "x",
  "2*x",
  "3*x",
  "x^2",
  "2^x",
  "1/x",
  "sin x",
  "cos x",
  "(sin x)^2",
  "sin x * cos x",
  "tan x",
  "tan (1/x)",
};
#define NUM_PROGS (sizeof(progs)/sizeof(*progs))

void evaluate(tlmmProgram *prog);

int main(int argc, char *argv[]) {
  tlmmProgram  *prog;
  int          down;
  unsigned int selection = 0;

  prog = tlmmInitProgram();

  consoleDemoInit();
  videoSetMode(MODE_5_2D);
  vramSetPrimaryBanks(VRAM_A_MAIN_BG, VRAM_B_MAIN_SPRITE, VRAM_C_SUB_BG, VRAM_D_SUB_SPRITE);
  bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

  iprintf("\x1b[2J================================\n");
  iprintf("Program: %s\n", progs[selection]);
  tlmmParseProgram(prog, progs[selection]);
  evaluate(prog);

  do {
    swiWaitForVBlank();
    scanKeys();
    down = keysDown();

    if(down & KEY_LEFT)
      selection = (selection + NUM_PROGS - 1) % NUM_PROGS;
    if(down & KEY_RIGHT)
      selection = (selection + 1) % NUM_PROGS;

    if(down & (KEY_LEFT|KEY_RIGHT)) {
      iprintf("\x1b[2J================================\n");
      iprintf("Program: %s\n", progs[selection]);
      tlmmParseProgram(prog, progs[selection]);
      evaluate(prog);
    }
  } while(!(down & KEY_B));

  return 0;
}

void evaluate(tlmmProgram *prog) {
  int x;
  dmaFillHalfWords(0, bgGetGfxPtr(3), 256*192*2);

  for(x = 0; x < 256; x++) {
#define SCALE 16
    float v = 192/2 - tlmmGetValue(prog, (float)(x-128)/SCALE)*SCALE;
    if(v >= 0 && v < 192)
      bgGetGfxPtr(3)[(int)v * 256 + x] = ARGB16(1, 31, 31, 31);
  }
}
