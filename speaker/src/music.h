/*
 * music.h
 *
 *  Created on: Sep 15, 2022
 *      Author: insti
 */

#ifndef MUSIC_H_
#define MUSIC_H_

#include <stdint.h>

#define TRANSPOSE_CONSTANT 0

typedef enum _PITCH{
    A = 1, Ash, B, C, Csh, D, Dsh, E, F, Fsh, G, Gsh
} PITCH;

typedef enum _REST{
    WHOLE = 16, HALF = 8, QUARTER = 4, D_EIGHTH = 3, EIGHTH = 2, SIXTEENTH = 1
} REST;
#define Wh WHOLE
#define Ha HALF
#define Qu QUARTER
#define Ei EIGHTH
#define Si SIXTEENTH

typedef struct _NOTE{
    PITCH pitch;
    uint8_t octave;
    char key;
} NOTE;

#define A(x) A,x
#define Ash(x) Ash,x
#define B(x) B,x
#define C(x) C,x
#define Csh(x) Csh,x
#define D(x) D,x
#define Dsh(x) Dsh,x
#define E(x) E,x
#define F(x) F,x
#define Fsh(x) Fsh,x
#define G(x) G,x
#define Gsh(x) Gsh,x

#define PLAY(x,y,z) playNote(x,y,z)
#define REST(x) playRest(x)

void setBPM(uint32_t bpm);
void playRest(REST rest);
void playNote(PITCH pitch, uint8_t octave, REST rest);

void pianoMain(void);

// SONGS
void playHappyBirthday(void);
void playDoom(void);
void playMegalovania(void);
void playDhoom(void);

#endif /* MUSIC_H_ */
