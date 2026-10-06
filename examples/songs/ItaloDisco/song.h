#ifndef SONG_H
#define SONG_H

#define Song const uint8_t PROGMEM

#ifndef NEWSONG_H
#define NEWSONG_H

// ---------------------------------------------------------------------------
//  "Italo Night" - 8 bars, A minor, ~127.5 BPM, ~15.06 s, loops seamlessly
//
//  Play with:   #include "newsong.h"      ATM.play(italoNight);
//
//  Timing : ATM_SET_TEMPO(17) -> ~34 ticks/s.  1 beat = 16 ticks, 1 bar = 64 ticks
//           8 bars = 512 ticks (every channel sums to 511 ticks of delay + 1 tick
//           that ATMlib spends restarting, so the loop is gapless).
//  Chords : Am | F | C | G | Am | F | G | E
//  Channels (ATMlib defaults):
//     CH0 PULSE  - lead melody
//     CH1 SQUARE - pitch-dropping kick on every beat + offbeat arpeggio chord stabs
//     CH2 SAW    - ducked octave bass (silent on the kick, then root-octave-root)
//     CH3 NOISE  - kick click, clap on 2 & 4, closed/open hats, snare roll fills (bars 4 and 8)
//  Pitch  : ATMlib's note table sounds about an octave above the note names and
//           ~80 cents flat (ATM_NOTE_A2 is ~210 Hz). Same for every ATMlib song.
// ---------------------------------------------------------------------------

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif


