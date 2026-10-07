#ifndef SONG_H
#define SONG_H

// ---------------------------------------------------------------------------
//  "Quest Theme" - chiptune in-game loop, A minor, 16 bars, 247 bytes
//
//  Play with:   #include "quest.h"      ATM.play(questTheme);
//
//  Timing : ATM_SET_TEMPO(34) (doubled).  1 beat = 16 ticks, 1 bar = 64 ticks,
//           16 bars = 1024 ticks. Every channel sums to 1023 ticks of delay + the 1 tick
//           ATMlib spends restarting, so the loop is gapless.
//  Form   : bars 1-8   Am | Am | Dm | Dm | Am | Am | Em | Em
//           bars 9-16  the cycle lifted a whole step: Bm | Bm | Em | Em | Bm | Bm | Em | Em
//                      (Em is chord iv in Bm and chord v in Am: it pivots back into bar 1)
//  Channels (ATMlib defaults):
//     CH0 PULSE  - lead: ONE 2-bar riff, transposed per chord (SET_TRA / ADD_TRA)
//     CH1 SQUARE - minor-triad ARPEGGIO chords (explicit roots, no transpose on this channel)
//     CH2 SAW    - root/octave bass, transposed per chord, beat-4 variation
//     CH3 NOISE  - tempo + loop points + kick click, hats, snare
//  Note   : track 0 is only CH3's entry point and is never called. ATMlib starts every
//           channel with "current track = 0", so calling track 0 would break the call stack.
// ---------------------------------------------------------------------------

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif


Song questTheme[] = {     // total song bytes = 247
  0x0D,                       // Number of tracks
  0x00, 0x00,                 // Address of track 0      0   drums
  0x0E, 0x00,                 // Address of track 1     14   drum_core
  0x27, 0x00,                 // Address of track 2     39   drum_half
  0x2B, 0x00,                 // Address of track 3     43   lead
  0x42, 0x00,                 // Address of track 4     66   lead_riff
  0x60, 0x00,                 // Address of track 5     96   lead_cycle
  0x6E, 0x00,                 // Address of track 6    110   chords
  0x96, 0x00,                 // Address of track 7    150   bass
  0xAE, 0x00,                 // Address of track 8    174   bass_cycle
  0xBC, 0x00,                 // Address of track 9    188   bass_bar
  0xC2, 0x00,                 // Address of track 10   194   bass_bar_last
  0xCC, 0x00,                 // Address of track 11   204   bass_beat
  0xD1, 0x00,                 // Address of track 12   209   bass_beat4

  0x03,                         // CH0 entry -> track 3 (lead)
  0x06,                         // CH1 entry -> track 6 (chords)
  0x07,                         // CH2 entry -> track 7 (bass)
  0x00,                         // CH3 entry -> track 0 (drums)

  //"Track 0" drums  [14b]
  ATM_SET_TEMPO(34),
  ATM_GOTO_ADV(3, 6, 7, 0),   // loop point per channel
  ATM_REPEAT(30, 2),   // 31x drum_half
  ATM_GOTO(1),   // -> drum_core
  ATM_DELAY(7),
  ATM_STOP_CHAN,

  //"Track 1" drum_core  [25b]
  ATM_VOL(48),
  ATM_SL_VOL((uint8_t)-12),
  ATM_DELAY(4),
  ATM_VOL(14),
  ATM_SL_VOL((uint8_t)-7),
  ATM_DELAY(4),
  ATM_VOL(26),
  ATM_SL_VOL((uint8_t)-3),
  ATM_DELAY(8),
  ATM_VOL(44),
  ATM_SL_VOL((uint8_t)-5),
  ATM_DELAY(8),
  ATM_VOL(26),
  ATM_SL_VOL((uint8_t)-3),
  ATM_RETURN,

  //"Track 2" drum_half  [4b]
  ATM_GOTO(1),   // -> drum_core
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 3" lead  [23b]
  ATM_VOL(38),
  ATM_SL_VOL((uint8_t)-2),
  ATM_SET_TRA(0),
  ATM_GOTO(5),   // -> lead_cycle
  ATM_ADD_TRA(7),
  ATM_GOTO(4),   // -> lead_riff
  ATM_DELAY(16),
  ATM_SET_TRA(2),
  ATM_GOTO(5),   // -> lead_cycle
  ATM_ADD_TRA(5),
  ATM_GOTO(4),   // -> lead_riff
  ATM_DELAY(15),
  ATM_STOP_CHAN,

  //"Track 4" lead_riff  [30b]
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(12),
  ATM_NOTE_C5,
  ATM_DELAY(4),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_G4,
  ATM_DELAY(8),
  ATM_NOTE_A4,
  ATM_RETURN,

  //"Track 5" lead_cycle  [14b]
  ATM_GOTO(4),   // -> lead_riff
  ATM_DELAY(16),
  ATM_ADD_TRA(5),
  ATM_GOTO(4),   // -> lead_riff
  ATM_DELAY(16),
  ATM_ADD_TRA((uint8_t)-5),
  ATM_GOTO(4),   // -> lead_riff
  ATM_DELAY(16),
  ATM_RETURN,

  //"Track 6" chords  [40b]
  ATM_VOL(30),
  ATM_SL_VOL((uint8_t)-1),
  ATM_ARP(0x34, 0x20),
  ATM_NOTE_A3,
  ATM_DELAY(64),
  ATM_NOTE_A3,
  ATM_DELAY(64),
  ATM_NOTE_D4,
  ATM_DELAY(64),
  ATM_NOTE_D4,
  ATM_DELAY(64),
  ATM_NOTE_A3,
  ATM_DELAY(64),
  ATM_NOTE_A3,
  ATM_DELAY(64),
  ATM_NOTE_E4,
  ATM_DELAY(64),
  ATM_NOTE_E4,
  ATM_DELAY(64),
  ATM_NOTE_B3,
  ATM_DELAY(64),
  ATM_NOTE_B3,
  ATM_DELAY(64),
  ATM_NOTE_E4,
  ATM_DELAY(64),
  ATM_NOTE_E4,
  ATM_DELAY(64),
  ATM_NOTE_B3,
  ATM_DELAY(64),
  ATM_NOTE_B3,
  ATM_DELAY(64),
  ATM_NOTE_E4,
  ATM_DELAY(64),
  ATM_NOTE_E4,
  ATM_DELAY(63),
  ATM_STOP_CHAN,

  //"Track 7" bass  [24b]
  ATM_VOL(63),
  ATM_SL_VOL((uint8_t)-5),
  ATM_SET_TRA(0),
  ATM_GOTO(8),   // -> bass_cycle
  ATM_ADD_TRA(7),
  ATM_REPEAT(1, 9),   // 2x bass_bar
  ATM_SET_TRA(2),
  ATM_GOTO(8),   // -> bass_cycle
  ATM_SET_TRA(7),
  ATM_GOTO(9),   // -> bass_bar
  ATM_GOTO(10),   // -> bass_bar_last
  ATM_STOP_CHAN,

  //"Track 8" bass_cycle  [14b]
  ATM_REPEAT(1, 9),   // 2x bass_bar
  ATM_ADD_TRA(5),
  ATM_REPEAT(1, 9),   // 2x bass_bar
  ATM_ADD_TRA((uint8_t)-5),
  ATM_REPEAT(1, 9),   // 2x bass_bar
  ATM_RETURN,

  //"Track 9" bass_bar  [6b]
  ATM_REPEAT(2, 11),   // 3x bass_beat
  ATM_GOTO(12),   // -> bass_beat4
  ATM_RETURN,

  //"Track 10" bass_bar_last  [10b]
  ATM_REPEAT(2, 11),   // 3x bass_beat
  ATM_NOTE_A2,
  ATM_DELAY(8),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_NOTE_A3,
  ATM_DELAY(3),
  ATM_RETURN,

  //"Track 11" bass_beat  [5b]
  ATM_NOTE_A2,
  ATM_DELAY(8),
  ATM_NOTE_A3,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 12" bass_beat4  [7b]
  ATM_NOTE_A2,
  ATM_DELAY(8),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_NOTE_A3,
  ATM_DELAY(4),
  ATM_RETURN,

};

#endif
