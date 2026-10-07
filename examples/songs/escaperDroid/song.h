#ifndef SONG_H
#define SONG_H

// ---------------------------------------------------------------------------
//  "Escaper Droid" — uptempo chiptune for Arduboy / ATMlib
//  D major, 8 bars, same structure as Italo Night
//
//  Play:  #include "newsong2.h"    ATM.play(escaperDroid);
//
//  Timing : ATM_SET_TEMPO(17) → same as Italo Night, ~127.5 BPM.
//            1 beat = 16 ticks, 1 bar = 64 ticks, 8 bars = 512 ticks.
//            Loop: 511 delay ticks + 1 restart tick = seamless.
//  Chords : D | G | A | D | D | Bm | A | D
//  Channels:
//    CH0 PULSE  — lead melody (real notes per bar, uptempo space-chase feel)
//    CH1 SQUARE — pitch-drop kick every beat + arpeggio chord stabs
//    CH2 SAW    — ducked bass: silence-on-kick, root–octave–root, transpose per bar
//    CH3 NOISE  — kick click, clap on 2&4, hats, snare-roll fills in bars 4 & 8
//  Pitch: ATMlib note labels sound ~1 octave lower on hardware (same as Italo Night).
// ---------------------------------------------------------------------------

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif


Song escaperDroid[] = {     // total song bytes = 544
  0x20,                       // Number of tracks
  0x00, 0x00,                 // Track 0  @    0  (silent)
  0x03, 0x00,                 // Track 1  @    3  (lead)
  0x18, 0x00,                 // Track 2  @   24  (bar1)
  0x25, 0x00,                 // Track 3  @   37  (bar2)
  0x32, 0x00,                 // Track 4  @   50  (bar3)
  0x3F, 0x00,                 // Track 5  @   63  (bar4)
  0x4C, 0x00,                 // Track 6  @   76  (bar5)
  0x5D, 0x00,                 // Track 7  @   93  (bar6)
  0x6E, 0x00,                 // Track 8  @  110  (bar7)
  0x7F, 0x00,                 // Track 9  @  127  (bar8)
  0x90, 0x00,                 // Track 10 @  144  (chords)
  0xAB, 0x00,                 // Track 11 @  171  (kick)
  0xB6, 0x00,                 // Track 12 @  182  (beat_D)
  0xC2, 0x00,                 // Track 13 @  194  (beat_G)
  0xCE, 0x00,                 // Track 14 @  206  (beat_A)
  0xDA, 0x00,                 // Track 15 @  218  (beat_Bm)
  0xE6, 0x00,                 // Track 16 @  230  (beat_D_last)
  0xF2, 0x00,                 // Track 17 @  242  (bass)
  0x15, 0x01,                 // Track 18 @  277  (bass_beat)
  0x21, 0x01,                 // Track 19 @  289  (bass_beat4)
  0x2D, 0x01,                 // Track 20 @  301  (bass_bar)
  0x33, 0x01,                 // Track 21 @  307  (bass_beat4_last)
  0x3F, 0x01,                 // Track 22 @  319  (bass_bar_last)
  0x45, 0x01,                 // Track 23 @  325  (drums)
  0x57, 0x01,                 // Track 24 @  343  (drum_A)
  0x67, 0x01,                 // Track 25 @  359  (drum_B)
  0x72, 0x01,                 // Track 26 @  370  (drum_pair)
  0x77, 0x01,                 // Track 27 @  375  (drum_bar)
  0x7B, 0x01,                 // Track 28 @  379  (drum_fill)
  0xA4, 0x01,                 // Track 29 @  420  (drum_fill_last)
  0xCD, 0x01,                 // Track 30 @  461  (drum_bar_fill)
  0xD4, 0x01,                 // Track 31 @  468  (drum_bar_fill_last)

  0x01,                         // CH0 entry → track 1 (lead)
  0x0A,                         // CH1 entry → track 10 (chords)
  0x11,                         // CH2 entry → track 17 (bass)
  0x17,                         // CH3 entry → track 23 (drums)

  // Track  0: silent  [3b]  dummy track 0 (never called)
  ATM_VOL(0),
  ATM_STOP_CHAN,

  // Track  1: lead  [21b]  CH0 PULSE: vol + 8 bars
  ATM_VOL(38),
  ATM_SL_VOL((uint8_t)-2),
  ATM_GOTO(2),
  ATM_GOTO(3),
  ATM_GOTO(4),
  ATM_GOTO(5),
  ATM_GOTO(6),
  ATM_GOTO(7),
  ATM_GOTO(8),
  ATM_GOTO(9),
  ATM_STOP_CHAN,

  // Track  2: bar1  [13b]  lead bar 1 (D)
  ATM_NOTE_D5,
  ATM_DELAY(12),
  ATM_NOTE_F5_,
  ATM_DELAY(12),
  ATM_NOTE_A5,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(12),
  ATM_NOTE_F5_,
  ATM_DELAY(12),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track  3: bar2  [13b]  lead bar 2 (G)
  ATM_NOTE_G5,
  ATM_DELAY(12),
  ATM_NOTE_B5,
  ATM_DELAY(12),
  ATM_NOTE_G5,
  ATM_DELAY(8),
  ATM_NOTE_B5,
  ATM_DELAY(12),
  ATM_NOTE_D6,
  ATM_DELAY(12),
  ATM_NOTE_B5,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track  4: bar3  [13b]  lead bar 3 (A)
  ATM_NOTE_A5,
  ATM_DELAY(12),
  ATM_NOTE_C6_,
  ATM_DELAY(12),
  ATM_NOTE_A5,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(12),
  ATM_NOTE_A5,
  ATM_DELAY(12),
  ATM_NOTE_C6_,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track  5: bar4  [13b]  lead bar 4 (D)
  ATM_NOTE_F5_,
  ATM_DELAY(12),
  ATM_NOTE_E5,
  ATM_DELAY(12),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_A4,
  ATM_DELAY(12),
  ATM_NOTE_D5,
  ATM_DELAY(12),
  ATM_NOTE_F5_,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track  6: bar5  [17b]  lead bar 5 (D)
  ATM_NOTE_D5,
  ATM_DELAY(4),
  ATM_NOTE_E5,
  ATM_DELAY(4),
  ATM_NOTE_F5_,
  ATM_DELAY(8),
  ATM_NOTE_A5,
  ATM_DELAY(16),
  ATM_NOTE_G5,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track  7: bar6  [17b]  lead bar 6 (G)
  ATM_NOTE_G4,
  ATM_DELAY(4),
  ATM_NOTE_B4,
  ATM_DELAY(4),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_G5,
  ATM_DELAY(16),
  ATM_NOTE_F5_,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_B4,
  ATM_DELAY(8),
  ATM_NOTE_G4,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track  8: bar7  [17b]  lead bar 7 (Bm)
  ATM_NOTE_B4,
  ATM_DELAY(4),
  ATM_NOTE_D5,
  ATM_DELAY(4),
  ATM_NOTE_F5_,
  ATM_DELAY(8),
  ATM_NOTE_B5,
  ATM_DELAY(16),
  ATM_NOTE_A5,
  ATM_DELAY(8),
  ATM_NOTE_F5_,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_B4,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track  9: bar8  [17b]  lead bar 8 (A, last – 63t loop comp)
  ATM_NOTE_A4,
  ATM_DELAY(4),
  ATM_NOTE_C5_,
  ATM_DELAY(4),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_A5,
  ATM_DELAY(16),
  ATM_NOTE_G5,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_C5_,
  ATM_DELAY(8),
  ATM_NOTE_A4,
  ATM_DELAY(7),
  ATM_RETURN,

  // Track 10: chords  [27b]  CH1 SQUARE: kick+stab, 8 bars
  ATM_REPEAT(3,12),
  ATM_REPEAT(3,13),
  ATM_REPEAT(3,14),
  ATM_REPEAT(3,12),
  ATM_REPEAT(3,12),
  ATM_REPEAT(3,15),
  ATM_REPEAT(3,14),
  ATM_REPEAT(2,12),
  ATM_GOTO(16),
  ATM_STOP_CHAN,

  // Track 11: kick  [11b]  pitch-drop kick (4t) + silence (4t)
  ATM_ARP_OFF,
  ATM_SL_FRQ((uint8_t)-127),
  ATM_VOL(30),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_RETURN,

  // Track 12: beat_D  [12b]  beat: kick + D major stab (root D3)
  ATM_GOTO(11),
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x47,0x20),
  ATM_NOTE_D3,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track 13: beat_G  [12b]  beat: kick + G major stab (root G3)
  ATM_GOTO(11),
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x47,0x20),
  ATM_NOTE_G3,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track 14: beat_A  [12b]  beat: kick + A major stab (root A3)
  ATM_GOTO(11),
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x47,0x20),
  ATM_NOTE_A3,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track 15: beat_Bm  [12b]  beat: kick + Bm minor stab (root B3)
  ATM_GOTO(11),
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x37,0x20),
  ATM_NOTE_B3,
  ATM_DELAY(8),
  ATM_RETURN,

  // Track 16: beat_D_last  [12b]  last beat: D stab 7t (loop comp)
  ATM_GOTO(11),
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x47,0x20),
  ATM_NOTE_D3,
  ATM_DELAY(7),
  ATM_RETURN,

  // Track 17: bass  [35b]  CH2 SAW: transposed bass, 8 bars
  ATM_SL_VOL((uint8_t)-5),
  ATM_SET_TRA(0),
  ATM_GOTO(20),
  ATM_SET_TRA(5),
  ATM_GOTO(20),
  ATM_SET_TRA(7),
  ATM_GOTO(20),
  ATM_SET_TRA(0),
  ATM_GOTO(20),
  ATM_SET_TRA(0),
  ATM_GOTO(20),
  ATM_SET_TRA(9),
  ATM_GOTO(20),
  ATM_SET_TRA(7),
  ATM_GOTO(20),
  ATM_SET_TRA(0),
  ATM_GOTO(22),
  ATM_STOP_CHAN,

  // Track 18: bass_beat  [12b]  bass beat: silence-root-oct-root (16t)
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(63),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_NOTE_D4,
  ATM_DELAY(4),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_RETURN,

  // Track 19: bass_beat4  [12b]  bass beat 4: silence-root-root-oct (16t)
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(63),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_NOTE_D4,
  ATM_DELAY(4),
  ATM_RETURN,

  // Track 20: bass_bar  [6b]  bass bar (64t): 3× beat + beat4
  ATM_REPEAT(2,18),
  ATM_GOTO(19),
  ATM_RETURN,

  // Track 21: bass_beat4_last  [12b]  last beat4: final note 3t (loop comp)
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(63),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_NOTE_D4,
  ATM_DELAY(3),
  ATM_RETURN,

  // Track 22: bass_bar_last  [6b]  last bass bar
  ATM_REPEAT(2,18),
  ATM_GOTO(21),
  ATM_RETURN,

  // Track 23: drums  [18b]  CH3 NOISE: tempo + 8 bars
  ATM_SET_TEMPO(17),
  ATM_GOTO_ADV(1,10,17,23),
  ATM_REPEAT(2,27),
  ATM_GOTO(30),
  ATM_REPEAT(2,27),
  ATM_GOTO(31),
  ATM_STOP_CHAN,

  // Track 24: drum_A  [16b]  beat 1/3: click, hat, open hat (16t)
  ATM_VOL(28),
  ATM_SL_VOL((uint8_t)-14),
  ATM_DELAY(4),
  ATM_VOL(12),
  ATM_SL_VOL((uint8_t)-6),
  ATM_DELAY(4),
  ATM_VOL(26),
  ATM_SL_VOL((uint8_t)-3),
  ATM_DELAY(8),
  ATM_RETURN,

  // Track 25: drum_B  [11b]  beat 2/4: clap, open hat (16t)
  ATM_VOL(40),
  ATM_SL_VOL((uint8_t)-5),
  ATM_DELAY(8),
  ATM_VOL(26),
  ATM_SL_VOL((uint8_t)-3),
  ATM_DELAY(8),
  ATM_RETURN,

  // Track 26: drum_pair  [5b]  beats 1+2 (32t)
  ATM_GOTO(24),
  ATM_GOTO(25),
  ATM_RETURN,

  // Track 27: drum_bar  [4b]  one bar (64t)
  ATM_REPEAT(1,26),
  ATM_RETURN,

  // Track 28: drum_fill  [41b]  snare roll 16t
  ATM_VOL(22),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(26),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(30),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(34),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(38),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(42),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(46),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(50),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_RETURN,

  // Track 29: drum_fill_last  [41b]  snare roll 15t (loop comp)
  ATM_VOL(22),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(26),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(30),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(34),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(38),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(42),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(46),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(50),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(1),
  ATM_RETURN,

  // Track 30: drum_bar_fill  [7b]  bar 4: pair + drum_A + fill
  ATM_GOTO(26),
  ATM_GOTO(24),
  ATM_GOTO(28),
  ATM_RETURN,

  // Track 31: drum_bar_fill_last  [7b]  bar 8: pair + drum_A + fill_last
  ATM_GOTO(26),
  ATM_GOTO(24),
  ATM_GOTO(29),
  ATM_RETURN,

};

#endif