Song italoNight[] = {     // total song bytes = 558
  0x21,                       // Number of tracks
  0x00, 0x00,                 // Address of track 0      0   silent
  0x03, 0x00,                 // Address of track 1      3   lead
  0x18, 0x00,                 // Address of track 2     24   lead_bar1
  0x25, 0x00,                 // Address of track 3     37   lead_bar2
  0x32, 0x00,                 // Address of track 4     50   lead_bar3
  0x3F, 0x00,                 // Address of track 5     63   lead_bar4
  0x4C, 0x00,                 // Address of track 6     76   lead_bar5
  0x5D, 0x00,                 // Address of track 7     93   lead_bar6
  0x6E, 0x00,                 // Address of track 8    110   lead_bar7
  0x7F, 0x00,                 // Address of track 9    127   lead_bar8
  0x90, 0x00,                 // Address of track 10   144   chords
  0xAB, 0x00,                 // Address of track 11   171   kick
  0xB6, 0x00,                 // Address of track 12   182   beat_Am
  0xC2, 0x00,                 // Address of track 13   194   beat_F
  0xCE, 0x00,                 // Address of track 14   206   beat_C
  0xDA, 0x00,                 // Address of track 15   218   beat_G
  0xE6, 0x00,                 // Address of track 16   230   beat_E
  0xF2, 0x00,                 // Address of track 17   242   beat_E_last
  0xFE, 0x00,                 // Address of track 18   254   bass
  0x21, 0x01,                 // Address of track 19   289   bass_beat
  0x2D, 0x01,                 // Address of track 20   301   bass_beat4
  0x39, 0x01,                 // Address of track 21   313   bass_bar
  0x3F, 0x01,                 // Address of track 22   319   bass_beat4_last
  0x4B, 0x01,                 // Address of track 23   331   bass_bar_last
  0x51, 0x01,                 // Address of track 24   337   drums
  0x63, 0x01,                 // Address of track 25   355   drum_A
  0x73, 0x01,                 // Address of track 26   371   drum_B
  0x7E, 0x01,                 // Address of track 27   382   drum_pair
  0x83, 0x01,                 // Address of track 28   387   drum_bar
  0x87, 0x01,                 // Address of track 29   391   drum_fill
  0xB0, 0x01,                 // Address of track 30   432   drum_fill_last
  0xD9, 0x01,                 // Address of track 31   473   drum_bar_fill
  0xE0, 0x01,                 // Address of track 32   480   drum_bar_fill_last

  0x01,                         // Channel 0 entry track  (lead)
  0x0A,                         // Channel 1 entry track  (chords)
  0x12,                         // Channel 2 entry track  (bass)
  0x18,                         // Channel 3 entry track  (drums)

  //"Track 0" silent: unused dummy (never called; index 0 is the 'stale track' slot in ATMlib)
  ATM_VOL(0),
  ATM_STOP_CHAN,

  //"Track 1" lead: CH0 PULSE lead: setup + 8 bars
  ATM_VOL(36),
  ATM_SL_VOL((uint8_t)-2),
  ATM_GOTO(2),   // -> lead_bar1
  ATM_GOTO(3),   // -> lead_bar2
  ATM_GOTO(4),   // -> lead_bar3
  ATM_GOTO(5),   // -> lead_bar4
  ATM_GOTO(6),   // -> lead_bar5
  ATM_GOTO(7),   // -> lead_bar6
  ATM_GOTO(8),   // -> lead_bar7
  ATM_GOTO(9),   // -> lead_bar8
  ATM_STOP_CHAN,

  //"Track 2" lead_bar1: lead bar 1 (Am)
  ATM_NOTE_E5,
  ATM_DELAY(12),
  ATM_NOTE_C5,
  ATM_DELAY(12),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(12),
  ATM_NOTE_C5,
  ATM_DELAY(12),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 3" lead_bar2: lead bar 2 (F)
  ATM_NOTE_F5,
  ATM_DELAY(12),
  ATM_NOTE_C5,
  ATM_DELAY(12),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_NOTE_F5,
  ATM_DELAY(12),
  ATM_NOTE_C5,
  ATM_DELAY(12),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 4" lead_bar3: lead bar 3 (C)
  ATM_NOTE_E5,
  ATM_DELAY(12),
  ATM_NOTE_C5,
  ATM_DELAY(12),
  ATM_NOTE_G4,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(12),
  ATM_NOTE_G5,
  ATM_DELAY(12),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 5" lead_bar4: lead bar 4 (G)
  ATM_NOTE_D5,
  ATM_DELAY(12),
  ATM_NOTE_B4,
  ATM_DELAY(12),
  ATM_NOTE_G4,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(12),
  ATM_NOTE_B4,
  ATM_DELAY(12),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 6" lead_bar5: lead bar 5 (Am)
  ATM_NOTE_A4,
  ATM_DELAY(4),
  ATM_NOTE_C5,
  ATM_DELAY(4),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_A5,
  ATM_DELAY(16),
  ATM_NOTE_G5,
  ATM_DELAY(8),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 7" lead_bar6: lead bar 6 (F)
  ATM_NOTE_F4,
  ATM_DELAY(4),
  ATM_NOTE_A4,
  ATM_DELAY(4),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_F5,
  ATM_DELAY(16),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_C5,
  ATM_DELAY(8),
  ATM_NOTE_A4,
  ATM_DELAY(8),
  ATM_NOTE_F4,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 8" lead_bar7: lead bar 7 (G)
  ATM_NOTE_G4,
  ATM_DELAY(4),
  ATM_NOTE_B4,
  ATM_DELAY(4),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_G5,
  ATM_DELAY(16),
  ATM_NOTE_F5,
  ATM_DELAY(8),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_B4,
  ATM_DELAY(8),
  ATM_NOTE_G4,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 9" lead_bar8: lead bar 8 (E)
  ATM_NOTE_G4_,
  ATM_DELAY(4),
  ATM_NOTE_B4,
  ATM_DELAY(4),
  ATM_NOTE_E5,
  ATM_DELAY(8),
  ATM_NOTE_G5_,
  ATM_DELAY(16),
  ATM_NOTE_D5,
  ATM_DELAY(8),
  ATM_NOTE_B4,
  ATM_DELAY(8),
  ATM_NOTE_G4_,
  ATM_DELAY(8),
  ATM_NOTE_E4,
  ATM_DELAY(7),
  ATM_RETURN,

  //"Track 10" chords: CH1 SQUARE: kick on every beat + offbeat arpeggiated chord stabs
  ATM_REPEAT(3, 12),   // 4x beat_Am
  ATM_REPEAT(3, 13),   // 4x beat_F
  ATM_REPEAT(3, 14),   // 4x beat_C
  ATM_REPEAT(3, 15),   // 4x beat_G
  ATM_REPEAT(3, 12),   // 4x beat_Am
  ATM_REPEAT(3, 13),   // 4x beat_F
  ATM_REPEAT(3, 15),   // 4x beat_G
  ATM_REPEAT(2, 16),   // 3x beat_E
  ATM_GOTO(17),   // -> beat_E_last
  ATM_STOP_CHAN,

  //"Track 11" kick: kick: square wave with a fast downward pitch slide (4 ticks), then silence for 4 ticks
  ATM_ARP_OFF,
  ATM_SL_FRQ((uint8_t)-127),
  ATM_VOL(30),
  ATM_NOTE_D3_,
  ATM_DELAY(4),
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_RETURN,

  //"Track 12" beat_Am: 1 beat: kick + Am chord stab (8 ticks)
  ATM_GOTO(11),   // -> kick
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x34, 0x20),
  ATM_NOTE_A3,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 13" beat_F: 1 beat: kick + F chord stab (8 ticks)
  ATM_GOTO(11),   // -> kick
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x35, 0x20),
  ATM_NOTE_A3,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 14" beat_C: 1 beat: kick + C chord stab (8 ticks)
  ATM_GOTO(11),   // -> kick
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x54, 0x20),
  ATM_NOTE_G3,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 15" beat_G: 1 beat: kick + G chord stab (8 ticks)
  ATM_GOTO(11),   // -> kick
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x35, 0x20),
  ATM_NOTE_B3,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 16" beat_E: 1 beat: kick + E chord stab (8 ticks)
  ATM_GOTO(11),   // -> kick
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x35, 0x20),
  ATM_NOTE_G3_,
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 17" beat_E_last: last beat of the loop: stab is 1 tick shorter
  ATM_GOTO(11),   // -> kick
  ATM_SL_VOL((uint8_t)-3),
  ATM_VOL(24),
  ATM_ARP(0x35, 0x20),
  ATM_NOTE_G3_,
  ATM_DELAY(7),
  ATM_RETURN,

  //"Track 18" bass: CH2 SAW bass: root changes via SET_TRANSPOSE (A2 is the base root)
  ATM_SL_VOL((uint8_t)-5),
  ATM_SET_TRA(0),
  ATM_GOTO(21),   // -> bass_bar
  ATM_SET_TRA((uint8_t)-4),
  ATM_GOTO(21),   // -> bass_bar
  ATM_SET_TRA(3),
  ATM_GOTO(21),   // -> bass_bar
  ATM_SET_TRA((uint8_t)-2),
  ATM_GOTO(21),   // -> bass_bar
  ATM_SET_TRA(0),
  ATM_GOTO(21),   // -> bass_bar
  ATM_SET_TRA((uint8_t)-4),
  ATM_GOTO(21),   // -> bass_bar
  ATM_SET_TRA((uint8_t)-2),
  ATM_GOTO(21),   // -> bass_bar
  ATM_SET_TRA((uint8_t)-5),
  ATM_GOTO(23),   // -> bass_bar_last
  ATM_STOP_CHAN,

  //"Track 19" bass_beat: beat 1-3: duck on the kick, then root - octave - root
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(63),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_NOTE_A3,
  ATM_DELAY(4),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_RETURN,

  //"Track 20" bass_beat4: beat 4 variation: root - root - octave
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(63),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_NOTE_A3,
  ATM_DELAY(4),
  ATM_RETURN,

  //"Track 21" bass_bar: one bar of bass
  ATM_REPEAT(2, 19),   // 3x bass_beat
  ATM_GOTO(20),   // -> bass_beat4
  ATM_RETURN,

  //"Track 22" bass_beat4_last: beat 4 variation, last note 1 tick short (loop compensation)
  ATM_VOL(0),
  ATM_DELAY(4),
  ATM_VOL(63),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_NOTE_A2,
  ATM_DELAY(4),
  ATM_NOTE_A3,
  ATM_DELAY(3),
  ATM_RETURN,

  //"Track 23" bass_bar_last: last bar of bass
  ATM_REPEAT(2, 19),   // 3x bass_beat
  ATM_GOTO(22),   // -> bass_beat4_last
  ATM_RETURN,

  //"Track 24" drums: CH3 NOISE: tempo + loop points + 8 bars of drums
  ATM_SET_TEMPO(17),
  ATM_GOTO_ADV(1, 10, 18, 24),   // loop point per channel
  ATM_REPEAT(2, 28),   // 3x drum_bar
  ATM_GOTO(31),   // -> drum_bar_fill
  ATM_REPEAT(2, 28),   // 3x drum_bar
  ATM_GOTO(32),   // -> drum_bar_fill_last
  ATM_STOP_CHAN,

  //"Track 25" drum_A: beat 1/3: kick click, closed hat, open hat
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

  //"Track 26" drum_B: beat 2/4: clap, open hat
  ATM_VOL(40),
  ATM_SL_VOL((uint8_t)-5),
  ATM_DELAY(8),
  ATM_VOL(26),
  ATM_SL_VOL((uint8_t)-3),
  ATM_DELAY(8),
  ATM_RETURN,

  //"Track 27" drum_pair: beats 1+2
  ATM_GOTO(25),   // -> drum_A
  ATM_GOTO(26),   // -> drum_B
  ATM_RETURN,

  //"Track 28" drum_bar: one bar of drums
  ATM_REPEAT(1, 27),   // 2x drum_pair
  ATM_RETURN,

  //"Track 29" drum_fill: snare roll (beat 4 of bar 4)
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

  //"Track 30" drum_fill_last: snare roll (beat 4 of bar 8), 1 tick short
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

  //"Track 31" drum_bar_fill: bar 4: 3 beats + roll
  ATM_GOTO(27),   // -> drum_pair
  ATM_GOTO(25),   // -> drum_A
  ATM_GOTO(29),   // -> drum_fill
  ATM_RETURN,

  //"Track 32" drum_bar_fill_last: bar 8: 3 beats + roll (loop compensated)
  ATM_GOTO(27),   // -> drum_pair
  ATM_GOTO(25),   // -> drum_A
  ATM_GOTO(30),   // -> drum_fill_last
  ATM_RETURN,

};

#endif


#endif
