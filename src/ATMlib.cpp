#include "ATMlib.h"

ATMLIB_CONSTRUCT_ISR(OCR4A)

byte trackCount;
byte tickRate;
const word *trackList;
const byte *trackBase;
uint8_t pcm __attribute__((used)) = 128;
bool half __attribute__((used));

byte ChannelActiveMute = 0b11110000;
//                         ||||||||
//                         |||||||└->  0  channel 0 is muted (0 = false / 1 = true)
//                         ||||||└-->  1  channel 1 is muted (0 = false / 1 = true)
//                         |||||└--->  2  channel 2 is muted (0 = false / 1 = true)
//                         ||||└---->  3  channel 3 is muted (0 = false / 1 = true)
//                         |||└----->  4  channel 0 is Active (0 = false / 1 = true)
//                         ||└------>  5  channel 1 is Active (0 = false / 1 = true)
//                         |└------->  6  channel 2 is Active (0 = false / 1 = true)
//                         └-------->  7  channel 3 is Active (0 = false / 1 = true)

//Imports
extern uint16_t cia;

// Exports
osc_t __attribute__((used)) osc[4];
uint8_t atmCue;
uint8_t atmScorePaused;
uint8_t atmSfxMask;

// Waveform types per channel: 0=PULSE, 1=SQUARE, 2=NOISE
// Defaults: Ch0=PULSE, Ch1=SQUARE, Ch2=PULSE, Ch3=NOISE


const word noteTable[64] PROGMEM = {
  0,
  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
  1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
  4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
  8372, 8870, 9397,
};


struct ch_t {
  const byte *ptr;
  byte note;

  // Nesting
  word stackPointer[7];
  byte stackCounter[7];
  byte stackTrack[7]; // note 1
  byte stackIndex;
  byte repeatPoint;

  // Looping
  word delay;
  byte counter;
  byte track;

  // External FX
  word freq;
  byte vol;

  // Volume & Frequency slide FX
  char volFreSlide;
  byte volFreConfig;
  byte volFreCount;

  // Arpeggio or Note Cut FX
  byte arpNotes;       // notes: base, base+[7:4], base+[7:4]+[3:0], if FF => note cut ON
  byte arpTiming;      // [7] = reserved, [6] = not third note ,[5] = retrigger, [4:0] = tick count
  byte arpCount;

  // Retrig FX
  byte reConfig;       // [7:2] = , [1:0] = speed // used for the noise channel
  byte reCount;        // also using this as a buffer for volume retrig on all channels

  // Transposition FX
  char transConfig;

  // Tremolo or Vibrato FX
  byte treviDepth;
  byte treviConfig;
  byte treviCount;

  // Glissando FX
  char glisConfig;
  byte glisCount;

};

ch_t channel[4];

ch_t sfxV;
uint8_t sfxOsc = 0xFF;

uint16_t read_vle(const byte **pp) {
  word q = 0;
  byte d;
  do {
    q <<= 7;
    d = pgm_read_byte(*pp++);
    q |= (d & 0x7F);
  } while (d & 0x80);
  return q;
}

static inline const byte *getTrackPointer(byte track) {
  return trackBase + pgm_read_word(&trackList[track]);
}


void ATMsynth::play(const byte *song) {
  TIMSK4 = 0b00000000;// ensure interrupt is disabled
  cia_count = 1;
  // cleanUp stuff first
  memset(channel, 0, sizeof(channel));
  ChannelActiveMute = 0b11110000;
  atmCue = 0;
  atmScorePaused = 0;
  atmSfxMask = 0;
  sfxOsc = 0xFF;
  // Defaults: Ch0=PULSE, Ch1=SQUARE, Ch2=SAW, Ch3=NOISE
  tickRate = 25;
  cia = 15625 / tickRate;
  // Sets up the ports, and the sample grinding ISR

  osc[3].freq = 0x0001; // Seed LFSR
  channel[3].freq = 0x0001; // xFX

  TCCR4A = 0b01000010;    // Fast-PWM 8-bit
  TCCR4B = 0b00000001;    // 62500Hz
  OCR4C  = 0xFF;          // Resolution to 8-bit (TOP=0xFF)
  OCR4A  = 0x80;
#if ATM_ALT_WIRING
  TCCR4C = 0b01000101;
  OCR4D  = 0x80;
#endif

  // Load a melody stream and start grinding samples
  // Read track count
  trackCount = pgm_read_byte(song++);
  // Store track list pointer
  trackList = (word*)song;
  // Store track pointer
  trackBase = (song += (trackCount << 1)) + 4;
  // Fetch starting points for each track
  for (unsigned n = 0; n < 4; n++) {
    channel[n].ptr = getTrackPointer(pgm_read_byte(song++));
  }
  TIMSK4 = 0b00000100;// enable interrupt as last
}

