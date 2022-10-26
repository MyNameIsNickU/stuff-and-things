/*
 * music.c
 *
 *  Created on: Sep 15, 2022
 *      Author: insti
 */

#include "music.h"
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "math.h"
#include "wait.h"
#include "uart0.h"
#include "utilities.h"

#define BB_BLUE_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))
#define BB_GREEN_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

uint32_t beatCount = 0;
void beats(void)
{
    beatCount += 1;

    TIMER2_ICR_R = TIMER_ICR_TATOCINT;
}

uint8_t vibrato = 10;
#define VIBRATO_AMOUNT 150
void beats2(void)
{
    if(vibrato == 10)
    {
        vibrato = 1;
        PWM1_2_LOAD_R += VIBRATO_AMOUNT;
    }
    else if(vibrato == 1)
    {
        vibrato = 0;
        PWM1_2_LOAD_R -= VIBRATO_AMOUNT*2;
    }
    else if(vibrato == 0)
    {
        vibrato = 1;
        PWM1_2_LOAD_R += VIBRATO_AMOUNT*2;
    }

    PWM1_2_CMPB_R++;

    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}

#define VIBRATO_ON (TIMER1_CTL_R |= TIMER_CTL_TAEN);
#define VIBRATO_OFF (TIMER1_CTL_R &= ~TIMER_CTL_TAEN);


#define PIN_PE0 (*((volatile uint32_t *)(0x42000000 + (0x4005C3FC-0x40000000)*32 + 4*0)))
void playIsr(void)
{
    PIN_PF1_BB ^= 1;
    //_delay_cycles(100);
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}

// assuming using Timer1A
// counts each sixteenth note in the BPM
// coverts BPM to timer load value
void setBPM(uint32_t bpm)
{
    bpm *= 4.0;
    TIMER2_TAILR_R = (uint32_t)(40000000.0 / ((float)bpm/60.0));
    TIMER2_CTL_R |= TIMER_CTL_TAEN;
}

// waits specific rest based on BPM
void playRest(REST rest)
{
    beatCount = 0;
    while(beatCount < rest);
}

/*playFreq(float freq)
{
	uint32_t pwm_load;
	
	pwm_load = (uint32_t)((40000000.0f / 8.0f) / freq);
	
	PWM1_2_CTL_R = 0;
    PWM1_2_LOAD_R = pwm_load;
    PWM1_2_CMPB_R = PWM1_2_LOAD_R / 2;
    PWM1_2_CTL_R = PWM_1_CTL_ENABLE;
}*/

// playNote(Fsh, length);
void playNote(PITCH pitch, uint8_t octave, REST rest)
{
	// assuming we are tuning from 440 Hz A4
    PITCH base_ref = A;
    uint8_t base_octave = 4;
    float note_freq;
    uint32_t pwm_load, timer_load;

	// finds the number of notes difference between A and desired note
    int8_t note_dif = pitch - base_ref;

	// adds the 12 note offset for each octave A4
    note_dif += 12 * (octave - base_octave);

    note_dif += TRANSPOSE_CONSTANT;
	// since C is acutally where the octave changes (not A), take away an octave
    if(pitch >= C)
        note_dif -= 12;


	// calculate note frequency from difference from A4
    note_freq = 440.0 * pow(2, (float)note_dif / 12.0);
	// convert desired frequency to PWM Load value
    pwm_load = (uint32_t)((40000000.0f / 8.0f) / note_freq);
    PWM1_2_CTL_R = 0;
    PWM1_2_LOAD_R = pwm_load;
    PWM1_2_CMPB_R = PWM1_2_LOAD_R * 3 / 4; // builds full square wave of frequency
    //PWM1_2_CMPB_R = PWM1_2_LOAD_R * 3 / 10; // builds full square wave of frequency
    PWM1_2_CTL_R = PWM_1_CTL_ENABLE;
    BB_GREEN_LED = 1;
    BB_BLUE_LED = 1;

    /*timer_load = (uint32_t)((40000000.0f) / note_freq);
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER1_TAILR_R = timer_load;
    TIMER1_CTL_R |= TIMER_CTL_TAEN;*/

	// beats() ISR counts 4 * Desired BPM
	// 4 counts means a quarter rest
    beatCount = 0;
    while(beatCount < rest);
    BB_GREEN_LED = 0;
    BB_BLUE_LED = 0;
    PWM1_2_CTL_R = 0;
    //TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    waitMicrosecond(10000);
}


