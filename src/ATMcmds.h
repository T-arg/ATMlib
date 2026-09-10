#include <stdint.h>

#ifndef ATMCMDS_H
#define ATMCMDS_H

// let's define command list for the ATMlib, that might be easier to use the AMT library

// Let's define the basic commands
#define ATM_STOP      	 				    0x00+0
#define ATM_DELAY(delay) 				    0x9F+(delay)
#define ATM_LONG_DELAY(delay)			    0x9F,(delay)
#define ATM_GOTO(onceGoto)				    0xFC,(onceGoto)
#define ATM_REPEAT(repeatTimes,track)	    0xFD,(repeatTimes),(track)
#define ATM_RETURN						    0xFE

#define ATM_ADD_TEMPO(amount)				0x9C,(amount)
#define ATM_SET_TEMPO(amount)				0x9D,(amount)
#define ATM_GOTO_ADV(ch0,ch1,ch2,ch3)	0x9E,(ch0),(ch1),(ch2),(ch3)
#define ATM_STOP_CHAN						0x9F


// Let's define the effect commands
// VOLUME - VOLUME SLIDE
#define ATM_VOL(setVolume)					0x40,(setVolume)
#define ATM_SL_VOL(slideVolume)				0x41,(slideVolume)
#define ATM_SL_VOL_ADV(amount,ticks)		0x42,(amount),(ticks)
#define ATM_SL_VOL_OFF						0x43

// FREQUENCE SLIDE
#define ATM_SL_FRQ(slideFrequency)			0x44,(slideFrequency)
#define ATM_SL_FRQ_ADV(amount,ticks)		0x45,(amount),(ticks)
#define ATM_SL_FRQ_OFF						0x46

// ARPEGGIO
#define ATM_ARP(thirdNote,ticks)			0x47,(thirdNote),(ticks)
#define ATM_ARP_OFF							0x48

// RETRIGGERING NOISE ON THE NOISE CHANNEL 3
#define ATM_NOISE(entryPointAndSpeed)		0x49,(entryPointAndSpeed)
#define ATM_NOISE_OFF						0x4A

// TRANSPOSITION
#define ATM_ADD_TRA(amount)					0x4B,(amount)
#define ATM_SET_TRA(amount)					0x4C,(amount)
#define ATM_TRA_OFF							0x4D

// TREMOLO
#define ATM_TREM(depth,rate)				0x4E,(depth),(rate)
#define ATM_TREM_OFF						0x4F

// VIBRATO
#define ATM_VIB(depth,rate)				0x50,(depth),(rate)
#define ATM_VIB_OFF							0x51

// GLISSANDO
#define ATM_GLIS(noteTicks)					0x52,(noteTicks)
#define ATM_GLIS_OFF						0x53

// NOTE CUT
#define ATM_CUT(amount)						0x54,(amount)
#define ATM_CUT_OFF							0x55

// WAVEFORM (0 = PULSE, 1 = SQUARE, 2 = NOISE)
#define ATM_WAVEFORM(type)					0x56,(type)

// Signal the sketch: ATM.check() returns this byte until the next cue or check()
#define ATM_CUE(value)						0x57,(value)


// let's Define all 64 NOTES from C2 up to D7, actually 63 because note 0 means mute or no note
#define ATM_NOTE_C2       		 0x00 + 1
#define ATM_NOTE_C2_      		 0x00 + 2
#define ATM_NOTE_D2       		 0x00 + 3
#define ATM_NOTE_D2_      		 0x00 + 4
#define ATM_NOTE_E2       		 0x00 + 5
#define ATM_NOTE_F2       		 0x00 + 6
#define ATM_NOTE_F2_      		 0x00 + 7
#define ATM_NOTE_G2       		 0x00 + 8
#define ATM_NOTE_G2_      		 0x00 + 9
#define ATM_NOTE_A2       		 0x00 + 10
#define ATM_NOTE_A2_      		 0x00 + 11
#define ATM_NOTE_B2       		 0x00 + 12
		 
#define ATM_NOTE_C3       		 0x00 + 13
#define ATM_NOTE_C3_      		 0x00 + 14
#define ATM_NOTE_D3       		 0x00 + 15
#define ATM_NOTE_D3_      		 0x00 + 16
#define ATM_NOTE_E3       		 0x00 + 17
#define ATM_NOTE_F3       		 0x00 + 18
#define ATM_NOTE_F3_      		 0x00 + 19
#define ATM_NOTE_G3       		 0x00 + 20
#define ATM_NOTE_G3_      		 0x00 + 21
#define ATM_NOTE_A3       		 0x00 + 22
#define ATM_NOTE_A3_      		 0x00 + 23
#define ATM_NOTE_B3       		 0x00 + 24
		 
#define ATM_NOTE_C4       		 0x00 + 25
#define ATM_NOTE_C4_      		 0x00 + 26
#define ATM_NOTE_D4       		 0x00 + 27
#define ATM_NOTE_D4_      		 0x00 + 28
#define ATM_NOTE_E4       		 0x00 + 29
#define ATM_NOTE_F4       		 0x00 + 30
#define ATM_NOTE_F4_      		 0x00 + 31
#define ATM_NOTE_G4       		 0x00 + 32
#define ATM_NOTE_G4_      		 0x00 + 33
#define ATM_NOTE_A4       		 0x00 + 34
#define ATM_NOTE_A4_      		 0x00 + 35
#define ATM_NOTE_B4       		 0x00 + 36
		 
#define ATM_NOTE_C5       		 0x00 + 37
#define ATM_NOTE_C5_      		 0x00 + 38
#define ATM_NOTE_D5       		 0x00 + 39
#define ATM_NOTE_D5_      		 0x00 + 40
#define ATM_NOTE_E5       		 0x00 + 41
#define ATM_NOTE_F5       		 0x00 + 42
#define ATM_NOTE_F5_      		 0x00 + 43
#define ATM_NOTE_G5       		 0x00 + 44
#define ATM_NOTE_G5_      		 0x00 + 45
#define ATM_NOTE_A5       		 0x00 + 46
#define ATM_NOTE_A5_      		 0x00 + 47
#define ATM_NOTE_B5       		 0x00 + 48
		 
#define ATM_NOTE_C6       		 0x00 + 49
#define ATM_NOTE_C6_      		 0x00 + 50
#define ATM_NOTE_D6       		 0x00 + 51
#define ATM_NOTE_D6_      		 0x00 + 52
#define ATM_NOTE_E6       		 0x00 + 53
#define ATM_NOTE_F6       		 0x00 + 54
#define ATM_NOTE_F6_      		 0x00 + 55
#define ATM_NOTE_G6       		 0x00 + 56
#define ATM_NOTE_G6_      		 0x00 + 57
#define ATM_NOTE_A6       		 0x00 + 58
#define ATM_NOTE_A6_      		 0x00 + 59
#define ATM_NOTE_B6       		 0x00 + 60
		 
#define ATM_NOTE_C7       		 0x00 + 61
#define ATM_NOTE_C7_      		 0x00 + 62
#define ATM_NOTE_D7       		 0x00 + 63



// One-track SFX for ATMsynth::playSfx(track, ch)
// Example: ATM_SFX_TRACK(sfxJump, ATM_VOL(48), ATM_NOTE_C5, ATM_DELAY(8));
#ifndef ATM_SFX_TRACK
#define ATM_SFX_TRACK(name, ...) \
  const uint8_t name[] PROGMEM = { __VA_ARGS__, ATM_STOP_CHAN }
#endif

#endif