// Stop playing, unload melody
void ATMsynth::stop() {
  TIMSK4 = 0; // Disable interrupt
  memset(channel, 0, sizeof(channel));
  ChannelActiveMute = 0b11110000;
  atmCue = 0;
  atmScorePaused = 0;
  atmSfxMask = 0;
  sfxOsc = 0xFF;
}

// Start grinding samples or Pause playback
void ATMsynth::playPause() {
  if (atmScorePaused) resume();
  else pause();
}

void ATMsynth::pause() {
  if (atmScorePaused) return;
  atmScorePaused = 1;
  for (byte n = 0; n < 4; n++) {
    if (!(atmSfxMask & (1 << n))) osc[n].vol = 0;
  }
}

void ATMsynth::resume() {
  if (!atmScorePaused) return;
  atmScorePaused = 0;
  TIMSK4 |= 0b00000100;
}

// Toggle mute on/off on a channel, so it can be used for sound effects
// So you have to call it before and after the sound effect
void ATMsynth::muteChannel(byte ch) {
  ChannelActiveMute |= (uint8_t)(1 << ch);
}

void ATMsynth::unMuteChannel(byte ch) {
  ChannelActiveMute &= (uint8_t)(~(1 << ch));
}

void ATMsynth::setTempo(byte t)
{
  if (t == 0) t = 1;          // cia = 15625/t, never divide by 0
  tickRate = t;
  cia = 15625 / tickRate;
}

void ATMsynth::playSfx(const byte *track, byte ch) {
  if (ch > 3) return;

  if (!(TIMSK4 & 0b00000100)) {
    tickRate = tickRate ? tickRate : 25;
    cia = 15625 / tickRate;
    cia_count = 1;
    osc[3].freq = 0x0001;
    channel[3].freq = 0x0001;
    TCCR4A = 0b01000010;
    TCCR4B = 0b00000001;
    OCR4C  = 0xFF;
    OCR4A  = 0x80;
#if ATM_ALT_WIRING
    TCCR4C = 0b01000101;
    OCR4D  = 0x80;
#endif
    // no score loaded: do not fetch song commands from ptr == 0
    atmScorePaused = 1;
    ChannelActiveMute = 0b11110000;
  }

  if (sfxOsc < 4 && sfxOsc != ch)
    atmSfxMask &= (uint8_t)(~(1 << sfxOsc));
  memset(&sfxV, 0, sizeof(sfxV));
  sfxV.ptr = track;
  sfxOsc = ch;
  atmSfxMask = (uint8_t)(1 << ch);
  if (ch == 3) osc[3].freq = 0x0001;

  TIMSK4 |= 0b00000100;
}



