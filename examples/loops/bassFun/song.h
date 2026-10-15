#ifndef SONG_H
#define SONG_H

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif

// Play with:   #include "song.h"      ATM.play(bassFun);
// 1 beat = 16 ticks, 1 bar = 64 ticks, 8 bars = 512 ticks. Every channel is one self-looping track (no hop).
// Pitch: ATMlib note names sound about an octave above their label.

// ---------------------------------------------------------------------------
// Bass Fun - Italo disco groove (after the demo "Bass Fun")
// D major / mixolydian, 8 bars (512 ticks, about 18 s), tempo 28 (105 BPM): D A | C G | D A | C G, twice.
// CH0 pad (slow arpeggio chords), CH1 kick, CH2 saw bass with octave jumps, CH3 noise clap + hats + a roll.
// 212 bytes
// ---------------------------------------------------------------------------
Song bassFun[] = {          // total song in bytes = 212
  0x0F,                         // Number of tracks
  0x00, 0x00,                   // Address of track 0   //0  drums
  0x07, 0x00,                   // Address of track 1   //7  drum4
  0x0F, 0x00,                   // Address of track 2   //15  pair
  0x14, 0x00,                   // Address of track 3   //20  hat
  0x1D, 0x00,                   // Address of track 4   //29  clap
  0x28, 0x00,                   // Address of track 5   //40  roll
  0x43, 0x00,                   // Address of track 6   //67  kicks
  0x48, 0x00,                   // Address of track 7   //72  kick
  0x53, 0x00,                   // Address of track 8   //83  pad
  0x58, 0x00,                   // Address of track 9   //88  pad4
  0x70, 0x00,                   // Address of track 10   //112  bass
  0x77, 0x00,                   // Address of track 11   //119  bass4
  0x88, 0x00,                   // Address of track 12   //136  bar
  0x95, 0x00,                   // Address of track 13   //149  beat1
  0xA4, 0x00,                   // Address of track 14   //164  beat

  0x08,                         // Channel 0 entry track (pad)
  0x06,                         // Channel 1 entry track (kicks)
  0x0A,                         // Channel 2 entry track (bass)
  0x00,                         // Channel 3 entry track (drums)

  //"Track 0" drums  [7 bytes]
  ATM_SET_TEMPO(28),
  ATM_REPEAT(1, 1),   // 2x drum4
  ATM_GOTO(0),   // -> drums

  //"Track 1" drum4  [8 bytes]
  ATM_REPEAT(6, 2),   // 7x pair
  ATM_GOTO(3),   // -> hat
  ATM_GOTO(5),   // -> roll
  ATM_RETURN,

  //"Track 2" pair  [5 bytes]
  ATM_GOTO(3),   // -> hat
  ATM_GOTO(4),   // -> clap
  ATM_RETURN,

  //"Track 3" hat  [9 bytes]
  ATM_VOL(0),
  ATM_DELAY(8),
  ATM_VOL(14),
  ATM_SL_VOL((uint8_t)-4),
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 4" clap  [11 bytes]
  ATM_VOL(44),
  ATM_SL_VOL((uint8_t)-6),
  ATM_DELAY(8),
  ATM_VOL(14),
  ATM_SL_VOL((uint8_t)-4),
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 5" roll  [27 bytes]
  ATM_VOL(22),
  ATM_SL_VOL((uint8_t)-9),
  ATM_DELAY(2),
  ATM_VOL(26),
  ATM_DELAY(2),
  ATM_VOL(30),
  ATM_DELAY(2),
  ATM_VOL(34),
  ATM_DELAY(2),
  ATM_VOL(38),
  ATM_DELAY(2),
  ATM_VOL(42),
  ATM_DELAY(2),
  ATM_VOL(46),
  ATM_DELAY(2),
  ATM_VOL(50),
  ATM_DELAY(2),
  ATM_RETURN,

  //"Track 6" kicks  [5 bytes]
  ATM_REPEAT(31, 7),   // 32x kick
  ATM_GOTO(6),   // -> kicks

  //"Track 7" kick  [11 bytes]
  ATM_ARP_OFF,
  ATM_SL_FRQ((uint8_t)-127),
  ATM_VOL(40),
  ATM_NOTE_D3_,
  ATM_DELAY(4),
  ATM_VOL(0),
  ATM_DELAY(12),
  ATM_RETURN,

  //"Track 8" pad  [5 bytes]
  ATM_REPEAT(1, 9),   // 2x pad4
  ATM_GOTO(8),   // -> pad

  //"Track 9" pad4  [24 bytes]
  ATM_VOL(24),
  ATM_SL_VOL((uint8_t)-1),
  ATM_ARP(0x43, 0x23),
  ATM_NOTE_D3,
  ATM_DELAY(32),
  ATM_NOTE_A2,
  ATM_DELAY(32),
  ATM_NOTE_C3,
  ATM_DELAY(32),
  ATM_NOTE_G2,
  ATM_DELAY(32),
  ATM_NOTE_D3,
  ATM_DELAY(32),
  ATM_NOTE_A2,
  ATM_DELAY(32),
  ATM_NOTE_C3,
  ATM_DELAY(32),
  ATM_NOTE_G2,
  ATM_DELAY(32),
  ATM_RETURN,

  //"Track 10" bass  [7 bytes]
  ATM_SL_VOL((uint8_t)-2),
  ATM_REPEAT(1, 11),   // 2x bass4
  ATM_GOTO(10),   // -> bass

  //"Track 11" bass4  [17 bytes]
  ATM_SET_TRA(0),
  ATM_GOTO(12),   // -> bar
  ATM_SET_TRA((uint8_t)-2),
  ATM_GOTO(12),   // -> bar
  ATM_SET_TRA(0),
  ATM_GOTO(12),   // -> bar
  ATM_SET_TRA((uint8_t)-2),
  ATM_GOTO(12),   // -> bar
  ATM_RETURN,

  //"Track 12" bar  [13 bytes]
  ATM_GOTO(13),   // -> beat1
  ATM_GOTO(14),   // -> beat
  ATM_ADD_TRA(7),
  ATM_GOTO(14),   // -> beat
  ATM_GOTO(14),   // -> beat
  ATM_ADD_TRA((uint8_t)-7),
  ATM_RETURN,

  //"Track 13" beat1  [15 bytes]
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(56),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(56),
  ATM_NOTE_D2,
  ATM_DELAY(4),
  ATM_RETURN,

  //"Track 14" beat  [13 bytes]
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(56),
  ATM_NOTE_D2,
  ATM_DELAY(4),
  ATM_NOTE_D3,
  ATM_DELAY(4),
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_RETURN,

};

#endif
