# LearnATMlib: how to write a song for ATMlib (Arduboy)

**Audience:** a person or an AI starting from zero.
**Status:** every statement here was checked against the ATMlib source (`ATMcmds.h`, `ATMlibimpl.h`, `README.md`) and against songs the user approved (Italo Night, Escaper Droid, Midnight Run, Quest Theme). Where something is an empirical finding rather than source code, it says so.
**How to use this file:** give a fresh AI this file plus the ATMlib folder (and, ideally, `song.h`, the reference song). Say what you want (style, key, length, byte budget, loop or one-shot). The AI should read sections 0, 4, 6 and 9 first.

---

## 0. Start here

### 0.1 The user's house rules (always apply)

1. **Double the tempo.** Wherever an emulator or offline analysis says `17`, write `ATM_SET_TEMPO(34)`. On the real Arduboy the song plays at about *tempo-value* ticks per second (see 5.1). Quest Theme uses 34.
2. **Deliver only the song `.h`.** No WAV file unless asked.
3. **Write songs as commands**, e.g. `ATM_NOTE_A4, ATM_DELAY(8), ATM_VOL(38)`. Never write track bodies as raw hex. The small header (track count, track addresses, channel entry tracks) is hex, exactly as in the reference `song.h`.
4. **Byte budget means the whole array**, including the header. "Max 250 bytes" = `sizeof(song) <= 250`.
5. **Loops must be gapless.** The user hears a one-tick error at the loop point (30 ms). See section 6.
6. **Tempo is the user's to set when they say so.** If they set it from the sketch (`ATM.setTempo()`), leave `ATM_SET_TEMPO` out of the song (`quest.h` has none now: without it the song plays at ATMlib's default of 25, not 34).
7. **Quality bar:** Italo Night, Midnight Run and Quest Theme were "really good / perfect". Pixel Chase was rejected: too plain, and actually broken (section 12).

### 0.2 Starter prompt for a fresh session

> Read `LearnATMlib.md` completely, then `ATMlib/src/ATMcmds.h` and `song.h`. Write a new ATMlib song: *(style, mood, key, number of bars or length, loop or one-shot, byte budget)*. Follow section 0.3, verify with the simulator, compile-check the header, and deliver only the `.h` in `ATM_*` commands with the doubled tempo.

### 0.3 The workflow in ten lines

1. Read `ATMcmds.h` for the real opcode values. Never guess them (section 12, mistake 1).
2. Decide: tempo (doubled), key, number of bars, loop or one-shot, byte budget.
3. Time grid: **16 ticks = 1 beat, 64 ticks = 1 bar** (used by all approved songs).
4. Plan every channel in ticks. For a loop of `L` ticks every channel must contain exactly `L-1` ticks of delay (section 6).
5. Write the song with a **generator script** (named tracks, automatic track numbers and offsets). Appendix B is a complete example.
6. Run the **simulator** (Appendix A): all channels stop on the same tick, restart cadence equals `L`, three loops are identical, call depth is at most 7, no note out of range.
7. Print the notes per bar and check harmony and register by eye.
8. Emit the `.h` with `ATM_*` macros and **compile it against the real `ATMcmds.h`**, then compare the compiled bytes with the generator's bytes (Appendix C).
9. Check the byte budget.
10. Deliver the `.h` only, with a short summary (form, channels, size).

---

## 1. Mental model

* ATMlib is a 4-channel tracker-style music player for the Arduboy. Default voices: **CH0 PULSE, CH1 SQUARE, CH2 SAW, CH3 NOISE**. Waveforms are fixed at compile time (`ATMconfig.h`), not per song.
* Time is counted in **ticks**. Tempo sets the tick rate.
* A song is a set of **tracks** (byte sequences, like subroutines). Each channel starts at one *entry track* and runs commands one after another. Tracks can call other tracks (`GOTO` = call and return, `REPEAT` = call n+1 times).
* Almost everything is **state that persists** until you change it: volume, volume slide, transposition, arpeggio. This is the source of most bugs and most savings.
* A note plays until the next note or the next `VOL(0)`. A `DELAY` makes the channel wait; the sound keeps going meanwhile.

---

## 2. Song data format

```
byte 0               N = number of tracks
bytes 1 .. 2N        N x uint16 little-endian: offset of each track,
                     relative to the first byte AFTER the 4 channel-entry bytes
next 4 bytes         entry track number for CH0, CH1, CH2, CH3
then                 track data, tracks back to back, in table order
```

* **Header cost = 1 + 2N + 4 bytes.** Every extra track costs 2 bytes of table plus its closing `RETURN`. The 250-byte budgets are tight mostly because of this.
* Tracks are addressed by index 0..N-1 (max 255 tracks).
* An *entry track* ends with `ATM_STOP_CHAN`. A *called track* ends with `ATM_RETURN`.
* The song array must be `const uint8_t PROGMEM`.

---

## 3. Command reference

Values are from `ATMcmds.h` (the source of truth; the README table agrees except where noted).

### 3.1 Core commands

| Macro | Bytes | Meaning |
|---|---|---|
| `ATM_NOTE_C2` .. `ATM_NOTE_D7` | 1 byte, value 1..63 | Start a note. C2=1, C#2=2 ... B2=12, C3=13, A3=22, A4=34, A5=46, C6=49, D7=63. Sharps use an **underscore**: `ATM_NOTE_F5_` is F#5. |
| raw `0x00` | 1 byte | Silence (note off). There is **no** `ATM_NOTE_OFF` macro. Often `ATM_VOL(0)` is used instead. |
| `ATM_DELAY(n)` | `0x9F+n` | Wait n ticks, **n = 1..64**. `ATM_DELAY(0)` is `0x9F` = STOP, so never use 0. |
| two delays in a row | | Valid and seamless: `ATM_DELAY(64), ATM_DELAY(10)` = 74 ticks. |
| `ATM_LONG_DELAY(n)` | **do not use** | The macro in `ATMcmds.h` emits `0x9F,n` (which is STOP). The real long-delay opcode is raw `0xE0, n` (n+65 ticks). Chained `ATM_DELAY` is simpler. |
| `ATM_GOTO(t)` | `0xFC, t` | Call track t; continue after it returns. |
| `ATM_REPEAT(n, t)` | `0xFD, n, t` | Call track t **n+1 times** in total. `REPEAT(3,t)` = 4 runs. |
| `ATM_RETURN` | `0xFE` | End of a called track. |
| `ATM_STOP_CHAN` | `0x9F` | Stop this channel. When all four have stopped, the song ends or restarts (4.4). |

### 3.2 Effects (all are `64+n`, i.e. `0x40+n`)

| Macro | Bytes | Meaning |
|---|---|---|
| `ATM_VOL(v)` | `0x40, v` | Set volume 0..63. Also stores v as the volume re-applied at every later note-on (see 4.2). |
| `ATM_SL_VOL(s)` | `0x41, s` | Add signed s to the volume **every tick** (clamped at 0 only, see 4.9: a positive slide is NOT clamped at 63). A negative value gives a decay after each note. |
| `ATM_SL_VOL_ADV(s, t)` | `0x42, s, t` | Slide s every t+1 ticks (`t` is the low 6 bits; bit 7 = no clamping at all). Used for long fades (4.9). **Caution:** with this form the volume is *not* re-applied at note-on (4.2). |
| `ATM_SL_VOL_OFF` | `0x43` | Stop the volume slide. |
| `ATM_SL_FRQ(s)` / `_ADV` / `_OFF` | `0x44` / `0x45` / `0x46` | Frequency slide (pitch drop for kicks: `-127`). Also disables note-on volume re-apply. |
| `ATM_ARP(a, y)` | `0x47, a, y` | Arpeggio, see 4.6. |
| `ATM_ARP_OFF` | `0x48` | Arpeggio off. |
| `ATM_NOISE(x)` / `ATM_NOISE_OFF` | `0x49, x` / `0x4A` | Noise retrigger: `x = AAAAAABB` entry point + speed (0 fastest). CH3 only. |
| `ATM_ADD_TRA(d)` | `0x4B, d` | Add signed d to the transposition. |
| `ATM_SET_TRA(v)` | `0x4C, v` | Set the transposition to signed v (semitones). |
| `ATM_TRA_OFF` | `0x4D` | Transposition = 0. |
| `ATM_TREM(depth, rate)` / `_OFF` | `0x4E, d, r` / `0x4F` | Tremolo: volume moves by `depth` every tick, reversing every rate+1 ticks (triangle). Shares state with vibrato, so one of the two per channel. |
| `ATM_VIB(depth, rate)` / `_OFF` | `0x50, d, r` / `0x51` | Vibrato: frequency (Hz) moves by `depth` every tick, reversing every rate+1 ticks. `VIB(3,3)` is about +-24 cents at 880 Hz. |
| `ATM_GLIS(x)` / `_OFF` | `0x52, x` / `0x53` | Glissando; bit 7 = direction (1 = down), low 7 bits = ticks per semitone step. |
| `ATM_CUT(n)` / `_OFF` | `0x54, n` / `0x55` | Staccato **gate**: sound for n+1 ticks, silence for n+1 ticks, repeating. Use `0x20+n` so every note-on restarts it. Shares state with the arpeggio (not both on one channel) and **only works with transposition = 0** (4.9). |
| `ATM_CUE(v)` | `0x57, v` | Hands a byte to the sketch (`ATM.check()`). |
| `ATM_ADD_TEMPO(v)` | `0x9C, v` | Add to the tempo (the tempo is a byte, so `(uint8_t)-1` subtracts 1). Ritardando: one `ADD_TEMPO(-1)` per beat. |
| `ATM_SET_TEMPO(v)` | `0x9D, v` | Set the tempo, 0..127, default 25. |
| `ATM_GOTO_ADV(a,b,c,d)` | `0x9E, a,b,c,d` | Set the **loop-restart track** for CH0..CH3 (4.4). Executed from any channel; applies to all four. |
| `ATM_WAVEFORM(t)` | **do not use** | Defined in the header (`0x56`) but the playroutine has no handler for it, so the parameter byte would be run as a command. Waveforms are compile-time (`ATMconfig.h`). |

### 3.3 Writing rules for the `.h`

* Negative arguments need a cast: `ATM_SL_VOL((uint8_t)-5)`, `ATM_ADD_TRA((uint8_t)-4)`. Plain `-5` fails strict C++ narrowing.
* Sharps: `ATM_NOTE_F5_`, never `ATM_NOTE_F#5`.
* Use the macro for every note and delay; comment each track.

---

## 4. How the playroutine really behaves

All of this is from `ATMlibimpl.h`. A Python model of it is Appendix A.