#define PIANO_SIZE 13
// 'a' == C3 or something
void pianoKey(char key)
{
	NOTE map[PIANO_SIZE] = {
	                {C(3),     'a'},
	                {Csh(3),   'w'},
	                {D(3),     's'},
	                {Dsh(3),   'e'},
	                {E(3),     'd'},
	                {F(3),     'f'},
	                {Fsh(3),   't'},
	                {G(3),     'g'},
	                {Gsh(3),     'y'},
	                {A(3),     'h'},
	                {Ash(3),     'u'},
	                {B(3),     'j'},
	                {C(4),     'k'},
	};

	uint8_t i;
	for(i = 0; map[i].key != key && i < PIANO_SIZE; i++) { }

	if(i < PIANO_SIZE)
	{
	    playNote(map[i].pitch, map[i].octave, EIGHTH);
	}
}

// Run this is main
void pianoMain(void)
{
	char key;
	
	emb_printf("Press keys along home row to play: ");

	while( 1 )
	{
		key = getcUart0();
		
		if(key == 'q')
		    return;

		pianoKey(key);
	}
}

void playHappyBirthday(void)
{
    playNote(C, 4, EIGHTH);
    playRest(SIXTEENTH);
    playNote(C, 4, EIGHTH);
    playNote(D, 4, QUARTER);
    playNote(C, 4, QUARTER);
    playNote(F, 4, QUARTER);
    playNote(E, 4, HALF);

    playNote(C, 4, EIGHTH);
    playRest(SIXTEENTH);
    playNote(C, 4, EIGHTH);
    playNote(D, 4, QUARTER);
    playNote(C, 4, QUARTER);
    playNote(G, 4, QUARTER);
    playNote(F, 4, HALF);

    playNote(C, 4, EIGHTH);
    playRest(SIXTEENTH);
    playNote(C, 4, EIGHTH);
    playNote(C, 5, QUARTER);
    playNote(A, 4, QUARTER);
    playNote(F, 4, QUARTER);
    playNote(E, 4, QUARTER);
    playNote(D, 4, QUARTER);

    playNote(Ash, 4, EIGHTH);
    playRest(SIXTEENTH);
    playNote(Ash, 4, EIGHTH);
    playNote(A, 4, QUARTER);
    playNote(F, 4, QUARTER);
    playNote(G, 4, QUARTER);
    playNote(F, 4, HALF);
}