__attribute__((used))
void ATM_playroutine() {
  ch_t *ch;

  for (byte pass = 0; pass < 2; pass++)
  for (byte n = 0; n < 4; n++)
  {
    byte out = 1;
    if (pass == 0) {
      ch = &channel[n];
      if (atmScorePaused) {
        if (!(atmSfxMask & (1 << n))) osc[n].vol = 0;
        continue;
      }
      out = !(atmSfxMask & (1 << n));
    } else {
      if (sfxOsc != n) continue;
      ch = &sfxV;
    }

    // Noise retriggering: reseed the LFSR (always osc[3].freq in the ISR)
    if (ch->reConfig) {
      if (ch->reCount >= (ch->reConfig & 0x03)) {
        osc[3].freq = pgm_read_word(&noteTable[ch->reConfig >> 2]);
        ch->reCount = 0;
      }
      else ch->reCount++;
    }


    //Apply Glissando
    if (ch->glisConfig) {
      if (ch->glisCount >= (ch->glisConfig & 0x7F)) {
        if (ch->glisConfig & 0x80) ch->note -= 1;
        else ch->note += 1;
        if (ch->note < 1) ch->note = 1;
        else if (ch->note > 63) ch->note = 63;
        ch->freq = pgm_read_word(&noteTable[ch->note]);
        ch->glisCount = 0;
      }
      else ch->glisCount++;
    }


    // Apply volume/frequency slides
    if (ch->volFreSlide) {
      if (!ch->volFreCount) {
        int16_t vf = ((ch->volFreConfig & 0x40) ? ch->freq : ch->vol);
        vf += (ch->volFreSlide);
        if (!(ch->volFreConfig & 0x80)) {
          if (vf < 0) vf = 0;
          else if (ch->volFreConfig & 0x40) if (vf > 9397) vf = 9397;
            else if (!(ch->volFreConfig & 0x40)) if (vf > 63) vf = 63;
        }
        (ch->volFreConfig & 0x40) ? ch->freq = vf : ch->vol = vf;
      }
      if (ch->volFreCount++ >= (ch->volFreConfig & 0x3F)) ch->volFreCount = 0;
    }


    // Apply Arpeggio or Note Cut
    if (ch->arpNotes && ch->note) {
      if ((ch->arpCount & 0x1F) < (ch->arpTiming & 0x1F)) ch->arpCount++;
      else {
        if ((ch->arpCount & 0xE0) == 0x00) ch->arpCount = 0x20;
        else if ((ch->arpCount & 0xE0) == 0x20 && !(ch->arpTiming & 0x40) && (ch->arpNotes != 0xFF)) ch->arpCount = 0x40;
        else ch->arpCount = 0x00;
        byte arpNote = ch->note;
        if ((ch->arpCount & 0xE0) != 0x00) {
          if (ch->arpNotes == 0xFF) arpNote = 0;
          else arpNote += (ch->arpNotes >> 4);
        }
        if ((ch->arpCount & 0xE0) == 0x40) arpNote += (ch->arpNotes & 0x0F);
        ch->freq = pgm_read_word(&noteTable[arpNote + ch->transConfig]);
      }
    }


    // Apply Tremolo or Vibrato
    if (ch->treviDepth) {
      int16_t vt = ((ch->treviConfig & 0x40) ? ch->freq : ch->vol);
      vt = (ch->treviCount & 0x80) ? (vt + ch->treviDepth) : (vt - ch->treviDepth);
      if (vt < 0) vt = 0;
      else if (ch->treviConfig & 0x40) if (vt > 9397) vt = 9397;
        else if (!(ch->treviConfig & 0x40)) if (vt > 63) vt = 63;
      (ch->treviConfig & 0x40) ? ch->freq = vt : ch->vol = vt;
      if ((ch->treviCount & 0x1F) < (ch->treviConfig & 0x1F)) ch->treviCount++;
      else {
        if (ch->treviCount & 0x80) ch->treviCount = 0;
        else ch->treviCount = 0x80;
      }
    }


    if (ch->delay) {
      if (ch->delay != 0xFFFF) ch->delay--;
    }
    else {
      do {
        byte cmd = pgm_read_byte(ch->ptr++);
        if (cmd < 64) {
          // 0 … 63 : NOTE ON/OFF
          if (ch->note = cmd) ch->note += ch->transConfig;
          ch->freq = pgm_read_word(&noteTable[ch->note]);
          if (!ch->volFreConfig) ch->vol = ch->reCount;
          if (ch->arpTiming & 0x20) ch->arpCount = 0; // ARP retriggering
        }
        else if (cmd < 160) {
          // 64 … 159 : SETUP FX
          switch (cmd - 64) {
            case 0: // Set volume
              ch->vol = pgm_read_byte(ch->ptr++);
              ch->reCount = ch->vol;
              break;
            case 1: case 4: // Slide volume/frequency ON
              ch->volFreSlide = pgm_read_byte(ch->ptr++);
              ch->volFreConfig = (cmd - 64) == 1 ? 0x00 : 0x40;
              break;
            case 2: case 5: // Slide volume/frequency ON advanced
              ch->volFreSlide = pgm_read_byte(ch->ptr++);
              ch->volFreConfig = pgm_read_byte(ch->ptr++);
              if ((cmd - 64) == 5) ch->volFreConfig |= 0x40;
              break;
            case 3: case 6: // Slide volume/frequency OFF (same as 0x01 0x00)
              ch->volFreSlide = 0;
              break;
            case 7: // Set Arpeggio
              ch->arpNotes = pgm_read_byte(ch->ptr++);    // 0x40 + 0x03
              ch->arpTiming = pgm_read_byte(ch->ptr++);   // 0x40 (no third note) + 0x20 (toggle retrigger) + amount
              break;
            case 8: // Arpeggio OFF
              ch->arpNotes = 0;
              break;
            case 9: // Set Retriggering (noise)
              ch->reConfig = pgm_read_byte(ch->ptr++);    // RETRIG: point = 1 (*4), speed = 0 (0 = fastest, 1 = faster , 2 = fast)
              break;
            case 10: // Retriggering (noise) OFF
              ch->reConfig = 0;
              break;
            case 11: // ADD Transposition
              ch->transConfig += (char)pgm_read_byte(ch->ptr++);
              break;
            case 12: // SET Transposition
              ch->transConfig = pgm_read_byte(ch->ptr++);
              break;
            case 13: // Transposition OFF
              ch->transConfig = 0;
              break;
            case 14: case 16: // SET Tremolo/Vibrato
              ch->treviDepth = pgm_read_word(ch->ptr++);
              ch->treviConfig = pgm_read_word(ch->ptr++) + ((cmd - 64) == 14 ? 0x00 : 0x40);
              break;
            case 15: case 17: // Tremolo/Vibrato OFF
              ch->treviDepth = 0;
              break;
            case 18: // Glissando
              ch->glisConfig = pgm_read_byte(ch->ptr++);
              break;
            case 19: // Glissando OFF
              ch->glisConfig = 0;
              break;
            case 20: // SET Note Cut
              ch->arpNotes = 0xFF;                        // 0xFF use Note Cut
              ch->arpTiming = pgm_read_byte(ch->ptr++);   // tick amount
              break;
            case 21: // Note Cut OFF
              ch->arpNotes = 0;
              break;
            case 23: // CUE: store a byte for the sketch
              atmCue = pgm_read_byte(ch->ptr++);
              break;
            case 92: // ADD tempo
              tickRate += pgm_read_byte(ch->ptr++);
              cia = 15625 / tickRate;
              break;
            case 93: // SET tempo
              tickRate = pgm_read_byte(ch->ptr++);
              cia = 15625 / tickRate;
              break;
            case 94: // Goto advanced
              for (byte i = 0; i < 4; i++) channel[i].repeatPoint = pgm_read_byte(ch->ptr++);
              break;
            case 95: // Stop channel
              if (pass) {
                ch->vol = 0;
                ch->delay = 0xFFFF;
                sfxOsc = 0xFF;
                atmSfxMask &= (uint8_t)(~(1 << n));
              } else {
                ChannelActiveMute ^= (uint8_t)(1 << (n + 4));
                ch->vol = 0;
                ch->delay = 0xFFFF;
              }
              break;
          }
        } else if (cmd < 224) {
          // 160 … 223 : DELAY
          ch->delay = cmd - 159;
        } else if (cmd == 224) {
          // 224: LONG DELAY
          ch->delay = read_vle(&ch->ptr) + 65;
        } else if (cmd < 252) {
          // 225 … 251 : RESERVED
        } else if (cmd == 252 || cmd == 253) {
          // 252 (253) : CALL (REPEATEDLY)
          byte new_counter = cmd == 252 ? 0 : pgm_read_byte(ch->ptr++);
          byte new_track = pgm_read_byte(ch->ptr++);

          if (new_track != ch->track) {
            // Stack PUSH
            ch->stackCounter[ch->stackIndex] = ch->counter;
            ch->stackTrack[ch->stackIndex] = ch->track; // note 1
            ch->stackPointer[ch->stackIndex] = ch->ptr - trackBase;
            ch->stackIndex++;
            ch->track = new_track;
          }

          ch->counter = new_counter;
          ch->ptr = getTrackPointer(ch->track);
        } else if (cmd == 254) {
          // 254 : RETURN
          if (ch->counter > 0 || ch->stackIndex == 0) {
            // Repeat track
            if (ch->counter) ch->counter--;
            ch->ptr = getTrackPointer(ch->track);
            //asm volatile ("  jmp 0"); // reboot
          } else {
            // Check stack depth
            if (ch->stackIndex == 0) {
              // Stop the channel
              ch->delay = 0xFFFF;
            } else {
              // Stack POP
              ch->stackIndex--;
              ch->ptr = ch->stackPointer[ch->stackIndex] + trackBase;
              ch->counter = ch->stackCounter[ch->stackIndex];
              ch->track = ch->stackTrack[ch->stackIndex]; // note 1
            }
          }
        } else if (cmd == 255) {
          // 255 : EMBEDDED DATA
          ch->ptr += read_vle(&ch->ptr);
        }
      } while (ch->delay == 0);

      if (ch->delay != 0xFFFF) ch->delay--;
    }

    if (out && !(ChannelActiveMute & (1 << n))) {
#if ATM_WAVE_CH3 == ATM_WAVE_NOISE
      if (n == 3) {
        osc[n].vol = ch->vol >> 1;
      } else
#endif
      {
        osc[n].freq = ch->freq;
        osc[n].vol = ch->vol;
      }
    }
    // if all channels are inactive, stop playing or check for repeat
    
    if (pass == 0 && !(ChannelActiveMute & 0xF0))
    {
      byte repeatSong = 0;
      for (byte j = 0; j < 4; j++) repeatSong += channel[j].repeatPoint;
      if (repeatSong) {
        for (byte k = 0; k < 4; k++) {
          channel[k].ptr = getTrackPointer(channel[k].repeatPoint);
          channel[k].delay = 0;
        }
        ChannelActiveMute = 0b11110000;
      }
      else if (sfxOsc >= 4)
      {
        memset(channel, 0, sizeof(channel));
        TIMSK4 = 0; // Disable interrupt
      }
    }
  }
}
uint8_t ATMsynth::check() {
  uint8_t v = atmCue;
  atmCue = 0;
  return v;
}

uint8_t ATMsynth::check(uint8_t id) {
  return atmCue == id;
}