### 4.1 Tick order and delay

Each tick, for channel 0, 1, 2, 3 in that order: apply effects (slides, arpeggio, ...), then, if the channel's delay is 0, run commands until one sets a delay. `ATM_DELAY(d)` makes the note/state last exactly **d ticks**. When a delay expires, the next command runs in that same tick, so chained delays and calls cost no extra tick.

### 4.2 Notes, volume and transposition

* A note value 1..63 sets the pitch to `note + transposition` **at the moment of the note-on**. Changing the transposition later does not move a sounding note.
* **Keep `note + transposition` inside 1..63.** With all octaves compiled in, the pitch table is read without bounds checking, so out-of-range values read garbage.
* At every note-on the volume is reset to the last `ATM_VOL(v)` value, **but only when the active slide is a plain `ATM_SL_VOL`/none** (config byte 0). With `SL_VOL_ADV` or any frequency slide active the volume is not reset. This is what makes "VOL(38) + SL_VOL(-2)" a plucky lead: every note starts at 38 and decays by 2 per tick.
* Volume range 0..63 per channel. The noise channel's volume is halved in the mixer. Very loud sums of all channels can distort.

### 4.3 Calls, the call stack and the track-0 trap

* `GOTO`/`REPEAT` push a return frame (counter, track, position). Maximum depth **7**.
* A call to the track the channel is *currently in* does **not** push; it just restarts that track.
* **Every channel starts with "current track = 0", whatever its entry track is.** So a channel that calls track 0 from its entry track does not push a frame. When track 0 then returns with an empty stack, it simply restarts itself forever.
  **Rule: never call track 0.** Track 0 may be used only as a channel entry (CH3 in Quest Theme) or as a dummy that nobody calls (the older songs have a "silent" track 0).
  *Measured example:* the rejected `pixelchase.h` had its melody bar as track 0. In the simulator CH0 plays bars 1-6 correctly, then repeats bar 1 forever and never stops, while the other three channels stop at tick 511 and the song never restarts.
* `RETURN` with a repeat counter above 0, or with an empty stack, **restarts the current track** instead of leaving it. So ending an entry track with `RETURN` instead of `STOP` loops that track forever.
* `REPEAT(n,t)` runs t n+1 times. Nested repeats are fine (counters are saved in the frame).

### 4.4 Stop, end of song and looping

* `STOP` silences the channel. When all four channels have stopped:
  * if the sum of the four `GOTO_ADV` restart tracks is 0 (or `GOTO_ADV` was never executed) the song **ends**;
  * otherwise every channel jumps to its restart track and the song **continues**.
* So: **a looping song contains `ATM_GOTO_ADV`, a one-shot song must not.** (Midnight Run, a 3-minute one-shot, has none. When Claude once left it in, the song restarted.)
* Put `ATM_GOTO_ADV(ch0, ch1, ch2, ch3)` near the start of the drum track. After a restart it runs again, which is harmless. An intro followed by a repeating part is made by pointing the restart tracks at the repeating part.
* The restart is decided right after the last channel stops, inside the channel loop. If the last channel to stop is not CH3, the channels after it start the new loop **one tick earlier** than the ones before it. Therefore **all four channels must stop on the same tick** so that CH3 is the last one processed. Then the new loop starts exactly one tick after the stop tick.
* The restart does **not** reset volume, slides, arpeggio or transposition. Set what you need at the start of each channel's main track (typically `ATM_SET_TRA(0)`).

### 4.5 Slides

`SL_VOL(s)` changes volume by s every tick. `SL_FRQ(-127)` on a square note makes the classic pitch-drop kick (Italo Night). After `SL_FRQ` the volume is not re-applied at note-on, so set `ATM_VOL` explicitly before the note.

### 4.6 Arpeggio

`ATM_ARP(a, y)` plays three notes in rotation from the note you start:

* `a = AAAABBBB`: second note = base + A semitones; third = second + B.
* `y = FEDttttt`: `D` (0x20) = restart the rotation at every note-on; `E` (0x40) = no third note (two-note arp); `ttttt` = ticks per step minus one. `0x20` means one tick per step (about 11 rotations per second at tempo 34); `0x21` two ticks, etc.
* Chord values (README `extras/notesInChords.md` + inversions):

| Chord | `a` | Notes from base |
|---|---|---|
| major, root position | `0x43` | root, +4, +7 |
| minor, root position | `0x34` | root, +3, +7 |
| major, 1st inversion (3rd in bass) | `0x35` | e.g. A C F = F/A |
| major, 2nd inversion (5th in bass) | `0x54` | e.g. G C E = C/G |
| minor, 1st inversion | `0x45` | e.g. C E A = Am/C |
| minor, 2nd inversion | `0x53` | e.g. E A C = Am/E |
| maj7 colour (no 5th) | `0x47` | root, +4, +11 (used in Escaper Droid) |
| m7 colour (no 5th) | `0x37` | root, +3, +10 |

* **Inversions keep the stabs in one register.** Italo Night plays every chord from around G3..B3 using `0x34 / 0x35 / 0x54` with a changing base note.
* **Do not combine `ATM_ARP` with transposition on the same channel.** During arpeggio steps the playroutine adds the transposition a second time (`ATMlibimpl.h`, the line `ATM__FREQ(arpNote + ch->transConfig)` while `arpNote` already includes it). Put the arpeggio on one channel with explicit root notes, and use transposition on other channels.
* `ATM_NOTE_x` after `ATM_ARP` sets the base note; a new note restarts the rotation when `0x20` is set.

### 4.7 Noise channel (CH3)

CH3 is noise; drums are made purely with `VOL` + `SL_VOL` + `DELAY` (no notes needed). Volume is halved in the mixer. See section 8.1 for the approved recipes.

### 4.8 Known traps in the library files

* `ATM_DELAY(0)` is the STOP byte.
* `ATM_LONG_DELAY` macro is wrong; `ATM_WAVEFORM` is unhandled (section 3).
* The README says note 1 is "C1"; the header and table use **C2 = 1**.
* README says tempo values are 0..127 and "the higher the tempo, the more CPU".

### 4.9 Effect details verified in `ATMlibimpl.h` (used by Cutlass Cove)

Per tick, per channel, the order is: noise retrigger, glissando, volume/frequency slide, arpeggio/cut, tremolo/vibrato, then the command stream.

* **One slide slot per channel.** `SL_VOL`, `SL_VOL_ADV`, `SL_FRQ`, `SL_FRQ_ADV` share a single slide. Starting one replaces the other. Cannon boom on the bass: `ATM_SL_FRQ(-24)` for 16 ticks, then `ATM_SL_VOL(-3)` both ends the pitch slide and starts the volume decay.
* **Positive volume slides are not clamped at 63** (a dangling `else` in the library: only freq is clamped above). `vol` is a byte, so a fade-in must be **stopped on purpose** (`ATM_SL_VOL_OFF`, or a new `ATM_SL_VOL`) before it reaches 63. Negative slides clamp at 0. Same for tremolo.
* **Fades:** `ATM_VOL(0), ATM_SL_VOL_ADV(1, 8)` adds +1 every 9 ticks and, because the config byte is non-zero, notes do not reset the volume, so the fade runs straight through note-ons. Fade-out: `ATM_SL_VOL_ADV((uint8_t)-1, 5)` from a starting `ATM_VOL`. `SL_VOL_ADV` and `SL_FRQ` set the config byte, so afterwards use plain `ATM_SL_VOL(x)` (which sets config 0) before expecting note-on volume resets again.
* **Frequency slide** `SL_FRQ(s)` moves the frequency by s **Hz per tick** (table of `extras/frequencyToTone.md`); clamped 0..9397.
* **Tremolo** is a triangle LFO on the volume: the first half goes *down*. `ATM_TREM(1, 15)` = sea swell with a 32-tick period. `ATM_TREM(6, 0)` on a rising volume (`VOL(10), SL_VOL(2)`) = a snare roll in 8 bytes; end it with `ATM_TREM_OFF`.
* **Vibrato** is the same on the frequency. A new note restarts the frequency, the LFO phase carries on, so the vibrato stays bounded.
* **Glissando** `ATM_GLIS(x)`: moves the note by one semitone every `(x & 0x7F)+1` ticks, upward, or downward when bit 7 is set (`0x82` = down, 3 ticks per step); stops at 1 and 63. End it with `ATM_GLIS_OFF`.
* **Note cut is a repeating gate:** after the note-on it sounds for n+1 ticks, is silent for n+1 ticks, then sounds again. With `ATM_CUT(0x23)` and 8-tick notes you get 4 ticks on, 4 off, exactly staccato. Notes longer than 2*(n+1) ticks come back to life, so keep the notes at one length. **With a non-zero transposition the "silence" plays note number = transposition** (the table is read at index 0+tra, negative = garbage), so use `CUT` only where transposition is 0.
* **Noise retrigger** `ATM_NOISE(x)` (x = entry point * 4 + speed; `0x14` = point 5, speed 0) reseeds the LFSR every tick = low rumble (thunder). It shares `reCount` with the volume buffer, so use it on CH3 where no notes are played, and call `ATM_NOISE_OFF` afterwards.
* **Cue:** `ATM_CUE(v)` stores one byte; `ATM.check()` returns it and clears it, `ATM.check(id)` tests without clearing. A cue is overwritten by the next one if the sketch does not poll often enough. Put cues on a track that is already part of the beat sequence (2 bytes per marker).
* **Tempo changes** are global (any channel); put them on CH3 so the tick domain stays easy to read. Ticks are all that other channels count, so a tempo change never breaks the alignment.

---

## 5. Timing, tempo and pitch

### 5.1 Tempo