void playDoom(void)
{
	setBPM(111);
	
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(E, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(D, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(C, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(Ash, 2, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(B, 2, SIXTEENTH); //
    playNote(C, 3, SIXTEENTH); //

    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(E, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(D, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(C, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    VIBRATO_ON
    playNote(Ash, 2, QUARTER); //
    VIBRATO_OFF
    playRest(SIXTEENTH);

    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(E, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(D, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(C, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(Ash, 2, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(B, 2, SIXTEENTH); //
    playNote(C, 3, SIXTEENTH); //

    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(E, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(D, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    playNote(C, 3, SIXTEENTH); //
    playNote(E, 2, SIXTEENTH);
    playNote(E, 2, SIXTEENTH);
    VIBRATO_ON
    playNote(Ash, 2, QUARTER); //
    VIBRATO_OFF



}

void playMegalovania(void)
{
	setBPM(120);
	
    PLAY(D,3,Si);
    PLAY(D,3,Si);
    PLAY(D,4,Ei);
    PLAY(A,3,Ei);
    REST(Si);
    PLAY(Gsh,3,Si);
    REST(Si);
    PLAY(G,3,Si);
    REST(Si);
    PLAY(F,3,Si);
    PLAY(F,3,Si);
    PLAY(D,3,Si);
    PLAY(F,3,Si);
    PLAY(G,3,Si);

    PLAY(C,3,Si);
    PLAY(C,3,Si);
    PLAY(D,4,Ei);
    PLAY(A,3,Ei);
    REST(Si);
    PLAY(Gsh,3,Si);
    REST(Si);
    PLAY(G,3,Si);
    REST(Si);
    PLAY(F,3,Si);
    PLAY(F,3,Si);
    PLAY(D,3,Si);
    PLAY(F,3,Si);
    PLAY(G,3,Si);

    PLAY(B,2,Si);
    PLAY(B,2,Si);
    PLAY(D,4,Ei);
    PLAY(A,3,Ei);
    REST(Si);
    PLAY(Gsh,3,Si);
    REST(Si);
    PLAY(G,3,Si);
    REST(Si);
    PLAY(F,3,Si);
    PLAY(F,3,Si);
    PLAY(D,3,Si);
    PLAY(F,3,Si);
    PLAY(G,3,Si);

    PLAY(Ash,2,Si);
    PLAY(Ash,2,Si);
    PLAY(D,4,Ei);
    PLAY(A,3,Ei);
    REST(Si);
    PLAY(Gsh,3,Si);
    REST(Si);
    PLAY(G,3,Si);
    REST(Si);
    PLAY(F,3,Si);
    PLAY(F,3,Si);
    PLAY(D,3,Si);
    PLAY(F,3,Si);
    PLAY(G,3,Si);

    //

    PLAY(F,4,Ei);
    PLAY(F,4,Si);
    PLAY(F,4,Si);
    REST(Si);
    PLAY(F,4,Si);
    REST(Si);
    PLAY(F,4,Si);
    REST(Si);
    PLAY(D,4,Ei);
    PLAY(D,4,Qu);
    REST(Si);

    PLAY(F,4,Ei);
    PLAY(F,4,Si);
    PLAY(F,4,Si);
    REST(Si);
    PLAY(G,4,Si);
    REST(Si);
    PLAY(Gsh,4,Ei);
    //PLAY(G,3,Si); //
    PLAY(A,4,Si); // Th
    PLAY(G,4,Si);
    PLAY(D,4,Si);
    PLAY(F,4,Si);
    PLAY(G,4,Si);
    REST(Ei);

    PLAY(F,4,Ei);
    PLAY(F,4,Si);
    PLAY(F,4,Si);
    REST(Si);
    PLAY(G,4,Si);
    REST(Si);
    PLAY(Gsh,4,Si);
    REST(Si);
    PLAY(A,4,Si);
    REST(Si);
    PLAY(C,5,Si);
    REST(Si);
    PLAY(A,4,Ei);
    REST(Si);

    PLAY(D,5,Ei);
    PLAY(D,5,Ei);
    PLAY(D,5,Si);
    PLAY(A,4,Si);
    PLAY(D,5,Si);
    PLAY(C,5,Qu);
}


void playDhoom(void)
{
    while(1)
    {
        PLAY(A,2,D_EIGHTH);
        PLAY(E,3,Ei);
        PLAY(D,3,D_EIGHTH);
        PLAY(E,3,Ei);
        PLAY(D,3,Ei);
        PLAY(E,3,Ei);
        PLAY(C,3,D_EIGHTH);
        PLAY(D,3,Ei);
        PLAY(E,3,Ei);
        PLAY(D,3,D_EIGHTH);
        PLAY(C,3,Ei);
        PLAY(B,2,Ei);

        PLAY(A,2,D_EIGHTH);
        PLAY(E,3,Ei);
        PLAY(D,3,D_EIGHTH);
        PLAY(E,3,Ei);
        PLAY(D,3,Ei);
        PLAY(E,3,Ei);
        PLAY(C,3,D_EIGHTH);
        PLAY(D,3,Ei);
        PLAY(E,3,Ei);
        VIBRATO_ON
        PLAY(D,3,QUARTER);
        REST(D_EIGHTH);
        VIBRATO_OFF

        PLAY(A,2,D_EIGHTH);
        PLAY(E,3,Ei);
        PLAY(D,3,D_EIGHTH);
        PLAY(E,3,Ei);
        PLAY(D,3,Ei);
        PLAY(E,3,Ei);
        PLAY(C,3,D_EIGHTH);
        PLAY(D,3,Ei);
        PLAY(E,3,Ei);
        PLAY(D,3,D_EIGHTH);
        PLAY(C,3,Ei);
        PLAY(B,2,Ei);
        PLAY(G,2,D_EIGHTH); //
        PLAY(A,2,Ei);
        PLAY(B,2,Ei);
        PLAY(C,3,D_EIGHTH);
        PLAY(B,2,Ei);
        PLAY(A,2,Ei);
        PLAY(G,2,D_EIGHTH);
        PLAY(B,2,Ei);
        PLAY(A,2,D_EIGHTH);
        PLAY(B,2,Ei);
        PLAY(A,2,D_EIGHTH);
    }
}