* Default tempo (no `SET_TEMPO`) is 25. `cia = 15625 / tempo`.
* **Empirical rule (user's hardware):** ticks per second ≈ the tempo value. An offline emulator that gives 2 ticks per second per tempo unit plays twice as fast as the hardware. Hence the house rule: **double the emulator's tempo value**.
* With 16 ticks per beat: **BPM = tempo x 3.75.** Examples: 24 -> 90, 30 -> 112.5, 32 -> 120, **34 -> 127.5**, 36 -> 135, 40 -> 150.
* Song length in seconds ≈ ticks / tempo. Quest Theme: 1024 ticks / 34 ≈ 30 s.

### 5.2 Pitch

* The note names are not concert pitch. Working assumption used in all approved songs (stated in the Italo Night header): the table frequency is halved by the 31250 Hz mixing, so a note sounds about **an octave above its name and about 80 cents flat** (`ATM_NOTE_A2` ≈ 210 Hz).
* What matters is relative pitch and register. Registers (by label) that worked: lead A4..D6, chord arps G3..E4, bass A2..A3 (plus transposition), drums on the noise channel.

---

## 6. Seamless loops (the most important section)

Let the loop be `L` ticks long (Quest Theme: 16 bars x 64 = 1024).

**The rule:** every channel must contain **exactly `L-1` ticks of delay**, and all channels must hit `STOP` on the same tick. ATMlib spends the last tick restarting, so the period is exactly `L`.

* Stop tick = sum of delays (here 1023). Restart happens that tick; the first notes of the next loop play on tick 1024.
* A single missing or extra tick in any channel is audible as a hiccup or a drifting channel.

**How to get the missing tick without duplicating whole tracks**

1. Shorten **only the last delay of the last pass**, e.g. the last note of the last bar becomes `ATM_DELAY(15)` instead of 16.
2. That delay must not sit inside a track that is also used for the earlier passes. Two ways:
   * **Caller supplies the final delay (cheapest).** The shared track ends on its last `NOTE` with no delay, and every caller writes the delay after the `GOTO`: `ATM_GOTO(riff), ATM_DELAY(16)` normally and `ATM_GOTO(riff), ATM_DELAY(15)` on the very last call. (Quest Theme lead and drums.)
   * **A `_last` variant track** of the final bar (Italo Night: `bass_bar_last`, `drum_bar_fill_last`). Costs the variant's bytes plus 2 for the table.
3. When a track is called through `REPEAT`, the last repetition cannot be trimmed. Call the shared track `n-1` times with `REPEAT` and handle the final one separately.
4. Reset persistent state at the start of the main track (`ATM_SET_TRA(0)`), and make sure arp/volume state at the loop end equals the state at the loop start.

**Proof, not belief:** simulate three loops. Stops must occur at ticks `L-1, 2L-1, 3L-1` on all four channels, and the note events of loop 1, 2 and 3 must be identical (Appendix B does this).

**One-shot songs:** no `GOTO_ADV`; channels may end at different times; the song ends when the last one stops. For fades use `ATM_VOL` steps per section or `ATM_SL_VOL`.

---

## 7. Fitting a byte budget

Costs: note 1, delay 1, `VOL/SL_VOL/SET_TRA/ADD_TRA/SET_TEMPO` 2, `ARP` 3, `GOTO` 2, `REPEAT` 3, `GOTO_ADV` 5, `RETURN/STOP` 1, plus 2 per track in the table and 5 for the header.

What saves bytes (in the order that mattered):

1. **Transposition instead of copies.** One 2-bar riff (30 B) plays over every chord with `SET_TRA`/`ADD_TRA` (2 B per use). The bass bar is written once in the key of A and transposed the same way.
2. **`REPEAT`** for identical bars: 3 bytes for any number of repeats.
3. **Shared tracks** pay off when a pattern of length `L >= 5` is used at least twice.
4. **Relative transposition cycles**: a track of `[riff][ADD_TRA(5) riff][ADD_TRA(-5) riff]` returns to its start key, so the whole cycle can be re-played in another key by one `SET_TRA`.
5. **Inline tiny things** (a 4-byte track costs 6 with its table entry).
6. **Drums**: one 24-tick core shared by every half-bar, called with `REPEAT(30, ...)`.
7. Chords as explicit long notes with an arp: `NOTE, DELAY(64), NOTE, DELAY(64)` is 4 bytes per two bars.

Quest Theme budget (247 bytes): header 31 B, drums 43, lead 67, chords 40, bass 66.

---

## 8. Composition recipes that worked

### 8.1 Drums (CH3 NOISE), copied from Italo Night

16-tick beats, all with `VOL` + `SL_VOL` + `DELAY`:

| Sound | Commands |
|---|---|
| kick click | `VOL(48) SL_VOL(-12) DELAY(4)` (Italo: `VOL(28) SL_VOL(-14)`) |
| closed hat | `VOL(14) SL_VOL(-7) DELAY(4)` (Italo: `VOL(12) SL_VOL(-6)`) |
| open hat | `VOL(26) SL_VOL(-3) DELAY(8)` |
| snare / clap | `VOL(44) SL_VOL(-5) DELAY(8)` (Italo: `VOL(40)`) |
| snare roll (8 steps of 2 ticks) | `VOL(22..50 in steps of 4) SL_VOL(-9) DELAY(2)` |

Beat A (beats 1, 3) = kick(4) + closed hat(4) + open hat(8). Beat B (beats 2, 4) = snare(8) + open hat(8). Fills: replace beat 4 of bar 4 and bar 8 with the roll.

### 8.2 Kick on a tonal channel (Italo Night CH1)

`ATM_ARP_OFF, ATM_SL_FRQ((uint8_t)-127), ATM_VOL(30), ATM_NOTE_D3_, ATM_DELAY(4), ATM_VOL(0), ATM_DELAY(4)`. Needs `ARP_OFF` first. Use when a channel is free of arpeggios.

### 8.3 Bass (CH2 SAW)

`VOL(63), SL_VOL(-5)`; one beat track, `bar = REPEAT(2, beat) + beat4`, with a variation on beat 4 (root, root, octave); the whole bar is written once in the key of A and transposed per chord.
* Italo Night beat (16 ticks): `VOL(0) DELAY(4)` (duck under the kick), `VOL(63)`, root(4), octave(4), root(4).
* Quest Theme beat (16 ticks): root(8), octave(8); beat 4: root(8), root(4), octave(4).

### 8.4 Lead (CH0 PULSE)

`VOL(38), SL_VOL(-2)`: every note starts loud and decays. Write a 1- or 2-bar riff and transpose it per chord.
**Which riffs survive transposition:** use the *pentatonic* notes of the chord.
* Minor key, chords i, iv, v (root offsets 0, +5, +7): minor pentatonic degrees 0, 3, 5, 7, 10 stay inside the key on all three chords. Avoid the 2nd degree on the v chord.
* Major key, chords I, IV, V: major pentatonic degrees 0, 2, 4, 7, 9.
* A riff containing the third only transposes cleanly between chords of the same quality. Otherwise leave the third out or write explicit notes.

### 8.5 Arpeggio chords (CH1 SQUARE)

* Stabs (Italo Night): `SL_VOL(-3), VOL(24), ARP(0x34, 0x20), NOTE, DELAY(8)` once per beat, after the kick.
* Held chords (Quest Theme): `VOL(30), SL_VOL(-1), ARP(0x34, 0x20)`, then `NOTE root, DELAY(64)` once per bar; the arpeggio shimmers and fades, then re-articulates on the bar.
* Root notes explicit (no transposition on this channel, see 4.6).

### 8.6 Form

* 8 bars (Italo Night, Escaper Droid): one pass of the chord progression, ~15 s at tempo 17/34.
* 16 bars (Quest Theme): the same 8-bar cycle twice, second time lifted a whole step with one `SET_TRA(2)`, ending on a pivot chord (Em: chord iv of Bm and chord v of Am) so the loop returns smoothly.
* Chord progressions used: Am | F | C | G | Am | F | G | E (Italo Night; bass transposition offsets from A: 0, -4, +3, -2, 0, -4, -2, -5), D | G | A | D | D | Bm | A | D (Escaper Droid; offsets from D: 0, +5, +7, 0, 0, +9, +7, 0), Am Am Dm Dm Am Am Em Em then +2 (Quest Theme).

---

## 9. Tooling

Appendix A is `atmsim.py`: a Python model of the playroutine's control flow (call stack, track-0 behaviour, delays, REPEAT counters, transposition, volume retrigger, arp setup, GOTO_ADV restart rule). It does **not** render audio. Since Cutlass Cove it also models the per-tick state: volume/frequency slides (with the missing upper clamp), tremolo, vibrato, glissando, arpeggio and the note-cut gate (and raises if cut is used with transposition), tempo commands, cues. After `simulate()`, `atmsim.TIMELINE[tick]` holds `(volume, frequency_hz)` for each of the four channels, which is how fades, the cut gate and slides are checked. It still raises on `ATM_WAVEFORM` and the long delay.

Appendix B is `gen_quest.py`: the full generator of Quest Theme. It contains the pieces to reuse: command constructors, named tracks, assembler (offsets, track numbers), verifier, header-file writer.

**Verification checklist (all must pass)**

- [ ] `sizeof(song)` within the byte budget
- [ ] all four STOPs on tick `L-1` (and `2L-1`, `3L-1`)
- [ ] restarts exactly `L` ticks apart
- [ ] loops 1, 2, 3 have identical note events
- [ ] max call depth <= 7
- [ ] no track calls track 0
- [ ] every `note + transposition` is within 1..63
- [ ] compiled header bytes == generator bytes (Appendix C)
- [ ] tempo is the doubled value

---

## 10. Output file template

```cpp
#ifndef QUEST_H
#define QUEST_H

// ---------------------------------------------------------------------------
//  "Title" - style, key, bars, bytes
//  Play with:   #include "quest.h"      ATM.play(questTheme);
//  Timing : ATM_SET_TEMPO(34) (doubled). 1 beat = 16 ticks, 1 bar = 64 ticks ...
//  Form / channels / notes about loop compensation
// ---------------------------------------------------------------------------

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif

Song questTheme[] = {     // total song bytes = 247
  0x0D,                       // Number of tracks
  0x00, 0x00,                 // Address of track 0   0   drums
  ...                         // one line per track
  0x03,                       // CH0 entry -> track 3 (lead)
  0x06,                       // CH1 entry
  0x07,                       // CH2 entry
  0x00,                       // CH3 entry
  //"Track 0" drums  [14b]
  ATM_SET_TEMPO(34),
  ATM_GOTO_ADV(3, 6, 7, 0),   // loop point per channel
  ...
};

#endif
```

The generator in Appendix B writes exactly this.

---

## 11. Using a song in a sketch

```cpp
#include <Arduboy2.h>
#include <ATMlib.h>
#include "quest.h"

Arduboy2Base arduboy;
ATMsynth ATM;

void setup() {
  arduboy.begin();
  arduboy.audio.on();          // make sure sound is not muted from a previous sketch
  ATM.play(questTheme);
}
void loop() { /* game */ }
```

Useful calls: `ATM.playPause()`, `ATM.pause()`, `ATM.resume()`, `ATM.stop()`, `ATM.muteChannel(ch)` / `unMuteChannel(ch)` (free a channel for sound effects), `ATM.playSfx(track, ch)`, `ATM.setTempo(t)`, `ATM.check()` (reads the last `ATM_CUE` byte).
Compile-time options (waveforms, effects, octaves) live in `ATMconfig.h`; copy `docs/ATMconfig.h` into the sketch to change them.

---

## 12. Mistakes made on this project (do not repeat)

1. **Invented opcodes.** A first Pixel Chase draft used guessed byte values (`0xC0` for volume ...). They collide with the delay range `0xA0..0xDF`. Always take values from `ATMcmds.h`.
2. **Track 0 called** (section 4.3). Looks fine in a tick counter, sounds like a stuck melody on hardware. A simulator that models the call stack catches it.
3. **`REPEAT(n)` is n+1 runs.** `REPEAT(7, beat)` made a 128-tick bar instead of 64.
4. **`GOTO_ADV` in a one-shot song** makes it restart.
5. **Off-by-one loops:** summing to `L` instead of `L-1`, or one channel off by a tick.
6. **Tempo not doubled.**
7. **Invalid names/arguments:** `ATM_NOTE_F#5`, `ATM_NOTE_OFF`, negative arguments without `(uint8_t)`.
8. **Plain, short, hex-looking songs** are not accepted. Use the Italo Night structure: real melody, kick/hat/snare with fills, transposed bass, chord stabs, a form with a destination.
9. **Combining `ATM_ARP` with transposition on one channel** (detunes the arpeggio).
10. **Trusting a counter instead of a model.** Verify with the simulator and by compiling.

---

## 13. Catalog of the existing songs

| File | Array | Style | Size | Notes |
|---|---|---|---|---|
| `newsong.h` | `italoNight` | Italo disco, A minor, 8 bars | 558 B | Original, written at emulator tempo 17 (use 34). Kick + arp stabs on CH1, transposed SAW bass, noise drums with fills. Chords Am F C G Am F G E. |
| `newsong2.h` | `escaperDroid` | uptempo space, D major, 8 bars | 546 B | Same skeleton as Italo Night. ARP `0x47` / `0x37`. `ATM_CUE(1)` once per beat (inside the kick track, 2 bytes) for the game's dancing droid sprite. |
| `newsong3.h` | `neonHeart` | Italo disco, E minor, 8 bars | 565 B | Tempo 36. Italo Night skeleton with new key, melody and chords: Em C G D Em C D B (dominant pulls back to Em). Chord stabs use inversions (Em, C/E, G/D, D/F#, B/D#). Generator `gen_neon.py` (reuses the Appendix D helpers + `ATM_GOTO_ADV`). |
| `synth.h` | `midnightRun` | 80s synthwave, A minor, 96 bars (~3 min) | 715 B | One-shot, fade in and out, no `GOTO_ADV`. |
| `quest.h` | `questTheme` | in-game chiptune, A minor, 16 bars | 245 B | **No tempo command** (the sketch sets it; intended value 34). Arpeggio chords, transposed lead and bass, key lift and Em pivot, gapless loop. Best reference for tight budgets. |
| `gameover.h` | `gameOver` | one-shot game over jingle, A minor, 5 bars | 106 B | Tempo 20. Andalusian descent Am-G-F-E-Am, one lead phrase transposed down with ADD_TRA, arp chords, bass on the same transposition, noise toll per bar. No GOTO_ADV, so it ends by itself. Reference for small one-shots (the user's own `youDied` does the same in 64 B). |
| `cutlass.h` | `cutlassCove` | original tropical-pirate showcase, C major / A minor, 32 bars (~75 s) | 839 B | One-shot, tempo 30 (+6 in B, ritardando in the outro), fades, cues 1..6. Uses nearly every command: Appendix D. **Not** a copy of any existing song (the Monkey Island theme is a copyrighted composition and is not reproduced). |
| `pixelchase.h` | `pixelChase` | NES-like | 181 B | **Broken (track-0 bug), rejected. Do not use.** |

---

## Appendix A: `atmsim.py` (playroutine simulator: control flow and per-tick state)

```python
#!/usr/bin/env python3
"""
Simulator of ATMlib's playroutine (ATMlibimpl.h) - control flow AND per-tick channel state.

Models exactly (see ATM_playroutine in ATMlibimpl.h):
  * per-channel call stack (7 deep) incl. the quirk that a channel's "current track" starts at 0
  * REPEAT counters, delays, GOTO_ADV loop points, the all-channels-stopped restart rule
    (checked after each channel, like the C code), tempo commands
  * per-tick effect order: noise retrigger, glissando, volume/frequency slide, arpeggio/note-cut,
    tremolo/vibrato, then the command stream
  * the real quirks: volume is NOT clamped above 63 by slides or tremolo (only freq is clamped),
    vol/freq values are bytes / int16 as in the C structs, volFreConfig != 0 disables the
    "reset volume at note-on" rule, ATM_CUT behaves as a repeating gate (on T+1 ticks, off T+1 ticks)
    and gives note `transposition` instead of silence when transposed.
After simulate():   atmsim.TIMELINE[tick] = [(vol, freq_hz) x4]  (state after the tick was processed)
"""
NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
TIMELINE = []


def note_name(n):
    if n <= 0:
        return '--'
    n0 = n - 1
    return f"{NOTE_NAMES[n0 % 12]}{2 + n0 // 12}"


def freq_hz(note):
    """frequency table of the library docs (nr 10 = 440 Hz); 0 = off"""
    if note <= 0 or note > 63:
        return 0
    return int(round(440 * 2 ** ((note - 10) / 12)))


def s8(v):
    return v - 256 if v > 127 else v


class Ch:
    def __init__(self):
        self.ptr = 0
        self.note = 0
        self.stack = []          # (counter, track, ptr)
        self.repeatPoint = 0
        self.delay = 0
        self.counter = 0
        self.track = 0
        self.vol = 0
        self.freq = 0
        self.reCount = 0
        self.reConfig = 0
        self.slide = 0           # volFreSlide (signed char)
        self.slideCfg = 0        # volFreConfig
        self.slideCount = 0
        self.arpNotes = 0
        self.arpTiming = 0
        self.arpCount = 0
        self.arp = None          # kept for the event log (notes, timing)
        self.tra = 0
        self.treD = 0
        self.treC = 0
        self.treN = 0
        self.glis = 0
        self.glisCount = 0
        self.stopped = False


def parse(song):
    ntr = song[0]
    offs = [song[1 + 2 * i] | (song[2 + 2 * i] << 8) for i in range(ntr)]
    pos = 1 + 2 * ntr
    entries = list(song[pos:pos + 4])
    base = pos + 4
    return ntr, offs, entries, base


ARGS = {0: 1, 1: 1, 2: 2, 3: 0, 4: 1, 5: 2, 6: 0, 7: 2, 8: 0, 9: 1, 10: 0, 11: 1, 12: 1, 13: 0,
        14: 2, 15: 0, 16: 2, 17: 0, 18: 1, 19: 0, 20: 1, 21: 0, 22: 1, 23: 1}


def simulate(song, max_ticks, trace=True):
    global TIMELINE
    TIMELINE = []
    ntr, offs, entries, base = parse(song)
    ch = [Ch() for _ in range(4)]
    for n in range(4):
        ch[n].ptr = base + offs[entries[n]]
    events = []        # (tick, chan, 'note', note+tra, vol, arp, tra) | (tick, chan, 'cue', value)
    restarts, stops = [], []
    maxstack = 0
    tempo = 25
    tempo_log = []
    tick = 0
    warn = []
    while tick < max_ticks:
        for n in range(4):
            c = ch[n]
            # ---- noise retrigger (shares reCount with the volume buffer)
            if c.reConfig:
                if c.reCount >= (c.reConfig & 3):
                    c.reCount = 0
                else:
                    c.reCount += 1
            # ---- glissando
            if c.glis:
                cfg = c.glis & 0xFF
                if c.glisCount >= (cfg & 0x7F):
                    c.note += -1 if cfg & 0x80 else 1
                    c.note = max(1, min(63, c.note))
                    c.freq = freq_hz(c.note)
                    c.glisCount = 0
                else:
                    c.glisCount += 1
            # ---- volume / frequency slide
            if c.slide:
                if not c.slideCount:
                    isf = bool(c.slideCfg & 0x40)
                    vf = (c.freq if isf else c.vol) + c.slide
                    if not (c.slideCfg & 0x80):
                        if vf < 0:
                            vf = 0
                        elif isf and vf > 9397:
                            vf = 9397
                    if isf:
                        c.freq = vf
                    else:
                        c.vol = vf & 0xFF
                if c.slideCount >= (c.slideCfg & 0x3F):
                    c.slideCount = 0
                else:
                    c.slideCount += 1
            # ---- arpeggio / note cut
            if c.arpNotes and c.note:
                if (c.arpCount & 0x1F) < (c.arpTiming & 0x1F):
                    c.arpCount += 1
                else:
                    if (c.arpCount & 0xE0) == 0x00:
                        c.arpCount = 0x20
                    elif (c.arpCount & 0xE0) == 0x20 and not (c.arpTiming & 0x40) and c.arpNotes != 0xFF:
                        c.arpCount = 0x40
                    else:
                        c.arpCount = 0x00
                    an = c.note
                    if (c.arpCount & 0xE0) != 0x00:
                        an = 0 if c.arpNotes == 0xFF else an + (c.arpNotes >> 4)
                    if (c.arpCount & 0xE0) == 0x40:
                        an += (c.arpNotes & 0x0F)
                    if c.arpNotes == 0xFF and an == 0 and c.tra != 0:
                        raise Exception('ATM_CUT with transposition != 0 plays note %d instead of silence' % c.tra)
                    c.freq = freq_hz(an + c.tra)
            # ---- tremolo / vibrato
            if c.treD:
                isf = bool(c.treC & 0x40)
                vt = c.freq if isf else c.vol
                vt = vt + c.treD if (c.treN & 0x80) else vt - c.treD
                if vt < 0:
                    vt = 0
                elif isf and vt > 9397:
                    vt = 9397
                if isf:
                    c.freq = vt
                else:
                    c.vol = vt & 0xFF
                if (c.treN & 0x1F) < (c.treC & 0x1F):
                    c.treN += 1
                else:
                    c.treN = 0 if (c.treN & 0x80) else 0x80
            # ---- commands
            if c.delay:
                if c.delay != 0xFFFF:
                    c.delay -= 1
            else:
                while True:
                    cmd = song[c.ptr]; c.ptr += 1
                    if cmd < 64:
                        c.note = cmd
                        if cmd:
                            c.note = (cmd + c.tra) & 0xFF
                        c.freq = freq_hz(c.note)
                        if c.slideCfg == 0:
                            c.vol = c.reCount
                        if c.arpTiming & 0x20:
                            c.arpCount = 0
                        events.append((tick, n, 'note', c.note, c.vol, c.arp, c.tra))
                    elif cmd < 160:
                        fx = cmd - 64
                        if fx == 0:
                            c.vol = song[c.ptr]; c.ptr += 1
                            c.reCount = c.vol
                        elif fx in (1, 4):
                            c.slide = s8(song[c.ptr]); c.ptr += 1
                            c.slideCfg = 0 if fx == 1 else 0x40
                        elif fx in (2, 5):
                            c.slide = s8(song[c.ptr]); c.slideCfg = song[c.ptr + 1]; c.ptr += 2
                            if fx == 5:
                                c.slideCfg |= 0x40
                        elif fx in (3, 6):
                            c.slide = 0
                        elif fx == 7:
                            c.arpNotes = song[c.ptr]; c.arpTiming = song[c.ptr + 1]
                            c.arp = (c.arpNotes, c.arpTiming); c.ptr += 2
                        elif fx in (8, 21):
                            c.arpNotes = 0; c.arp = None
                        elif fx == 9:
                            c.reConfig = song[c.ptr]; c.ptr += 1
                        elif fx == 10:
                            c.reConfig = 0
                        elif fx == 11:
                            c.tra = s8((c.tra + s8(song[c.ptr])) & 0xFF); c.ptr += 1
                        elif fx == 12:
                            c.tra = s8(song[c.ptr]); c.ptr += 1
                        elif fx == 13:
                            c.tra = 0
                        elif fx in (14, 16):
                            c.treD = song[c.ptr]; c.treC = song[c.ptr + 1] + (0 if fx == 14 else 0x40)
                            c.ptr += 2
                        elif fx in (15, 17):
                            c.treD = 0
                        elif fx == 18:
                            c.glis = song[c.ptr]; c.ptr += 1
                        elif fx == 19:
                            c.glis = 0
                        elif fx == 20:
                            c.arpNotes = 0xFF; c.arpTiming = song[c.ptr]; c.ptr += 1
                            c.arp = ('cut', c.arpTiming)
                        elif fx == 22:
                            raise Exception("ATM_WAVEFORM is unhandled by the playroutine - do not use")
                        elif fx == 23:
                            events.append((tick, n, 'cue', song[c.ptr])); c.ptr += 1
                        elif cmd == 156:           # ADD tempo (92+64)
                            tempo = (tempo + song[c.ptr]) & 0xFF; c.ptr += 1
                            tempo_log.append((tick, tempo))
                        elif cmd == 157:           # SET tempo (93+64)
                            tempo = song[c.ptr]; c.ptr += 1
                            tempo_log.append((tick, tempo))
                        elif cmd == 158:           # GOTO_ADV
                            for i in range(4):
                                ch[i].repeatPoint = song[c.ptr]; c.ptr += 1
                        elif cmd == 159:           # STOP
                            c.vol = 0
                            c.delay = 0xFFFF
                            c.stopped = True
                            stops.append((tick, n))
                        else:
                            raise Exception(f"unmodelled FX {cmd:#x} at {c.ptr-1}")
                    elif cmd < 224:
                        c.delay = cmd - 159
                    elif cmd in (252, 253):
                        newc = 0 if cmd == 252 else song[c.ptr]
                        if cmd == 253:
                            c.ptr += 1
                        newt = song[c.ptr]; c.ptr += 1
                        if newt != c.track:
                            c.stack.append((c.counter, c.track, c.ptr - base))
                            if len(c.stack) > 7:
                                raise Exception("call stack overflow (>7)")
                            maxstack = max(maxstack, len(c.stack))
                            c.track = newt
                        c.counter = newc
                        c.ptr = base + offs[c.track]
                    elif cmd == 254:
                        if c.counter > 0 or len(c.stack) == 0:
                            if c.counter:
                                c.counter -= 1
                            c.ptr = base + offs[c.track]
                        else:
                            cnt, trk, p = c.stack.pop()
                            c.ptr = p + base
                            c.counter = cnt
                            c.track = trk
                    else:
                        raise Exception(f"bad cmd {cmd} at {c.ptr-1}")
                    if c.delay != 0:
                        break
                if c.delay != 0xFFFF:
                    c.delay -= 1
            # restart check (inside the channel loop, like the C code)
            if all(x.stopped for x in ch):
                if sum(x.repeatPoint for x in ch):
                    restarts.append((tick, n))
                    for k in range(4):
                        ch[k].ptr = base + offs[ch[k].repeatPoint]
                        ch[k].delay = 0
                        ch[k].stopped = False
                else:
                    TIMELINE.append([(x.vol, x.freq) for x in ch])
                    simulate.tempo_log = tempo_log
                    return events, restarts, stops, maxstack, tempo
        TIMELINE.append([(x.vol, x.freq) for x in ch])
        tick += 1
    simulate.tempo_log = tempo_log
    return events, restarts, stops, maxstack, tempo
```

## Appendix B: `gen_quest.py` (generator, assembler, verifier, .h writer)

Run it in the same folder as `atmsim.py`; it writes `quest.h` and `quest.bin` there.

```python
#!/usr/bin/env python3
"""
"Quest Theme" - chiptune in-game loop for ATMlib, target <= 250 bytes total.

16 bars x 64 ticks = 1024 ticks, A minor, second half lifted a whole step (Bm), pivoting back through Em.
  CH0 PULSE  lead   : one 2-bar riff, TRANSPOSED per chord (SET_TRA / ADD_TRA)
  CH1 SQUARE chords : ARPEGGIO minor triads (explicit roots - arp + transpose on the
                      same channel would transpose twice, see ATMlibimpl.h line ~457)
  CH2 SAW    bass   : root/octave pattern, TRANSPOSED per chord
  CH3 NOISE  drums  : tempo + loop points + kick/hat/snare
Track 0 is NEVER called (ATMlib starts every channel with 'current track' = 0, calling
track 0 breaks the call stack), it is only CH3's entry point.
"""
import sys
sys.path.insert(0, '.')
from atmsim import simulate, note_name, parse

SONG_VAR = 'questTheme'
TEMPO = 34            # doubled, as always

# ---------------------------------------------------------------- note helpers
_NAMES = ['C', 'C_', 'D', 'D_', 'E', 'F', 'F_', 'G', 'G_', 'A', 'A_', 'B']
_SHARP = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8,
          'A': 9, 'A#': 10, 'B': 11}


def nnum(name):
    if name[1] == '#':
        pc, octv = name[:2], int(name[2:])
    else:
        pc, octv = name[0], int(name[1:])
    return (octv - 2) * 12 + _SHARP[pc] + 1


def nmacro(n):
    return 'ATM_NOTE_' + _NAMES[(n - 1) % 12] + str(2 + (n - 1) // 12)


def NOTE(name):
    n = nnum(name)
    # sharps are written with the underscore suffix: ATM_NOTE_F5_
    o = 2 + (n - 1) // 12
    pc = _NAMES[(n - 1) % 12]
    return (f'ATM_NOTE_{pc[0]}{o}{"_" if len(pc) > 1 else ""}', [n])


def DELAY(d):
    assert 1 <= d <= 64, d
    return (f'ATM_DELAY({d})', [0x9F + d])


def _s(v):
    return f'(uint8_t){v}' if v < 0 else str(v)


def VOL(v):       return (f'ATM_VOL({v})', [0x40, v])
def SLV(v):       return (f'ATM_SL_VOL({_s(v)})', [0x41, v & 0xFF])
def ARP(a, t):    return (f'ATM_ARP(0x{a:02X}, 0x{t:02X})', [0x47, a, t])
def ADDTRA(v):    return (f'ATM_ADD_TRA({_s(v)})', [0x4B, v & 0xFF])
def SETTRA(v):    return (f'ATM_SET_TRA({_s(v)})', [0x4C, v & 0xFF])
def TEMPOc(v):    return (f'ATM_SET_TEMPO({v})', [0x9D, v])
def STOP():       return ('ATM_STOP_CHAN', [0x9F])
def RET():        return ('ATM_RETURN', [0xFE])
def GOTO(t):      return ('GOTO', t)
def REPEAT(r, t): return ('REPEAT', r, t)
def ADV(a, b, c, d): return ('ADV', a, b, c, d)


# ---------------------------------------------------------------- the song
TR = {}          # name -> list of commands
ORDER = []


def track(name, cmds):
    TR[name] = cmds
    ORDER.append(name)


def n_(name, d):
    return [NOTE(name), DELAY(d)]


# 0: drums (CH3 entry, repeat point 0). tempo + loop points + 31 halves + trimmed tail
track('drums', [
    TEMPOc(TEMPO),
    ADV('lead', 'chords', 'bass', 'drums'),
    REPEAT(30, 'drum_half'),             # 31 x 32t = 992t
    GOTO('drum_core'), DELAY(7),         # 24t + 7t = 31t  -> 1023t total
    STOP(),
])
# kick, closed hat, open hat, snare, open hat (its delay is added by the caller)
track('drum_core', [
    VOL(48), SLV(-12), DELAY(4),         # kick click
    VOL(14), SLV(-7), DELAY(4),          # closed hat
    VOL(26), SLV(-3), DELAY(8),          # open hat
    VOL(44), SLV(-5), DELAY(8),          # snare
    VOL(26), SLV(-3),                    # open hat, delay supplied by caller
    RET(),
])
track('drum_half', [GOTO('drum_core'), DELAY(8), RET()])

# 3: lead
track('lead', [
    VOL(38), SLV(-2),
    SETTRA(0), GOTO('lead_cycle'),       # bars 1-6   Am Dm Am
    ADDTRA(7), GOTO('lead_riff'), DELAY(16),   # bars 7-8   Em
    SETTRA(2), GOTO('lead_cycle'),       # bars 9-14  Bm Em Bm   (lifted a whole step)
    ADDTRA(5), GOTO('lead_riff'), DELAY(15),   # bars 15-16 Em (pivot back to Am), last tick trimmed
    STOP(),
])
track('lead_riff', (
    n_('A4', 8) + n_('C5', 8) + n_('E5', 8) + n_('C5', 8) +
    n_('D5', 12) + n_('C5', 4) + n_('A4', 8) + n_('C5', 8) +
    n_('E5', 8) + n_('D5', 8) + n_('C5', 8) + n_('A4', 8) +
    n_('C5', 8) + n_('G4', 8) + [NOTE('A4')] +      # last delay supplied by caller
    [RET()]
))
track('lead_cycle', [
    GOTO('lead_riff'), DELAY(16),
    ADDTRA(5), GOTO('lead_riff'), DELAY(16),
    ADDTRA(-5), GOTO('lead_riff'), DELAY(16),
    RET(),
])

# 6: chords (explicit roots, minor arpeggio)
ROOTS = ['A3', 'D4', 'A3', 'E4',   'B3', 'E4', 'B3', 'E4']
ch = [VOL(30), SLV(-1), ARP(0x34, 0x20)]
for i, r in enumerate(ROOTS):
    last = (i == len(ROOTS) - 1)
    ch += [NOTE(r), DELAY(64), NOTE(r), DELAY(63 if last else 64)]
ch += [STOP()]
track('chords', ch)

# 7: bass
track('bass', [
    VOL(63), SLV(-5),
    SETTRA(0), GOTO('bass_cycle'),                               # bars 1-6   Am Dm Am
    ADDTRA(7), REPEAT(1, 'bass_bar'),                            # bars 7-8   Em
    SETTRA(2), GOTO('bass_cycle'),                               # bars 9-14  Bm Em Bm (lifted)
    SETTRA(7), GOTO('bass_bar'), GOTO('bass_bar_last'),          # bars 15-16 Em, last tick trimmed
    STOP(),
])
track('bass_cycle', [
    REPEAT(1, 'bass_bar'),
    ADDTRA(5), REPEAT(1, 'bass_bar'),
    ADDTRA(-5), REPEAT(1, 'bass_bar'),
    RET(),
])
track('bass_bar', [REPEAT(2, 'bass_beat'), GOTO('bass_beat4'), RET()])
track('bass_bar_last', (
    [REPEAT(2, 'bass_beat')] +
    n_('A2', 8) + n_('A2', 4) + n_('A3', 3) +        # last note 1 tick short
    [RET()]
))
track('bass_beat', n_('A2', 8) + n_('A3', 8) + [RET()])
track('bass_beat4', n_('A2', 8) + n_('A2', 4) + n_('A3', 4) + [RET()])

CH_ENTRY = ['lead', 'chords', 'bass', 'drums']


# ---------------------------------------------------------------- assemble
def assemble():
    idx = {name: i for i, name in enumerate(ORDER)}
    blobs = []
    for name in ORDER:
        b = []
        for c in TR[name]:
            if c[0] == 'GOTO':
                b += [0xFC, idx[c[1]]]
            elif c[0] == 'REPEAT':
                b += [0xFD, c[1], idx[c[2]]]
            elif c[0] == 'ADV':
                b += [0x9E] + [idx[x] for x in c[1:]]
            else:
                b += c[1]
        blobs.append(b)
    offs, pos = [], 0
    for b in blobs:
        offs.append(pos)
        pos += len(b)
    n = len(ORDER)
    song = [n]
    for o in offs:
        song += [o & 0xFF, o >> 8]
    song += [idx[e] for e in CH_ENTRY]
    for b in blobs:
        song += b
    return song, idx, offs, blobs


def render_text(idx, offs, blobs, total):
    L = []
    L += [
        '#ifndef QUEST_H', '#define QUEST_H', '',
        '// ---------------------------------------------------------------------------',
        f'//  "Quest Theme" - chiptune in-game loop, A minor, 16 bars, {total} bytes',
        '//',
        '//  Play with:   #include "quest.h"      ATM.play(questTheme);',
        '//',
        f'//  Timing : ATM_SET_TEMPO({TEMPO}) (doubled).  1 beat = 16 ticks, 1 bar = 64 ticks,',
        '//           16 bars = 1024 ticks. Every channel sums to 1023 ticks of delay + the 1 tick',
        '//           ATMlib spends restarting, so the loop is gapless.',
        '//  Form   : bars 1-8   Am | Am | Dm | Dm | Am | Am | Em | Em',
        '//           bars 9-16  the cycle lifted a whole step: Bm | Bm | Em | Em | Bm | Bm | Em | Em',
        '//                      (Em is chord iv in Bm and chord v in Am: it pivots back into bar 1)',
        '//  Channels (ATMlib defaults):',
        '//     CH0 PULSE  - lead: ONE 2-bar riff, transposed per chord (SET_TRA / ADD_TRA)',
        '//     CH1 SQUARE - minor-triad ARPEGGIO chords (explicit roots, no transpose on this channel)',
        '//     CH2 SAW    - root/octave bass, transposed per chord, beat-4 variation',
        '//     CH3 NOISE  - tempo + loop points + kick click, hats, snare',
        '//  Note   : track 0 is only CH3\'s entry point and is never called. ATMlib starts every',
        '//           channel with "current track = 0", so calling track 0 would break the call stack.',
        '// ---------------------------------------------------------------------------', '',
        '#include <ATMcmds.h>', '',
        '#ifndef Song', '#define Song const uint8_t PROGMEM', '#endif', '', '',
        f'Song {SONG_VAR}[] = {{     // total song bytes = {total}',
        f'  0x{len(ORDER):02X},                       // Number of tracks',
    ]
    for i, name in enumerate(ORDER):
        o = offs[i]
        L.append(f'  0x{o & 0xFF:02X}, 0x{o >> 8:02X},                 // Address of track {i:<2d} {o:5d}   {name}')
    L.append('')
    for k, e in enumerate(CH_ENTRY):
        L.append(f'  0x{idx[e]:02X},                         // CH{k} entry -> track {idx[e]} ({e})')
    L.append('')
    for i, name in enumerate(ORDER):
        L.append(f'  //"Track {i}" {name}  [{len(blobs[i])}b]')
        for c in TR[name]:
            if c[0] == 'GOTO':
                L.append(f'  ATM_GOTO({idx[c[1]]}),   // -> {c[1]}')
            elif c[0] == 'REPEAT':
                L.append(f'  ATM_REPEAT({c[1]}, {idx[c[2]]}),   // {c[1] + 1}x {c[2]}')
            elif c[0] == 'ADV':
                L.append(f'  ATM_GOTO_ADV({", ".join(str(idx[x]) for x in c[1:])}),   // loop point per channel')
            else:
                L.append(f'  {c[0]},')
        L.append('')
    L += ['};', '', '#endif', '']
    return '\n'.join(L)


if __name__ == '__main__':
    song, idx, offs, blobs = assemble()
    total = len(song)
    print(f'tracks={len(ORDER)} total={total} bytes (limit 250)')
    for i, name in enumerate(ORDER):
        print(f'  T{i:<2d} {name:14s} {len(blobs[i]):3d}b')

    # ---- verification with the control-flow simulator
    ev, restarts, stops, maxstack, tempo = simulate(song, 1024 * 3 + 40)
    print('tempo set:', tempo, '| max call depth:', maxstack)
    print('stop events (first 8):', stops[:8])
    print('restarts:', restarts)
    ok = True
    st = [t for t, _ in stops[:4]]
    if len(set(st)) != 1 or st[0] != 1023:
        print('FAIL: channels do not all stop on tick 1023'); ok = False
    if [t for t, _ in restarts[:2]] != [1023, 2047]:
        print('FAIL: restart cadence is not 1024 ticks'); ok = False

    def loop_events(k):
        return [(t - 1024 * k,) + e[1:] for e in ev for t in [e[0]] if 1024 * k <= t < 1024 * (k + 1)]
    l0, l1, l2 = loop_events(0), loop_events(1), loop_events(2)
    if not (l0 == l1 == l2):
        print('FAIL: loops differ (state leaking across the restart)'); ok = False
    else:
        print(f'loop 0 == loop 1 == loop 2  ({len(l0)} note events each)')

    # ---- musical summary per channel/bar
    names = ['CH0 lead ', 'CH1 chord', 'CH2 bass ', 'CH3 drum']
    for n in range(3):
        print('\n' + names[n])
        for bar in range(16):
            row = [note_name(e[3]) + f'@{(e[0] % 64):02d}' for e in l0 if e[1] == n and e[0] // 64 == bar]
            print(f'  bar {bar + 1:2d}: ' + ' '.join(row))
    print('\nOK' if ok else '\nPROBLEMS')
    if ok and total <= 250:
        open('./quest.h', 'w').write(render_text(idx, offs, blobs, total))
        open('./quest.bin', 'wb').write(bytes(song))
        print('wrote ./quest.h')
```

## Appendix C: compile the header against the real ATMlib and compare bytes

```cpp
// chk.cpp
#include <stdio.h>
#define PROGMEM
#include "quest.h"
int main(){ fwrite(questTheme, 1, sizeof(questTheme), stdout); return 0; }
```

```bash
g++ -std=c++11 -Wall -Wextra -I/path/to/ATMlib/src chk.cpp -o chk
./chk > from_header.bin
cmp from_header.bin quest.bin && echo IDENTICAL     # quest.bin is written by the generator
```

---

## Appendix D: `gen_cutlass.py` (showcase song: every effect, fades, cues, tempo changes)

Reference for the commands that Quest Theme does not use. Run it in the folder that contains `atmsim.py`; it writes `cutlass.bin` and `cutlass_text.h`. Verify with the checks in section 9 plus: `atmsim.TIMELINE` for the fades, the cut gate, the cannon slide and the vibrato range, and the `simulate.tempo_log` list for the tempo changes.

```python
#!/usr/bin/env python3
"""
"Cutlass Cove" - original tropical-pirate showcase for ATMlib (one-shot, 32 bars, ~73 s).

  INTRO  4 bars  fade-in: sea (tremolo noise), seagull (glissando), pad (arp), bass swell
  A      8 bars  sunny calypso/reggae: steel-drum lead, off-beat skank, bouncy bass, one-drop
  B      8 bars  storm / swashbuckle: tempo up, thunder (noise retrigger), cannon (freq slide),
                 staccato lead (note cut), vibrato lead, driving drums, tremolo snare rolls
  A'     8 bars  return, lifted a whole step (C -> D major), tempo back
  OUTRO  4 bars  ritardando (ADD_TEMPO), fade-out on all channels, final cue

Track 0 is only CH3's entry point and is never called (ATMlib starts every channel at 'track 0').
"""
import sys
sys.path.insert(0, '/home/claude/work')
import atmsim
from atmsim import simulate, note_name

SONG_VAR = 'cutlassCove'
TEMPO = 30            # doubled, as always  (30 -> ~112 BPM in the emulator's scale)

_NAMES = ['C', 'C_', 'D', 'D_', 'E', 'F', 'F_', 'G', 'G_', 'A', 'A_', 'B']
_SHARP = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8,
          'A': 9, 'A#': 10, 'B': 11}


def nnum(name):
    if name[1] == '#':
        pc, octv = name[:2], int(name[2:])
    else:
        pc, octv = name[0], int(name[1:])
    return (octv - 2) * 12 + _SHARP[pc] + 1


def NOTE(name):
    n = nnum(name)
    o = 2 + (n - 1) // 12
    pc = _NAMES[(n - 1) % 12]
    return (f'ATM_NOTE_{pc[0]}{o}{"_" if len(pc) > 1 else ""}', [n])


def DELAY(d):
    assert 1 <= d <= 64, d
    return (f'ATM_DELAY({d})', [0x9F + d])


def _s(v):
    return f'(uint8_t){v}' if v < 0 else str(v)


def VOL(v):         return (f'ATM_VOL({v})', [0x40, v])
def SLV(v):         return (f'ATM_SL_VOL({_s(v)})', [0x41, v & 0xFF])
def SLVADV(a, t):   return (f'ATM_SL_VOL_ADV({_s(a)}, {t})', [0x42, a & 0xFF, t])
SLVOFF = ('ATM_SL_VOL_OFF', [0x43])
def SLFRQ(v):       return (f'ATM_SL_FRQ({_s(v)})', [0x44, v & 0xFF])
def ARP(a, t):      return (f'ATM_ARP(0x{a:02X}, 0x{t:02X})', [0x47, a, t])
def NOISE(v):       return (f'ATM_NOISE(0x{v:02X})', [0x49, v])
NOISEOFF = ('ATM_NOISE_OFF', [0x4A])
def ADDTRA(v):      return (f'ATM_ADD_TRA({_s(v)})', [0x4B, v & 0xFF])
def SETTRA(v):      return (f'ATM_SET_TRA({_s(v)})', [0x4C, v & 0xFF])
def TREM(d, r):     return (f'ATM_TREM({d}, {r})', [0x4E, d, r])
TREMOFF = ('ATM_TREM_OFF', [0x4F])
def VIB(d, r):      return (f'ATM_VIB({d}, {r})', [0x50, d, r])
VIBOFF = ('ATM_VIB_OFF', [0x51])
def GLIS(v):        return (f'ATM_GLIS(0x{v:02X})', [0x52, v])
GLISOFF = ('ATM_GLIS_OFF', [0x53])
def CUT(v):         return (f'ATM_CUT(0x{v:02X})', [0x54, v])
CUTOFF = ('ATM_CUT_OFF', [0x55])
def CUE(v):         return (f'ATM_CUE({v})', [0x57, v])
def ADDTEMPO(v):    return (f'ATM_ADD_TEMPO({_s(v)})', [0x9C, v & 0xFF])
def TEMPOc(v):      return (f'ATM_SET_TEMPO({v})', [0x9D, v])
def STOP():         return ('ATM_STOP_CHAN', [0x9F])
def RET():          return ('ATM_RETURN', [0xFE])
def GOTO(t):        return ('GOTO', t)
def REPEAT(r, t):   return ('REPEAT', r, t)


TR, ORDER = {}, []


def track(name, cmds):
    TR[name] = cmds
    ORDER.append(name)


def n_(name, d):
    return [NOTE(name), DELAY(d)]


# =============================================================================== CH3 drums / sea
# (track 0 = CH3 entry)
track('drums', [
    TEMPOc(TEMPO),
    CUE(1),                                                  # intro
    VOL(4), SLVADV(1, 7), TREM(1, 15),                       # sea: fade-in + slow tremolo swell
    DELAY(64), DELAY(32), SLVOFF,                            # stop the fade at ~tick 96 (slide does NOT clamp at 63!)
    DELAY(32), DELAY(64), DELAY(48),
    GOTO('roll'),                                            # tremolo snare roll into the A section
    CUE(2),                                                  # A
    REPEAT(1, 'dr_a4'),
    CUE(3),                                                  # B
    ADDTEMPO(6),                                             # tempo up
    GOTO('thunder'), GOTO('half_b'),                         # bar 1: thunder + half a bar of drums
    REPEAT(1, 'bar_b'),                                      # bars 2-3
    GOTO('fill_b'),                                          # bar 4
    GOTO('dr_b4'),                                           # bars 5-8
    CUE(4),                                                  # A'
    TEMPOc(TEMPO),
    REPEAT(1, 'dr_a4'),
    CUE(5),                                                  # outro
    GOTO('bar_a'),
    VOL(20), SLVADV(-1, 5), TREM(1, 15),                     # sea again, fading out
    REPEAT(11, 'slow'),                                      # 12 beats, tempo -1 per beat
    CUE(6),                                                  # the end
    STOP(),
])
track('hat',   [VOL(14), SLV(-7), DELAY(8), RET()])
track('ohat',  [VOL(26), SLV(-3), DELAY(8), RET()])
track('thump', [VOL(46), SLV(-6), DELAY(8), RET()])
track('kick',  [VOL(48), SLV(-12), DELAY(8), RET()])
track('snr',   [VOL(44), SLV(-5), DELAY(8), RET()])
# tremolo (depth 6, rate 0 = flips every tick) on a rising volume = snare roll in 8 bytes
track('roll',  [VOL(10), SLV(2), TREM(6, 0), DELAY(16), TREMOFF, RET()])
# noise retrigger: reseeds the LFSR every tick -> low rumble
track('thunder', [NOISE(0x14), VOL(36), SLV(-1), DELAY(32), NOISEOFF, RET()])
track('bar_a',  [REPEAT(3, 'hat'), GOTO('thump'), REPEAT(1, 'hat'), GOTO('ohat'), RET()])
track('fill_a', [REPEAT(3, 'hat'), GOTO('thump'), GOTO('hat'), GOTO('roll'), RET()])
track('dr_a4',  [REPEAT(2, 'bar_a'), GOTO('fill_a'), RET()])
track('half_b', [GOTO('kick'), GOTO('hat'), GOTO('snr'), GOTO('hat'), RET()])
track('bar_b',  [REPEAT(1, 'half_b'), RET()])
track('fill_b', [GOTO('half_b'), GOTO('kick'), GOTO('hat'), GOTO('roll'), RET()])
track('dr_b4',  [REPEAT(2, 'bar_b'), GOTO('fill_b'), RET()])
track('slow',   [ADDTEMPO(-1), DELAY(16), RET()])

# =============================================================================== CH0 lead
track('lead', [
    # ---- INTRO (4 bars): bar 1 silent, bar 2 seagulls, bars 3-4 a pan-flute foreshadowing
    DELAY(64),
    GOTO('gull'),
    VOL(30), SLV(-1), VIB(3, 3),
    SETTRA(-4), GOTO('sing'),                 # F
    ADDTRA(2), GOTO('sing'),                  # G
    VIBOFF,
    # ---- A (8 bars): steel-drum lead, riff transposed per chord
    VOL(40), SLV(-1),
    SETTRA(0), REPEAT(1, 'phr_a'),
    # ---- B (8 bars): 4 bars of staccato pedal riff (note cut), 4 bars vibrato melody
    VOL(36), SLV(-1),
    SETTRA(0), CUT(0x23), REPEAT(3, 'riff_p'), CUTOFF,
    VIB(3, 3),
    SETTRA(5), GOTO('sing'),                  # Dm
    ADDTRA(-5), GOTO('sing'),                 # Am
    ADDTRA(-4), GOTO('sing'),                 # F
    ADDTRA(2), GOTO('sing'),                  # G7
    VIBOFF,
    # ---- A' (8 bars): same, a whole step higher
    VOL(40), SLV(-1),
    SETTRA(2), REPEAT(1, 'phr_a'),
    # ---- OUTRO (4 bars): fading vibrato melody
    VOL(34), SLVADV(-1, 5), VIB(3, 3),
    SETTRA(5), GOTO('sing'),                  # D
    ADDTRA(-7), GOTO('sing'),                 # G
    ADDTRA(7), GOTO('sing'),                  # D
    NOTE('E5'), DELAY(64),                    # held 5th of D
    STOP(),
])
track('gull', [
    VOL(24), SLV(-1),
    NOTE('G6'), GLIS(0x82), DELAY(18), GLISOFF, DELAY(14),
    NOTE('E6'), GLIS(0x82), DELAY(12), GLISOFF, DELAY(20),
    RET(),
])
track('sing', n_('E5', 32) + n_('A5', 16) + n_('B5', 16) + [RET()])
track('riff_a', (n_('E5', 12) + n_('G5', 4) + n_('A5', 8) + n_('G5', 8) +
                 n_('E5', 12) + n_('D5', 4) + n_('C5', 8) + n_('D5', 8) + [RET()]))
track('riff_b', (n_('E5', 12) + n_('G5', 4) + n_('A5', 8) + n_('G5', 8) +
                 n_('E5', 8) + n_('D5', 8) + n_('C5', 16) + [RET()]))
track('phr_a', [GOTO('riff_a'), ADDTRA(5), GOTO('riff_a'), ADDTRA(2), GOTO('riff_a'),
                ADDTRA(-7), GOTO('riff_b'), RET()])
track('riff_p', (n_('A5', 8) + n_('A5', 8) + n_('E5', 8) + n_('A5', 8) +
                 n_('A5', 8) + n_('E5', 8) + n_('A5', 8) + n_('B5', 8) + [RET()]))

# =============================================================================== CH1 chords
track('chords', [
    # ---- INTRO: slow arpeggio pad, fade-in (positive slide, ends at ~28)
    VOL(0), SLVADV(1, 8), ARP(0x43, 0x23),
    NOTE('C4'), DELAY(64), NOTE('C4'), DELAY(64), NOTE('F4'), DELAY(64), NOTE('G4'), DELAY(64),
    # ---- A: reggae skank on beats 2 and 4
    VOL(30), SLV(-6), ARP(0x43, 0x20),
    GOTO('sk_a1'), GOTO('sk_a2'),
    # ---- B: arpeggio stabs every beat
    VOL(28), SLV(-1),
    GOTO('st_b1'), GOTO('st_b2'),
    # ---- A' (lifted)
    VOL(30), SLV(-6), ARP(0x43, 0x20),
    GOTO('sk_a1b'), GOTO('sk_a2b'),
    # ---- OUTRO: slow pad again, fading
    VOL(26), SLVADV(-1, 6), ARP(0x43, 0x23),
    NOTE('D4'), DELAY(64), NOTE('G4'), DELAY(64), NOTE('D4'), DELAY(64), NOTE('A4'), DELAY(64),
    STOP(),
])


def skank(root):
    return [DELAY(16), NOTE(root), DELAY(32), NOTE(root), DELAY(16), RET()]


for nm, r in [('C', 'C4'), ('F', 'F4'), ('G', 'G4'), ('Am', 'A3'), ('D', 'D4'), ('A', 'A4'), ('Bm', 'B3')]:
    track('sk_' + nm, skank(r))
track('sk_a1', [GOTO('sk_C'), GOTO('sk_F'), GOTO('sk_G'), GOTO('sk_C'), RET()])
track('sk_a2', [ARP(0x34, 0x20), GOTO('sk_Am'), ARP(0x43, 0x20),
                GOTO('sk_F'), GOTO('sk_G'), GOTO('sk_C'), RET()])
track('sk_a1b', [GOTO('sk_D'), GOTO('sk_G'), GOTO('sk_A'), GOTO('sk_D'), RET()])
track('sk_a2b', [ARP(0x34, 0x20), GOTO('sk_Bm'), ARP(0x43, 0x20),
                 GOTO('sk_G'), GOTO('sk_A'), GOTO('sk_D'), RET()])
for nm, r in [('Am', 'A3'), ('G', 'G3'), ('F', 'F3'), ('E', 'E3'), ('Dm', 'D4')]:
    track('st_' + nm, [NOTE(r), DELAY(16), RET()])
track('st_b1', [ARP(0x34, 0x20), REPEAT(3, 'st_Am'),
                ARP(0x43, 0x20), REPEAT(3, 'st_G'), REPEAT(3, 'st_F'), REPEAT(3, 'st_E'), RET()])
track('st_b2', [ARP(0x34, 0x20), REPEAT(3, 'st_Dm'), REPEAT(3, 'st_Am'),
                ARP(0x43, 0x20), REPEAT(3, 'st_F'),
                ARP(0x46, 0x20), REPEAT(3, 'st_G'), RET()])          # G7 shell (G B F)

# =============================================================================== CH2 bass
track('bass', [
    # ---- INTRO: swell (positive slide, ends at ~51)
    VOL(0), SLVADV(1, 4),
    NOTE('C3'), DELAY(64), NOTE('C3'), DELAY(64), NOTE('F3'), DELAY(64), NOTE('G3'), DELAY(64),
    # ---- A
    VOL(63), SLV(-5),
    SETTRA(0), GOTO('bass_p1'), GOTO('bass_p2'),
    # ---- B: cannon boom on beat 1, then galloping eighths; transposed per chord
    SLV(-3),
    SETTRA(-3), GOTO('boom'), GOTO('bass_bx'),            # Am (bar 1: boom + 6 notes)
    ADDTRA(-2), GOTO('bass_b'),                           # G
    ADDTRA(-2), GOTO('bass_b'),                           # F
    ADDTRA(-1), GOTO('bass_b'),                           # E
    ADDTRA(10), GOTO('bass_b'),                           # Dm
    ADDTRA(-5), GOTO('bass_b'),                           # Am
    ADDTRA(-4), GOTO('bass_b'),                           # F
    ADDTRA(2), GOTO('bass_b'),                            # G7
    # ---- A' (lifted)
    VOL(63), SLV(-5),
    SETTRA(2), GOTO('bass_p1'), GOTO('bass_p2'),
    # ---- OUTRO: long notes, fading
    VOL(63), SLVADV(-1, 3), SETTRA(0),
    NOTE('D3'), DELAY(64), NOTE('G2'), DELAY(64), NOTE('D3'), DELAY(64), NOTE('D2'), DELAY(64),
    STOP(),
])
track('bass_h', n_('C3', 12) + n_('G3', 4) + n_('C4', 8) + n_('G3', 8) + [RET()])
track('bass_a', [REPEAT(1, 'bass_h'), RET()])
track('bass_f', [GOTO('bass_h')] + n_('C3', 8) + n_('D3', 8) + n_('E3', 8) + n_('G3', 8) + [RET()])
track('bass_p1', [GOTO('bass_a'), ADDTRA(5), GOTO('bass_a'), ADDTRA(2), GOTO('bass_a'),
                  ADDTRA(-7), GOTO('bass_f'), RET()])
track('bass_p2', [ADDTRA(-3), GOTO('bass_a'), ADDTRA(8), GOTO('bass_a'), ADDTRA(2), GOTO('bass_a'),
                  ADDTRA(-7), GOTO('bass_f'), RET()])
track('bass_bx', (n_('G3', 8) + n_('C3', 8) + n_('C4', 8) + n_('G3', 8) + n_('C3', 8) + n_('G3', 8) + [RET()]))
track('bass_b', n_('C3', 8) + n_('C3', 8) + [GOTO('bass_bx'), RET()])
# cannon: a 16-tick downward frequency slide, then back to the volume slide of the pattern
track('boom', [VOL(63), SLFRQ(-24), NOTE('C3'), DELAY(16), SLV(-3), RET()])

CH_ENTRY = ['lead', 'chords', 'bass', 'drums']
assert ORDER[0] == 'drums'


# ---------------------------------------------------------------- assemble
def assemble():
    idx = {name: i for i, name in enumerate(ORDER)}
    blobs = []
    for name in ORDER:
        b = []
        for c in TR[name]:
            if c[0] == 'GOTO':
                b += [0xFC, idx[c[1]]]
            elif c[0] == 'REPEAT':
                b += [0xFD, c[1], idx[c[2]]]
            else:
                b += c[1]
        blobs.append(b)
    offs, pos = [], 0
    for b in blobs:
        offs.append(pos)
        pos += len(b)
    n = len(ORDER)
    song = [n]
    for o in offs:
        song += [o & 0xFF, o >> 8]
    song += [idx[e] for e in CH_ENTRY]
    for b in blobs:
        song += b
    return song, idx, offs, blobs


HEADER = '''#ifndef CUTLASS_H
#define CUTLASS_H

// ---------------------------------------------------------------------------
//  "Cutlass Cove" - an original tropical-pirate showcase for ATMlib       {total} bytes
//
//  Play with:   #include "cutlass.h"
//               ATM.play(cutlassCove);
//
//  A ONE-SHOT: it ends by itself after 2048 ticks (32 bars).  Tempo is set INSIDE the song
//  (ATM_SET_TEMPO({tempo}), then ATM_ADD_TEMPO changes), so do not call ATM.setTempo() while it plays.
//
//  FORM (1 bar = 64 ticks = 4 beats of 16 ticks)
//     bars  1- 4  INTRO   fade-in: sea, seagulls, arpeggio pad, bass swell        CUE 1
//     bars  5-12  A       C | F | G | C   Am | F | G | C   (calypso / reggae)       CUE 2
//     bars 13-20  B       Am G F E   Dm Am F G7  (storm, tempo +6)                CUE 3
//     bars 21-28  A'      same as A, lifted a whole step: D | G | A | D ...       CUE 4
//     bars 29-32  OUTRO   ritardando (tempo -1 per beat) + fade-out               CUE 5
//     last tick                                                                   CUE 6
//  Poll the cues from your sketch with  uint8_t c = ATM.check();  (0 = nothing new)
//
//  WHAT IT SHOWS OFF  (nearly every ATMlib command)
//     ATM_SET_TEMPO / ATM_ADD_TEMPO      A->B speed-up, B->A' reset, outro ritardando (-1 per beat)
//     ATM_CUE                            section markers 1..6 (and a final "song ended" 6)
//     ATM_GOTO / ATM_REPEAT / ATM_RETURN nested tracks: bars are tracks, phrases call bars, depth 4
//     ATM_ADD_TRA / ATM_SET_TRA          ONE riff per chord on lead + bass (C F G C, Am F G C ...),
//                                        the whole A' section is the A tracks + SET_TRA(2)
//     ATM_ARP                            CH1 pad (slow, 4 ticks/step), skank + stabs (1 tick/step),
//                                        major 0x43, minor 0x34 and a G7 shell 0x46
//     ATM_VOL / ATM_SL_VOL               plucked steel-drum lead, skank decay, drum hits
//     ATM_SL_VOL_ADV / ATM_SL_VOL_OFF    long fades: +1 every 5-9 ticks in, -1 every 4-7 ticks out
//     ATM_SL_FRQ                         the cannon "boom" on the bass at the start of B
//     ATM_GLIS                           the two seagull cries (falling glissando)
//     ATM_VIB                            pan-flute / singing lead
//     ATM_CUT                            staccato pedal riff in B (gate: 4 ticks on, 4 off)
//     ATM_TREM                           sea swell (slow) and the snare roll (flip every tick)
//     ATM_NOISE                          thunder rumble at the start of B
//     ATM_STOP_CHAN                      every channel stops by itself at the end
//  Not used here: ATM_GOTO_ADV (loops), ATM_SL_FRQ_ADV, the *_OFF of arp/transpose, ATM_WAVEFORM.
//
//  CHANNELS (ATMlib defaults)
//     CH0 PULSE   lead      CH1 SQUARE  chords      CH2 SAW  bass      CH3 NOISE  drums + sea + cues
//
//  Needs every ATM_FX_* option left ON (the default).  Track 0 is only CH3's entry point and is
//  never called: ATMlib starts every channel with "current track = 0".
// ---------------------------------------------------------------------------

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif


'''


def render_text(idx, offs, blobs, total):
    L = [HEADER.format(total=total, tempo=TEMPO)]
    L.append(f'Song {SONG_VAR}[] = {{     // total song bytes = {total}')
    L.append(f'  0x{len(ORDER):02X},                       // Number of tracks')
    for i, name in enumerate(ORDER):
        o = offs[i]
        L.append(f'  0x{o & 0xFF:02X}, 0x{o >> 8:02X},                 // Address of track {i:<2d} {o:5d}   {name}')
    L.append('')
    for k, e in enumerate(CH_ENTRY):
        L.append(f'  0x{idx[e]:02X},                         // CH{k} entry -> track {idx[e]} ({e})')
    L.append('')
    for i, name in enumerate(ORDER):
        L.append(f'  //"Track {i}" {name}  [{len(blobs[i])}b]')
        for c in TR[name]:
            if c[0] == 'GOTO':
                L.append(f'  ATM_GOTO({idx[c[1]]}),   // -> {c[1]}')
            elif c[0] == 'REPEAT':
                L.append(f'  ATM_REPEAT({c[1]}, {idx[c[2]]}),   // {c[1] + 1}x {c[2]}')
            else:
                L.append(f'  {c[0]},')
        L.append('')
    L += ['};', '', '#endif', '']
    return '\n'.join(L)


if __name__ == '__main__':
    song, idx, offs, blobs = assemble()
    total = len(song)
    print(f'tracks={len(ORDER)} total={total} bytes')
    for i, name in enumerate(ORDER):
        print(f'  T{i:<2d} {name:10s} {len(blobs[i]):3d}b', end='   ' if i % 4 != 3 else '\n')
    print()
    open('/home/claude/work/cutlass.bin', 'wb').write(bytes(song))
    open('/home/claude/work/cutlass_text.h', 'w').write(render_text(idx, offs, blobs, total))
    print('wrote cutlass.bin / cutlass_text.h')
```
