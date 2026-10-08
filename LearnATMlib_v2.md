# LearnATMlib: how to write a song for ATMlib (Arduboy)

**Audience:** a person or an AI starting from zero.
**Status (loop update):** all loops now use the *self-loop* method of section 6 (no `ATM_STOP_CHAN`, no `ATM_GOTO_ADV`). The old STOP + `GOTO_ADV` loop leaves one silent tick at the seam, which the user heard as a "hop" in every song; the self-loop fixed it (confirmed by ear). Italo Night, Escaper Droid, Neon Heart, Quest Theme and Star Light were converted; the converter is Appendix F.
**Status:** every statement here was checked against the ATMlib source (`ATMcmds.h`, `ATMlibimpl.h`, `README.md`) and against songs the user approved (Italo Night, Escaper Droid, Midnight Run, Quest Theme). Where something is an empirical finding rather than source code, it says so.
**How to use this file:** give a fresh AI this file plus the ATMlib folder (and, ideally, `song.h`, the reference song). Say what you want (style, key, length, byte budget, loop or one-shot). The AI should read sections 0, 4, 6 and 9 first.

---

## 0. Start here

### 0.1 The user's house rules (always apply)

1. **Double the tempo.** Wherever an emulator or offline analysis says `17`, write `ATM_SET_TEMPO(34)`. On the real Arduboy the song plays at about *tempo-value* ticks per second (see 5.1). Quest Theme uses 34.
2. **Deliver only the song `.h`.** No WAV file unless asked.
3. **Write songs as commands**, e.g. `ATM_NOTE_A4, ATM_DELAY(8), ATM_VOL(38)`. Never write track bodies as raw hex. The small header (track count, track addresses, channel entry tracks) is hex, exactly as in the reference `song.h`.
4. **Byte budget means the whole array**, including the header. "Max 250 bytes" = `sizeof(song) <= 250`.
5. **Loops must be hop-free.** A `STOP` + `GOTO_ADV` restart silences all four channels for one tick (about 30 ms) at every loop, and the user hears it. Make every loop a *self-loop*: each channel's entry track ends with `ATM_GOTO(<itself>)` and every channel sums to exactly `L` ticks. See section 6.
6. **Tempo is the user's to set when they say so.** If they set it from the sketch (`ATM.setTempo()`), leave `ATM_SET_TEMPO` out of the song (`quest.h` has none now: without it the song plays at ATMlib's default of 25, not 34).
7. **Quality bar:** Italo Night, Midnight Run and Quest Theme were "really good / perfect"; the five in-game loops (section 8.7) were "all liked". Pixel Chase was rejected: too plain, and actually broken (section 12).

### 0.2 Starter prompt for a fresh session

> Read `LearnATMlib.md` completely, then `ATMlib/src/ATMcmds.h` and `song.h`. Write a new ATMlib song: *(style, mood, key, number of bars or length, loop or one-shot, byte budget)*. Follow section 0.3, verify with the simulator, compile-check the header, and deliver only the `.h` in `ATM_*` commands with the doubled tempo.

### 0.3 The workflow in ten lines

1. Read `ATMcmds.h` for the real opcode values. Never guess them (section 12, mistake 1).
2. Decide: tempo (doubled), key, number of bars, loop or one-shot, byte budget.
3. Time grid: **16 ticks = 1 beat, 64 ticks = 1 bar** (used by all approved songs).
4. Plan every channel in ticks. For a loop of `L` ticks every channel must contain exactly `L` ticks of delay, and its entry track must end with `ATM_GOTO(itself)` (section 6). No `STOP`, no `GOTO_ADV`, no trimmed last delay, no `_last` tracks.
5. Write the song with a **generator script** (named tracks, automatic track numbers and offsets). Appendix B is a complete example.
6. Run the **simulator** (Appendix A): no channel ever stops (except an unused one at tick 0), the song never restarts, the per-tick timeline repeats exactly with period `L`, call depth is at most 7, no note out of range.
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
* A *called track* ends with `ATM_RETURN`. An *entry track* of a **looping** song ends with `ATM_GOTO(<itself>)` (section 6). An entry track of a **one-shot** song ends with `ATM_STOP_CHAN`. An *unused channel* points at a track that holds only `ATM_STOP_CHAN` (1 byte).
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
| `ATM_STOP_CHAN` | `0x9F` | Stop this channel **and set its volume to 0** (4.4). Use it for one-shots and unused channels. **Never use it to loop**: the restart leaves one silent tick. |

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
| `ATM_GOTO_ADV(a,b,c,d)` | `0x9E, a,b,c,d` | Set the **restart track** for CH0..CH3 used when all channels have stopped (4.4). **Obsolete for loops** (it needs `STOP`, which leaves a silent tick); do not use it any more. |
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
* A call to the track the channel is *currently in* does **not** push; it just restarts that track, **in the same tick**, without touching volume or any other state. This is what the self-loop of section 6 is built on. (Because every channel starts with "current track = 0", the very first `GOTO(entry)` of a channel whose entry track is not 0 does push one frame, once; count it in the depth limit.)
* **Every channel starts with "current track = 0", whatever its entry track is.** So a channel that calls track 0 from its entry track does not push a frame. When track 0 then returns with an empty stack, it simply restarts itself forever.
  **Rule: never call track 0 from another track.** Track 0 may be used only as a channel entry (CH3 in Quest Theme) or as a dummy that nobody calls (the older songs have a "silent" track 0). An entry track 0 that ends with `ATM_GOTO(0)` is fine: that is a call to the track it is already in (see below).
  *Measured example:* the rejected `pixelchase.h` had its melody bar as track 0. In the simulator CH0 plays bars 1-6 correctly, then repeats bar 1 forever and never stops, while the other three channels stop at tick 511 and the song never restarts.
* `RETURN` with a repeat counter above 0, or with an empty stack, **restarts the current track** instead of leaving it. So ending an entry track with `RETURN` instead of `STOP` loops that track forever.
* `REPEAT(n,t)` runs t n+1 times. Nested repeats are fine (counters are saved in the frame).

### 4.4 Stop, end of song and looping

* `STOP` **sets the channel volume to 0** (`ch->vol = 0`, `ATMlibimpl.h`, case 95) and parks the channel with an endless delay. When all four channels have stopped:
  * if the sum of the four `GOTO_ADV` restart tracks is 0 (or `GOTO_ADV` was never executed) the song **ends**;
  * otherwise every channel jumps to its restart track and the song **continues**.
* **This restart is the cause of the loop "hop".** The channels that stopped first are silent (volume 0) from their `STOP` until the last one stops, and the restart itself costs one more tick, so every loop contains a tick in which *all four channels are silent*. The period is exactly right, but the seam is audible (about 30 ms at tempo 34). This was found by ear on Italo Night, Escaper Droid, Neon Heart, Quest Theme, Star Light and the first version of the in-game songs.
* **Looping songs therefore must not stop at all.** Loop with a self-`GOTO` at the end of every channel's entry track (section 6). Do not use `STOP`, do not use `GOTO_ADV`.
* **One-shot songs** end with `STOP` on every channel and contain no `GOTO_ADV` (Midnight Run, a 3-minute one-shot, has none. When Claude once left it in, the song restarted).
* *Background, only for old files:* the restart is decided right after the last channel stops, inside the channel loop. If the last channel to stop is not CH3, the channels after it start the new loop one tick earlier than the ones before it, so all four channels had to stop on the same tick. The restart does not reset volume, slides, arpeggio or transposition. Both facts are irrelevant for a self-loop, but the second one still holds: **state persists across the loop**, so set what you need at the start of each channel's main track (typically `ATM_SET_TRA(0)`).

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

## 6. Hop-free loops: the self-loop (the most important section)

Let the loop be `L` ticks long (Quest Theme: 16 bars x 64 = 1024).

**The method**

1. Every channel's entry track ends with **`ATM_GOTO(<that same track>)`** instead of `ATM_STOP_CHAN`. A call to the track you are already in does not push the call stack: it jumps back to the first command of the track **in the same tick**. No tick is lost, nothing is silenced, volume and slides simply carry on.
2. Every channel contains **exactly `L` ticks of delay** (not `L-1`). The last note of the last bar keeps its full delay.
3. No `ATM_STOP_CHAN`, no `ATM_GOTO_ADV`, no trimmed delay, no `_last` variant tracks.
4. An unused channel's entry track holds a single `ATM_STOP_CHAN` (1 byte); it is silent from tick 0, which is what you want.
5. State persists across the loop (volume, slide, transposition, arpeggio). Set what the pass depends on at the start of each entry track (typically `ATM_SET_TRA(0)`), and let the state at the end equal the state at the start.

```
Song x[] = {
  ...
  //"Track 3" lead
  ATM_VOL(38),
  ATM_SL_VOL((uint8_t)-2),
  ...bars...
  ATM_GOTO(3),   // loop: restart this track (no STOP, no gap)
```

**Why the old method hopped** is in 4.4 (`STOP` sets volume 0 and the restart tick is silent on all channels). The self-loop never stops, so there is no seam to hear. The user confirmed it by ear: *"YUP, that fixed it"*.

**Details that matter**

* **Delay sum:** if one channel sums to `L+1` or `L-1` it drifts by one tick against the others every loop. The simulator catches it (see below).
* **Call depth:** an entry track other than 0 pushes one frame on its first `GOTO(itself)` (every channel starts with "current track = 0"). Entry track 0 (CH3 in Quest Theme) pushes none. Count this against the limit of 7.
* **Intro + body:** give every channel an *entry track* `E = [GOTO intro, GOTO loopT]` and a *loop track* `loopT = [GOTO body, GOTO loopT]`. The intro plays once; the loop track repeats the body forever and calls itself. The body must start by setting everything it depends on (`ATM_VOL`, `ATM_SL_VOL`, `ATM_SET_TRA`, `ATM_ARP`). Star Light (Appendix E) is built this way. A leftover slide counter of the intro can change a volume by a few units for a few ticks in the first body pass only; the note events are identical.
* **Cost:** `GOTO` is 2 bytes where `STOP` was 1 (+1 per channel), `GOTO_ADV` (5 bytes) disappears, and every `_last` variant track (10 to 40 bytes plus 2 in the table) disappears because the last bar is the same as every other pass of that bar. Converting Italo Night saved 89 bytes (558 to 469), Escaper Droid 89, Neon Heart 89, Star Light 147.
* **Tempo command in the loop:** a song whose drum track (the usual CH3 entry, track 0) begins with `ATM_SET_TEMPO` re-executes it every pass. It sets the same value, so it is harmless.

**Proof, not belief.** Simulate three loops (Appendix A, B). All of these must hold:

* `restarts == []` and no `STOP` after tick 0 (an unused channel's `STOP` at tick 0 is fine)
* for every tick `t >= L`: `TIMELINE[t] == TIMELINE[t-L]` for all four channels (volume, and frequency whenever the volume is not 0; a silent channel's last pitch is irrelevant)
* the note events of loop 1, 2 and 3 are identical
* max call depth <= 7

**Converting an old STOP + GOTO_ADV song:** `convert.py` (Appendix F) does it automatically: it adds 1 to the last executed delay of every channel (the old loop was `L-1` ticks of delay plus one restart tick), turns the entry track's final `STOP` into `GOTO(entry)`, deletes `GOTO_ADV`, merges the tracks that became identical, and checks the result against the old song with the simulator and the real `ATMcmds.h`.

**One-shot songs:** no self-loop, no `GOTO_ADV`; channels end with `ATM_STOP_CHAN` and may end at different times; the song ends when the last one stops. For fades use `ATM_VOL` steps per section or `ATM_SL_VOL`.

---

## 7. Fitting a byte budget

Costs: note 1, delay 1, `VOL/SL_VOL/SET_TRA/ADD_TRA/SET_TEMPO` 2, `ARP` 3, `GOTO` 2, `REPEAT` 3, `RETURN/STOP` 1, plus 2 per track in the table and 5 for the header.

What saves bytes (in the order that mattered):

1. **Transposition instead of copies.** One 2-bar riff (30 B) plays over every chord with `SET_TRA`/`ADD_TRA` (2 B per use). The bass bar is written once in the key of A and transposed the same way.
2. **`REPEAT`** for identical bars: 3 bytes for any number of repeats.
3. **Shared tracks** pay off when a pattern of length `L >= 5` is used at least twice.
4. **Relative transposition cycles**: a track of `[riff][ADD_TRA(5) riff][ADD_TRA(-5) riff]` returns to its start key, so the whole cycle can be re-played in another key by one `SET_TRA`.
5. **Inline tiny things** (a 4-byte track costs 6 with its table entry).
6. **Drums**: one 24-tick core shared by every half-bar, called with `REPEAT(30, ...)`.
7. Chords as explicit long notes with an arp: `NOTE, DELAY(64), NOTE, DELAY(64)` is 4 bytes per two bars.

Quest Theme budget (233 bytes, self-loop): header 29 B, drums 39 (entry + core + half), lead 68, chords 41, bass 56. (The old STOP + `GOTO_ADV` version was 247 B: it needed the 5-byte `GOTO_ADV`, a trimmed `bass_bar_last` track and `STOP`s.)

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

### 8.7 Tiny in-game loops (50 to 100 bytes)

The five songs in `ingame5.h` (Meadow Waltz 94 B, Lantern Lake 73 B, Sunday Market 89 B, Pixel Breeze 87 B, Music Box Tide 87 B) were written to show how little a pleasant, smooth, looping background track needs. What they share:

* **No drums unless the groove needs them** (Lantern Lake, Meadow Waltz, Pixel Breeze, Music Box Tide have none; Sunday Market uses one 16-tick brush track with `REPEAT`). Harmony and a good bass do the work.
* **Self-loop on every channel** (section 6): the entry track is the whole part and ends with `ATM_GOTO(itself)`. With only 3 or 4 tracks the header is 11 to 15 bytes.
* **Unused channels** point at a 1-byte `ATM_STOP_CHAN` track.
* **One long note per bar + an arpeggio** is the cheapest chord: `ATM_ARP(0x34, 0x23)` once, then `NOTE, DELAY(64)` per bar = 3 bytes per bar. Slow arps (`0x23`: 4 ticks per step) sound dreamy; fast ones (`0x20`) sound like stabs.
* **Soft decay** instead of drums for rhythm: `ATM_VOL(30), ATM_SL_VOL(-1)` makes every note a bell.
* **One riff, many chords:** Pixel Breeze plays one 6-note pentatonic riff over `C G F C` with one cycle track moved by `ATM_ADD_TRA`; the bass calls the same track from `ATM_SET_TRA(-24)`.
* **Rhythmic tricks that cost nothing:** 3-3-2 bossa grid (24+24+16 ticks); 5 against 3 phasing (a 5-note pattern of 8-tick notes against a 3-note arp at 8 ticks per step realigns every 120 ticks, exactly at the chord change, Music Box Tide).
* **Waltz** = 36 ticks per bar (3 beats of 12), loop 288 = 8 bars.
* **Tempo is the hardware value** here (20 to 32); `ATM_SET_TEMPO` costs 2 bytes, leave it out if the sketch calls `ATM.setTempo()`.

Complete example, Lantern Lake (73 bytes, D minor, tempo 20, loop 256 ticks = 12.8 s). Each channel is one self-looping track:

```cpp
Song lanternLake[] = {          // total song in bytes = 73
  0x04,                         // Number of tracks
  0x00, 0x00,                   // Address of track 0   //0
  0x18, 0x00,                   // Address of track 1   //24
  0x2D, 0x00,                   // Address of track 2   //45
  0x3B, 0x00,                   // Address of track 3   //59

  0x00,                         // Channel 0 entry track (bells)
  0x01,                         // Channel 1 entry track (pad)
  0x02,                         // Channel 2 entry track (bass)
  0x03,                         // Channel 3 entry track (silent)

  //"Track 0" bells  [24 bytes] - two bell strikes per bar, one per half bar
  ATM_SET_TEMPO(20),
  ATM_VOL(30),
  ATM_SL_VOL((uint8_t)-1),
  ATM_NOTE_A5, ATM_DELAY(32),
  ATM_NOTE_F5, ATM_DELAY(32),
  ATM_NOTE_D6, ATM_DELAY(32),
  ATM_NOTE_A5_, ATM_DELAY(32),
  ATM_NOTE_A5, ATM_DELAY(32),
  ATM_NOTE_C6, ATM_DELAY(32),
  ATM_NOTE_G5, ATM_DELAY(32),
  ATM_NOTE_E5, ATM_DELAY(32),
  ATM_GOTO(0),            // loop: this very track

  //"Track 1" pad  [21 bytes] - slow arpeggio chords: Dm | Bb | F | C
  ATM_VOL(26),
  ATM_TREM(1, 7),
  ATM_ARP(0x34, 0x23),
  ATM_NOTE_D4, ATM_DELAY(64),
  ATM_ARP(0x43, 0x23),
  ATM_NOTE_A3_, ATM_DELAY(64),
  ATM_NOTE_F4, ATM_DELAY(64),
  ATM_NOTE_C4, ATM_DELAY(64),
  ATM_GOTO(1),

  //"Track 2" bass  [14 bytes] - one soft root per bar
  ATM_VOL(30),
  ATM_SL_VOL((uint8_t)-1),
  ATM_NOTE_D3, ATM_DELAY(64),
  ATM_NOTE_A2_, ATM_DELAY(64),
  ATM_NOTE_F3, ATM_DELAY(64),
  ATM_NOTE_C3, ATM_DELAY(64),
  ATM_GOTO(2),

  //"Track 3" silent  [1 byte] - unused channel stops at once
  ATM_STOP_CHAN,
};
```

All three musical channels sum to exactly 256 ticks (8 x 32, 4 x 64, 4 x 64). The pad's arpeggio is set before the first note and its two chord qualities are set before their notes (minor `0x34`, major `0x43`); the second pass starts with the arpeggio still on `0x43` from the last bar, but `ATM_ARP(0x34, 0x23)` at the top of the track resets it before the first note, so every pass is identical. A WAV preview of all five songs was made with a small offline synth (pulse 25 %, square, saw, noise; tick rate = tempo), not with ATMlib itself.

---

## 9. Tooling

Appendix A is `atmsim.py`: a Python model of the playroutine's control flow (call stack, track-0 behaviour, delays, REPEAT counters, transposition, volume retrigger, arp setup, the STOP / GOTO_ADV restart rule for old files). It does **not** render audio. Since Cutlass Cove it also models the per-tick state: volume/frequency slides (with the missing upper clamp), tremolo, vibrato, glissando, arpeggio and the note-cut gate (and raises if cut is used with transposition), tempo commands, cues. After `simulate()`, `atmsim.TIMELINE[tick]` holds `(volume, frequency_hz)` for each of the four channels, which is how fades, the cut gate and slides are checked. It still raises on `ATM_WAVEFORM` and the long delay. Two logs support converters and loop checks: `atmsim.DELAYLOG` (tick, channel, address of every delay executed) and `atmsim.STOPLOG` (tick, channel, address of every `STOP`).

Appendix B is `gen_quest.py`: the full generator of Quest Theme (self-loop version, 233 B). It contains the pieces to reuse: command constructors, named tracks, assembler (offsets, track numbers), verifier, header-file writer.

**Verification checklist (all must pass)**

- [ ] `sizeof(song)` within the byte budget
- [ ] **loops:** `restarts == []` and no `STOP` after tick 0 (unused channels excepted); no `ATM_GOTO_ADV`
- [ ] **loops:** every channel's entry track ends with `ATM_GOTO(itself)`, and every channel sums to exactly `L` ticks
- [ ] **loops:** `TIMELINE[t] == TIMELINE[t-L]` for all `t >= L` (volume, and frequency while audible)
- [ ] loops 1, 2, 3 have identical note events
- [ ] max call depth <= 7
- [ ] no track calls track 0
- [ ] every `note + transposition` is within 1..63
- [ ] compiled header bytes == generator bytes (Appendix C)
- [ ] tempo is the doubled value (in-game songs: the stated hardware value)
- [ ] one-shots only: every channel ends with `STOP`, no `GOTO_ADV`

---

## 10. Output file template

```cpp
#ifndef SONG_H
#define SONG_H

// ---------------------------------------------------------------------------
//  "Title" - style, key, bars, bytes
//  Play with:   #include "quest.h"      ATM.play(questTheme);
//  Timing : ATM_SET_TEMPO(34) (doubled). 1 beat = 16 ticks, 1 bar = 64 ticks ...
//  Form / channels. Loop: every entry track ends with ATM_GOTO(itself), no STOP, no hop.
// ---------------------------------------------------------------------------

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif

Song questTheme[] = {     // total song bytes = 233
  0x0C,                       // Number of tracks
  0x00, 0x00,                 // Address of track 0   0   drums
  ...                         // one line per track
  0x03,                       // CH0 entry -> track 3 (lead)
  0x06,                       // CH1 entry
  0x07,                       // CH2 entry
  0x00,                       // CH3 entry
  //"Track 0" drums  [10b]
  ATM_SET_TEMPO(34),
  ATM_REPEAT(30, 2),   // 31x drum_half
  ATM_GOTO(1),   // -> drum_core
  ATM_DELAY(8),
  ATM_GOTO(0),   // loop: this very track
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
4. **`GOTO_ADV` in a one-shot song** makes it restart. (Looping songs must not use it either, see mistake 11.)
5. **Off-by-one loops:** with the self-loop every channel must sum to exactly `L`; one channel off by a tick drifts against the others. (The old rule was `L-1`; do not mix the two.)
6. **Tempo not doubled.**
7. **Invalid names/arguments:** `ATM_NOTE_F#5`, `ATM_NOTE_OFF`, negative arguments without `(uint8_t)`.
8. **Plain, short, hex-looking songs** are not accepted. Use the Italo Night structure: real melody, kick/hat/snare with fills, transposed bass, chord stabs, a form with a destination.
9. **Combining `ATM_ARP` with transposition on one channel** (detunes the arpeggio).
10. **Trusting a counter instead of a model.** Verify with the simulator and by compiling.
11. **Looping with `STOP` + `GOTO_ADV`.** Every loop has one tick in which all four channels are silent (`STOP` sets volume 0, the restart tick plays nothing). The period was exact and the simulator's "all stops on tick `L-1`" check passed, yet the user heard a "hop" in every song. The model has to check *what is audible* (the per-tick timeline), not only when events happen. Fixed by the self-loop (section 6).
12. **Believing "gapless on paper".** The first attempt at a gapless version (trimming one tick and keeping `STOP`) was wrong for the same reason; only removing `STOP` from the loop removes the seam.

---

## 13. Catalog of the existing songs

| File | Array | Style | Size | Notes |
|---|---|---|---|---|
| `newsong.h` | `italoNight` | Italo disco, A minor, 8 bars | 469 B | Original, written at emulator tempo 17 (now `SET_TEMPO(35)` in the file; use the doubled value). Kick + arp stabs on CH1, transposed SAW bass, noise drums with fills. Chords Am F C G Am F G E. **Self-loop** (was 558 B with STOP + `GOTO_ADV`). |
| `newsong2.h` | `escaperDroid` | uptempo space, D major, 8 bars | 455 B | Same skeleton as Italo Night. ARP `0x47` / `0x37`. `ATM_CUE(1)` once per beat (inside the kick track, 2 bytes) for the game's dancing droid sprite. Self-loop (was 544 B). |
| `newsong3.h` | `neonHeart` | Italo disco, E minor, 8 bars | 476 B | Tempo 36. Italo Night skeleton with new key, melody and chords: Em C G D Em C D B (dominant pulls back to Em). Chord stabs use inversions (Em, C/E, G/D, D/F#, B/D#). Self-loop (was 565 B). |
| `synth.h` | `midnightRun` | 80s synthwave, A minor, 96 bars (~3 min) | 715 B | One-shot, fade in and out, no `GOTO_ADV`. |
| `quest.h` | `questTheme` | in-game chiptune, A minor, 16 bars | 233 B | Tempo 34 inside the song (remove `ATM_SET_TEMPO` if the sketch sets it). Arpeggio chords, transposed lead and bass, key lift and Em pivot, **self-loop** (was 247 B). Best reference for tight budgets. Generator: Appendix B. |
| `ingame5.h` | `meadowWaltz` | folk waltz in 3/4, C major, 8 bars of 36 ticks | 94 B | Tempo 26, loop 288 ticks. Melody written out, oom-pah-pah bass. Self-loop. See 8.7. |
| `ingame5.h` | `lanternLake` | ambient / dreamy, D minor, 4 bars | 73 B | Tempo 20, loop 256 ticks. Bells, slow arpeggio pad with tremolo, long bass roots. Listed in full in 8.7. |
| `ingame5.h` | `sundayMarket` | bossa-nova lounge, A dorian, 4 bars | 89 B | Tempo 32, loop 256 ticks. Minor-7 arp stabs (`0x37`), 3-3-2 bass, brush noise with `REPEAT`. |
| `ingame5.h` | `pixelBreeze` | light chiptune, C major pentatonic, 4 bars | 87 B | Tempo 30, loop 256 ticks. One riff moved by `ADD_TRA` over C G F C, bass calls the same cycle track. |
| `ingame5.h` | `musicBoxTide` | minimal music box, phasing 5 against 3, C G F | 87 B | Tempo 30, loop 360 ticks. 5-note pattern against a 3-note arp, realigns at each chord change. |
| `gameover.h` | `gameOver` | one-shot game over jingle, A minor, 5 bars | 106 B | Tempo 20. Andalusian descent Am-G-F-E-Am, one lead phrase transposed down with ADD_TRA, arp chords, bass on the same transposition, noise toll per bar. No GOTO_ADV, so it ends by itself. Reference for small one-shots (the user's own `youDied` does the same in 64 B). |
| `cutlass.h` | `cutlassCove` | original tropical-pirate showcase, C major / A minor, 32 bars (~75 s) | 839 B | One-shot, tempo 30 (+6 in B, ritardando in the outro), fades, cues 1..6. Uses nearly every command: Appendix D. **Not** a copy of any existing song (the Monkey Island theme is a copyrighted composition and is not reproduced). |
| `starlight.h` | `starLight` | original K-pop style dance track, C major, 3:00, intro + 84-bar body that loops forever | 1307 B | Tempo 33 (~124 BPM). Verse / pre-chorus / chorus / dance break / bridge / key-lifted last chorus (+2). Hook, pre motif and dance riff are one track each, transposed per chord. **Self-loop with intro** (was 1454 B). Appendix E. |
| `pixelchase.h` | `pixelChase` | NES-like | 181 B | **Broken (track-0 bug), rejected. Do not use.** |

---

## Appendix A: `atmsim.py` (playroutine simulator: control flow and per-tick state)

```python
#!/usr/bin/env python3
"""Simulator of ATMlib's playroutine (control flow + per-tick channel state). From LearnATMlib.md Appendix A."""
NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
TIMELINE = []
DELAYLOG = []
STOPLOG = []

def note_name(n):
    if n <= 0:
        return '--'
    n0 = n - 1
    return f"{NOTE_NAMES[n0 % 12]}{2 + n0 // 12}"

def freq_hz(note):
    if note <= 0 or note > 63:
        return 0
    return int(round(440 * 2 ** ((note - 10) / 12)))

def s8(v):
    return v - 256 if v > 127 else v

class Ch:
    def __init__(self):
        self.ptr = 0; self.note = 0; self.stack = []; self.repeatPoint = 0
        self.delay = 0; self.counter = 0; self.track = 0; self.vol = 0; self.freq = 0
        self.reCount = 0; self.reConfig = 0; self.slide = 0; self.slideCfg = 0; self.slideCount = 0
        self.arpNotes = 0; self.arpTiming = 0; self.arpCount = 0; self.arp = None; self.tra = 0
        self.treD = 0; self.treC = 0; self.treN = 0; self.glis = 0; self.glisCount = 0
        self.stopped = False

def parse(song):
    ntr = song[0]
    offs = [song[1 + 2 * i] | (song[2 + 2 * i] << 8) for i in range(ntr)]
    pos = 1 + 2 * ntr
    entries = list(song[pos:pos + 4])
    base = pos + 4
    return ntr, offs, entries, base

def simulate(song, max_ticks, trace=True):
    global TIMELINE
    TIMELINE = []
    DELAYLOG.clear(); STOPLOG.clear()
    ntr, offs, entries, base = parse(song)
    ch = [Ch() for _ in range(4)]
    for n in range(4):
        ch[n].ptr = base + offs[entries[n]]
    events = []; restarts, stops = [], []
    maxstack = 0; tempo = 25; tempo_log = []; tick = 0
    while tick < max_ticks:
        for n in range(4):
            c = ch[n]
            if c.reConfig:
                if c.reCount >= (c.reConfig & 3): c.reCount = 0
                else: c.reCount += 1
            if c.glis:
                cfg = c.glis & 0xFF
                if c.glisCount >= (cfg & 0x7F):
                    c.note += -1 if cfg & 0x80 else 1
                    c.note = max(1, min(63, c.note)); c.freq = freq_hz(c.note); c.glisCount = 0
                else: c.glisCount += 1
            if c.slide:
                if not c.slideCount:
                    isf = bool(c.slideCfg & 0x40)
                    vf = (c.freq if isf else c.vol) + c.slide
                    if not (c.slideCfg & 0x80):
                        if vf < 0: vf = 0
                        elif isf and vf > 9397: vf = 9397
                    if isf: c.freq = vf
                    else: c.vol = vf & 0xFF
                if c.slideCount >= (c.slideCfg & 0x3F): c.slideCount = 0
                else: c.slideCount += 1
            if c.arpNotes and c.note:
                if (c.arpCount & 0x1F) < (c.arpTiming & 0x1F):
                    c.arpCount += 1
                else:
                    if (c.arpCount & 0xE0) == 0x00: c.arpCount = 0x20
                    elif (c.arpCount & 0xE0) == 0x20 and not (c.arpTiming & 0x40) and c.arpNotes != 0xFF: c.arpCount = 0x40
                    else: c.arpCount = 0x00
                    an = c.note
                    if (c.arpCount & 0xE0) != 0x00:
                        an = 0 if c.arpNotes == 0xFF else an + (c.arpNotes >> 4)
                    if (c.arpCount & 0xE0) == 0x40: an += (c.arpNotes & 0x0F)
                    if c.arpNotes == 0xFF and an == 0 and c.tra != 0:
                        raise Exception('ATM_CUT with transposition != 0')
                    c.freq = freq_hz(an + c.tra)
            if c.treD:
                isf = bool(c.treC & 0x40)
                vt = c.freq if isf else c.vol
                vt = vt + c.treD if (c.treN & 0x80) else vt - c.treD
                if vt < 0: vt = 0
                elif isf and vt > 9397: vt = 9397
                if isf: c.freq = vt
                else: c.vol = vt & 0xFF
                if (c.treN & 0x1F) < (c.treC & 0x1F): c.treN += 1
                else: c.treN = 0 if (c.treN & 0x80) else 0x80
            if c.delay:
                if c.delay != 0xFFFF: c.delay -= 1
            else:
                while True:
                    cmd = song[c.ptr]; c.ptr += 1
                    if cmd < 64:
                        c.note = cmd
                        if cmd: c.note = (cmd + c.tra) & 0xFF
                        c.freq = freq_hz(c.note)
                        if c.slideCfg == 0: c.vol = c.reCount
                        if c.arpTiming & 0x20: c.arpCount = 0
                        events.append((tick, n, 'note', c.note, c.vol, c.arp, c.tra))
                    elif cmd < 160:
                        fx = cmd - 64
                        if fx == 0:
                            c.vol = song[c.ptr]; c.ptr += 1; c.reCount = c.vol
                        elif fx in (1, 4):
                            c.slide = s8(song[c.ptr]); c.ptr += 1; c.slideCfg = 0 if fx == 1 else 0x40
                        elif fx in (2, 5):
                            c.slide = s8(song[c.ptr]); c.slideCfg = song[c.ptr + 1]; c.ptr += 2
                            if fx == 5: c.slideCfg |= 0x40
                        elif fx in (3, 6): c.slide = 0
                        elif fx == 7:
                            c.arpNotes = song[c.ptr]; c.arpTiming = song[c.ptr + 1]
                            c.arp = (c.arpNotes, c.arpTiming); c.ptr += 2
                        elif fx in (8, 21): c.arpNotes = 0; c.arp = None
                        elif fx == 9: c.reConfig = song[c.ptr]; c.ptr += 1
                        elif fx == 10: c.reConfig = 0
                        elif fx == 11: c.tra = s8((c.tra + s8(song[c.ptr])) & 0xFF); c.ptr += 1
                        elif fx == 12: c.tra = s8(song[c.ptr]); c.ptr += 1
                        elif fx == 13: c.tra = 0
                        elif fx in (14, 16):
                            c.treD = song[c.ptr]; c.treC = song[c.ptr + 1] + (0 if fx == 14 else 0x40); c.ptr += 2
                        elif fx in (15, 17): c.treD = 0
                        elif fx == 18: c.glis = song[c.ptr]; c.ptr += 1
                        elif fx == 19: c.glis = 0
                        elif fx == 20:
                            c.arpNotes = 0xFF; c.arpTiming = song[c.ptr]; c.ptr += 1; c.arp = ('cut', c.arpTiming)
                        elif fx == 22: raise Exception("ATM_WAVEFORM unhandled")
                        elif fx == 23:
                            events.append((tick, n, 'cue', song[c.ptr])); c.ptr += 1
                        elif cmd == 156:
                            tempo = (tempo + song[c.ptr]) & 0xFF; c.ptr += 1; tempo_log.append((tick, tempo))
                        elif cmd == 157:
                            tempo = song[c.ptr]; c.ptr += 1; tempo_log.append((tick, tempo))
                        elif cmd == 158:
                            for i in range(4):
                                ch[i].repeatPoint = song[c.ptr]; c.ptr += 1
                        elif cmd == 159:
                            c.vol = 0; c.delay = 0xFFFF; c.stopped = True; stops.append((tick, n)); STOPLOG.append((tick, n, c.ptr - 1))
                        else: raise Exception(f"unmodelled FX {cmd:#x} at {c.ptr-1}")
                    elif cmd < 224:
                        c.delay = cmd - 159; DELAYLOG.append((tick, n, c.ptr - 1))
                    elif cmd in (252, 253):
                        newc = 0 if cmd == 252 else song[c.ptr]
                        if cmd == 253: c.ptr += 1
                        newt = song[c.ptr]; c.ptr += 1
                        if newt != c.track:
                            c.stack.append((c.counter, c.track, c.ptr - base))
                            if len(c.stack) > 7: raise Exception("call stack overflow (>7)")
                            maxstack = max(maxstack, len(c.stack))
                            c.track = newt
                        c.counter = newc
                        c.ptr = base + offs[c.track]
                    elif cmd == 254:
                        if c.counter > 0 or len(c.stack) == 0:
                            if c.counter: c.counter -= 1
                            c.ptr = base + offs[c.track]
                        else:
                            cnt, trk, p = c.stack.pop()
                            c.ptr = p + base; c.counter = cnt; c.track = trk
                    else: raise Exception(f"bad cmd {cmd} at {c.ptr-1}")
                    if c.delay != 0: break
                if c.delay != 0xFFFF: c.delay -= 1
            if all(x.stopped for x in ch):
                if sum(x.repeatPoint for x in ch):
                    restarts.append((tick, n))
                    for k in range(4):
                        ch[k].ptr = base + offs[ch[k].repeatPoint]
                        ch[k].delay = 0; ch[k].stopped = False
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

Run it in the same folder as `atmsim.py`; it writes `quest.h` and `quest.bin` there. This is the self-loop version (no `STOP`, no `GOTO_ADV`, no `_last` track).

```python
#!/usr/bin/env python3
"""
"Quest Theme" - chiptune in-game loop for ATMlib, target <= 250 bytes total.

16 bars x 64 ticks = 1024 ticks, A minor, second half lifted a whole step (Bm), pivoting back through Em.
  CH0 PULSE  lead   : one 2-bar riff, TRANSPOSED per chord (SET_TRA / ADD_TRA)
  CH1 SQUARE chords : ARPEGGIO minor triads (explicit roots - arp + transpose on the
                      same channel would transpose twice, see ATMlibimpl.h line ~457)
  CH2 SAW    bass   : root/octave pattern, TRANSPOSED per chord
  CH3 NOISE  drums  : tempo + kick/hat/snare
LOOPING: every channel's entry track ends with GOTO(itself) (a self-loop). There is no STOP
and no GOTO_ADV, so the loop restarts in the same tick (no silent tick = no 'hop') and every
channel sums to exactly 1024 ticks of delay.
Track 0 is never CALLED from another track (ATMlib starts every channel with 'current track'
= 0, calling track 0 breaks the call stack). It is only CH3's entry point; its own final
GOTO(0) is a call to the track it is already in, which is harmless.
"""
import sys
sys.path.insert(0, '.')
import atmsim
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


# ---------------------------------------------------------------- the song
TR = {}          # name -> list of commands
ORDER = []


def track(name, cmds):
    TR[name] = cmds
    ORDER.append(name)


def n_(name, d):
    return [NOTE(name), DELAY(d)]


# 0: drums (CH3 entry). tempo + 32 halves, then loop to itself
track('drums', [
    TEMPOc(TEMPO),
    REPEAT(30, 'drum_half'),             # 31 x 32t = 992t
    GOTO('drum_core'), DELAY(8),         # 24t + 8t = 32t  -> 1024t total
    GOTO('drums'),                       # self-loop: same-tick restart
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
    ADDTRA(5), GOTO('lead_riff'), DELAY(16),   # bars 15-16 Em (pivot back to Am)
    GOTO('lead'),                        # self-loop
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
    ch += [NOTE(r), DELAY(64), NOTE(r), DELAY(64)]
ch += [GOTO('chords')]                  # self-loop
track('chords', ch)

# 7: bass
track('bass', [
    VOL(63), SLV(-5),
    SETTRA(0), GOTO('bass_cycle'),                               # bars 1-6   Am Dm Am
    ADDTRA(7), REPEAT(1, 'bass_bar'),                            # bars 7-8   Em
    SETTRA(2), GOTO('bass_cycle'),                               # bars 9-14  Bm Em Bm (lifted)
    SETTRA(7), REPEAT(1, 'bass_bar'),                            # bars 15-16 Em
    GOTO('bass'),                                                # self-loop
])
track('bass_cycle', [
    REPEAT(1, 'bass_bar'),
    ADDTRA(5), REPEAT(1, 'bass_bar'),
    ADDTRA(-5), REPEAT(1, 'bass_bar'),
    RET(),
])
track('bass_bar', [REPEAT(2, 'bass_beat'), GOTO('bass_beat4'), RET()])
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
        '//           16 bars = 1024 ticks. Every channel sums to exactly 1024 ticks of delay and its',
        '//           entry track ends with ATM_GOTO(itself): same-tick restart, no STOP, no hop.',
        '//  Form   : bars 1-8   Am | Am | Dm | Dm | Am | Am | Em | Em',
        '//           bars 9-16  the cycle lifted a whole step: Bm | Bm | Em | Em | Bm | Bm | Em | Em',
        '//                      (Em is chord iv in Bm and chord v in Am: it pivots back into bar 1)',
        '//  Channels (ATMlib defaults):',
        '//     CH0 PULSE  - lead: ONE 2-bar riff, transposed per chord (SET_TRA / ADD_TRA)',
        '//     CH1 SQUARE - minor-triad ARPEGGIO chords (explicit roots, no transpose on this channel)',
        '//     CH2 SAW    - root/octave bass, transposed per chord, beat-4 variation',
        '//     CH3 NOISE  - tempo + kick click, hats, snare',
        '//  Note   : track 0 is only CH3\'s entry point and is never called from another track. ATMlib',
        '//           starts every channel with "current track = 0", so calling track 0 would break the',
        '//           call stack. (CH3\'s own end-of-track ATM_GOTO(0) is fine: it is a call to the track it is in.)',
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
    L = 1024
    ev, restarts, stops, maxstack, tempo = simulate(song, L * 3 + 40)
    tl = [list(t) for t in atmsim.TIMELINE]
    print('tempo set:', tempo, '| max call depth:', maxstack)
    ok = True
    if restarts:
        print('FAIL: the song restarted through STOP/ADV (it must self-loop)'); ok = False
    if [s_ for s_ in stops if s_[0] > 0]:
        print('FAIL: a channel stopped:', stops[:4]); ok = False
    nz = lambda r: [(v, f if v else 0) for v, f in r]      # a silent channel's pitch is irrelevant
    for t in range(L, 3 * L):
        if nz(tl[t]) != nz(tl[t - L]):
            print(f'FAIL: tick {t} differs from tick {t - L}'); ok = False; break
    if maxstack > 7:
        print('FAIL: call stack deeper than 7'); ok = False

    def loop_events(k):
        return [(t - L * k,) + e[1:] for e in ev for t in [e[0]] if L * k <= t < L * (k + 1)]
    l0, l1, l2 = loop_events(0), loop_events(1), loop_events(2)
    if not (l0 == l1 == l2):
        print('FAIL: loops differ (state leaking across the loop)'); ok = False
    else:
        print(f'loop 0 == loop 1 == loop 2  ({len(l0)} note events each, period {L} ticks)')

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

---

## Appendix E: `gen_starlight.py` (intro + looping body, 90 tracks, section table check)

Self-loop version of the K-pop track: every channel has an entry track (intro once, then the loop track) and a loop track (body, then itself). Uses the same helper block as Appendix D. Run it in the folder that contains `atmsim.py`; it checks that nothing stops or restarts, that body pass 1 and pass 2 are identical tick by tick, that the note events of passes 0 to 2 are identical, the note ranges and the volume, prints the first notes of every bar of every section, and writes `starlight.h` and `starlight.bin`.

```python
#!/usr/bin/env python3
"""
"Star Light" - original K-pop style dance track for ATMlib: 8-bar intro (once), then an 84-bar body that loops forever.
See HEADER below for the form.  Track 0 is only CH3's entry point and is never called from another track.

LOOPING (the hop-free method, see LearnATMlib.md section 6): every channel has a LOOP track that calls the body
and then calls ITSELF (GOTO to the track it is already in = same-tick restart, no STOP, no silent tick).
The entry track plays the intro once, then calls the loop track and never comes back.
"""
import sys
sys.path.insert(0, '.')
import atmsim
from atmsim import simulate, note_name

SONG_VAR = 'starLight'
TEMPO = 33            # doubled, as always (emulator ~16.5): ~124 BPM

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
def ARP(a, t):      return (f'ATM_ARP(0x{a:02X}, 0x{t:02X})', [0x47, a, t])
def ADDTRA(v):      return (f'ATM_ADD_TRA({_s(v)})', [0x4B, v & 0xFF])
def SETTRA(v):      return (f'ATM_SET_TRA({_s(v)})', [0x4C, v & 0xFF])
def TREM(d, r):     return (f'ATM_TREM({d}, {r})', [0x4E, d, r])
TREMOFF = ('ATM_TREM_OFF', [0x4F])
def VIB(d, r):      return (f'ATM_VIB({d}, {r})', [0x50, d, r])
VIBOFF = ('ATM_VIB_OFF', [0x51])
def GLIS(v):        return (f'ATM_GLIS(0x{v:02X})', [0x52, v])
GLISOFF = ('ATM_GLIS_OFF', [0x53])
def TEMPOc(v):      return (f'ATM_SET_TEMPO({v})', [0x9D, v])
def RET():          return ('ATM_RETURN', [0xFE])
def GOTO(t):        return ('GOTO', t)
def REPEAT(r, t):   return ('REPEAT', r, t)


TR, ORDER = {}, []


def track(name, cmds):
    assert name not in TR, name
    TR[name] = cmds
    ORDER.append(name)


def n_(name, d):
    return [NOTE(name), DELAY(d)]


def seq(*items):
    """items: names -> GOTO, ints -> ADDTRA"""
    out = []
    for it in items:
        out.append(ADDTRA(it) if isinstance(it, int) else GOTO(it))
    return out


# ============================================================ entries (track 0 = CH3 drums entry)
# entry = intro once, then the loop track.  loop track = body, then itself.
track('drums',  [TEMPOc(TEMPO), GOTO('d_intro'), GOTO('drums_loop')])
track('lead',   [GOTO('l_intro'), GOTO('lead_loop')])
track('chords', [GOTO('c_intro'), GOTO('chords_loop')])
track('bass',   [GOTO('b_intro'), GOTO('bass_loop')])
track('lead_loop',   [GOTO('l_body'), GOTO('lead_loop')])
track('chords_loop', [GOTO('c_body'), GOTO('chords_loop')])
track('bass_loop',   [GOTO('b_body'), GOTO('bass_loop')])
track('drums_loop',  [GOTO('d_body'), GOTO('drums_loop')])

# ============================================================ LEAD (CH0 PULSE)
# -- chorus hook, C pentatonic; transposed per chord by the phrase tracks (C +0, G +7, Am +0, F +5)
track('hookA', n_('G5', 8) + n_('G5', 4) + n_('A5', 4) + n_('G5', 8) + n_('E5', 8) + n_('D5', 8) + n_('E5', 8) + n_('G5', 16) + [RET()])
track('hookB', n_('A5', 8) + n_('A5', 4) + n_('C6', 4) + n_('A5', 8) + n_('G5', 8) + n_('E5', 8) + n_('G5', 8) + n_('A5', 16) + [RET()])
track('hookC', n_('C6', 8) + n_('A5', 8) + n_('G5', 8) + n_('E5', 8) + n_('G5', 16) + n_('A5', 16) + [RET()])
track('hookE', n_('E5', 8) + n_('G5', 8) + n_('A5', 8) + n_('G5', 8) + n_('E5', 16) + n_('D5', 16) + [RET()])
track('ch_ph1', seq('hookA', 7, 'hookA', -7, 'hookB', 5, 'hookC', -5) + [RET()])
track('ch_ph2', seq('hookA', 7, 'hookA', -7, 'hookB', 5, 'hookE', -5) + [RET()])
# -- verse (absolute notes in the C pentatonic, lower register)
track('vA', n_('A4', 8) + n_('C5', 4) + n_('A4', 4) + n_('E5', 8) + n_('C5', 8) + n_('A4', 8) + n_('C5', 8) + n_('E5', 8) + n_('D5', 8) + [RET()])
track('vB', n_('C5', 8) + n_('A4', 4) + n_('C5', 4) + n_('E5', 8) + n_('D5', 8) + n_('C5', 8) + n_('A4', 8) + n_('G4', 8) + n_('A4', 8) + [RET()])
track('vC', n_('E5', 8) + n_('G5', 4) + n_('E5', 4) + n_('D5', 8) + n_('C5', 8) + n_('D5', 8) + n_('E5', 8) + n_('G5', 8) + n_('E5', 8) + [RET()])
track('vD', n_('D5', 8) + n_('E5', 4) + n_('D5', 4) + n_('C5', 8) + n_('A4', 8) + n_('G4', 8) + n_('A4', 8) + n_('C5', 8) + n_('D5', 8) + [RET()])
track('v_ph', seq('vA', 'vB', 'vC', 'vD') + [RET()])
# -- pre-chorus: one rising motif on F G F G (major pentatonic from F), +2 for the G bars
track('pm',    n_('F5', 8) + n_('G5', 8) + n_('A5', 8) + n_('C6', 8) + n_('A5', 8) + n_('C6', 8) + n_('D6', 16) + [RET()])
track('pmEnd', n_('F5', 8) + n_('G5', 8) + n_('A5', 8) + n_('C6', 8) + n_('D6', 32) + [RET()])
track('pre_ph', seq('pm', 2, 'pm', -2, 'pm', 2, 'pmEnd', -2) + [RET()])
# -- dance break: 16th riff, root/5th/octave/9th so it survives transposition (Am F C G = 0 -4 +3 -2)
track('cell',  n_('A5', 4) + n_('A5', 4) + n_('E5', 4) + n_('A5', 4) + [RET()])
track('cellB', n_('A5', 4) + n_('E5', 4) + n_('A5', 4) + n_('B5', 4) + [RET()])
track('bar_d', [REPEAT(2, 'cell'), GOTO('cellB'), RET()])
track('dance_ph', seq('bar_d', -4, 'bar_d', 7, 'bar_d', -5, 'bar_d', 2) + [RET()])
# -- bridge: long vibrato notes, then a rising glissando riser
track('br_a', (n_('C6', 32) + n_('A5', 32) + n_('B5', 32) + n_('D6', 32) +
               n_('G5', 32) + n_('B5', 32) + n_('A5', 64) + [RET()]))
track('br_b', (n_('A5', 16) + n_('C6', 16) + n_('F6', 32) + n_('D6', 16) + n_('B5', 16) + n_('G5', 32) +
               n_('B5', 16) + n_('E6', 16) + n_('G6', 32) +
               [NOTE('E5'), GLIS(0x01), DELAY(32), GLISOFF, DELAY(32), RET()]))

track('l_intro', [DELAY(64)] * 4 + [VOL(26), SLV(-1), SETTRA(0), GOTO('ch_ph1'), RET()])
body_lead = [
    SETTRA(0),
    VOL(36), SLV(-1), REPEAT(1, 'v_ph'),                                    # verse 1
    VOL(40), GOTO('pre_ph'),                                                # pre 1
    VOL(46), GOTO('ch_ph1'), GOTO('ch_ph2'),                                # chorus 1
    VOL(40), SLV(-1), GOTO('dance_ph'),                                     # dance break 1
    VOL(36), ADDTRA(12), REPEAT(1, 'v_ph'), ADDTRA(-12),                    # verse 2 (octave up)
    VOL(40), GOTO('pre_ph'),                                                # pre 2
    VOL(46), GOTO('ch_ph1'), GOTO('ch_ph2'),                                # chorus 2
    VOL(40), GOTO('dance_ph'),                                              # dance break 2
    VOL(36), SLV(-1), VIB(3, 3), GOTO('br_a'), GOTO('br_b'), VIBOFF,        # bridge
    VOL(34), REPEAT(1, 'v_ph'),                                             # verse 3
    VOL(40), GOTO('pre_ph'),                                                # pre 3
    VOL(46), ADDTRA(2), REPEAT(1, 'ch_pair'), ADDTRA(-2),                   # last chorus, a whole step higher, 2 x (ph1+ph2)
    RET(),
]
track('ch_pair', [GOTO('ch_ph1'), GOTO('ch_ph2'), RET()])
track('l_body', body_lead)

# ============================================================ CHORDS (CH1 SQUARE)
CH = {'C': ('C4', 0x43), 'G': ('G3', 0x43), 'Am': ('A3', 0x34), 'F': ('F3', 0x43), 'Em': ('E3', 0x34),
      'D': ('D4', 0x43), 'A': ('A3', 0x43), 'Bm': ('B3', 0x34)}


def stab(name):
    r, a = CH[name]
    return [ARP(a, 0x20)] + n_(r, 12) + n_(r, 12) + n_(r, 8) + n_(r, 12) + n_(r, 12) + n_(r, 8) + [RET()]


def pad(name):
    r, a = CH[name]
    return [ARP(a, 0x22), NOTE(r), DELAY(64), RET()]


for nm in ['C', 'G', 'Am', 'F', 'D', 'A', 'Bm']:
    track('st_' + nm, stab(nm))
for nm in ['C', 'G', 'Am', 'F', 'Em']:
    track('pd_' + nm, pad(nm))
track('ph_pd_intro',  seq('pd_C', 'pd_G', 'pd_Am', 'pd_F') + [RET()])
track('ph_pd_verse',  seq('pd_Am', 'pd_F', 'pd_C', 'pd_G') + [RET()])
track('ph_pd_bridge', seq('pd_F', 'pd_G', 'pd_Em', 'pd_Am') + [RET()])
track('ph_st_pre',    seq('st_F', 'st_G', 'st_F', 'st_G') + [RET()])
track('ph_st_chor',   seq('st_C', 'st_G', 'st_Am', 'st_F') + [RET()])
track('ph_st_dance',  seq('st_Am', 'st_F', 'st_C', 'st_G') + [RET()])
track('ph_st_lift',   seq('st_D', 'st_A', 'st_Bm', 'st_G') + [RET()])

track('c_intro', [VOL(0), SLVADV(1, 17), GOTO('ph_pd_intro'), GOTO('ph_pd_intro'), RET()])
track('c_body', [
    VOL(16), SLV(0), REPEAT(1, 'ph_pd_verse'),                          # verse 1
    VOL(24), SLV(-2), GOTO('ph_st_pre'),                                # pre 1
    REPEAT(1, 'ph_st_chor'),                                            # chorus 1
    GOTO('ph_st_dance'),                                                # dance 1
    VOL(16), SLV(0), REPEAT(1, 'ph_pd_verse'),                          # verse 2
    VOL(24), SLV(-2), GOTO('ph_st_pre'),                                # pre 2
    REPEAT(1, 'ph_st_chor'),                                            # chorus 2
    GOTO('ph_st_dance'),                                                # dance 2
    VOL(16), SLV(0), REPEAT(1, 'ph_pd_bridge'),                         # bridge
    REPEAT(1, 'ph_pd_verse'),                                           # verse 3
    VOL(24), SLV(-2), GOTO('ph_st_pre'),                                # pre 3
    REPEAT(3, 'ph_st_lift'),                                            # lifted chorus (16 bars)
    RET(),
])

# ============================================================ BASS (CH2 SAW), transposed per chord
track('bassV', n_('C3', 12) + n_('C3', 4) + n_('G3', 8) + n_('C3', 8) + n_('C3', 12) + n_('C3', 4) + n_('C4', 8) + n_('G3', 8) + [RET()])
track('bassC', n_('C3', 8) + n_('C3', 8) + n_('C4', 8) + n_('C3', 8) + n_('C3', 8) + n_('C3', 8) + n_('G3', 8) + n_('C4', 8) + [RET()])
track('bassB', n_('C3', 32) + n_('G3', 16) + n_('C3', 16) + [RET()])
track('bv_ph',  seq(-3, 'bassV', -4, 'bassV', 7, 'bassV', -5, 'bassV', 5) + [RET()])         # Am F C G
track('bp_ph',  seq(-7, 'bassC', 2, 'bassC', -2, 'bassC', 2, 'bassC', 5) + [RET()])          # F G F G
track('bc_ph',  seq('bassC', -5, 'bassC', 2, 'bassC', -4, 'bassC', 7) + [RET()])             # C G Am F
track('bd_ph',  seq(-3, 'bassC', -4, 'bassC', 7, 'bassC', -5, 'bassC', 5) + [RET()])         # Am F C G
track('bb_ph',  seq(-7, 'bassB', 2, 'bassB', -3, 'bassB', 5, 'bassB', 3) + [RET()])          # F G Em Am

track('b_intro', [DELAY(64)] * 4 + [VOL(63), SLV(-3), SETTRA(0), GOTO('bc_ph'), RET()])
track('b_body', [
    SETTRA(0), VOL(63), SLV(-3),
    REPEAT(1, 'bv_ph'),                                                # verse 1
    GOTO('bp_ph'),                                                     # pre 1
    REPEAT(1, 'bc_ph'),                                                # chorus 1
    GOTO('bd_ph'),                                                     # dance 1
    REPEAT(1, 'bv_ph'),                                                # verse 2
    GOTO('bp_ph'),                                                     # pre 2
    REPEAT(1, 'bc_ph'),                                                # chorus 2
    GOTO('bd_ph'),                                                     # dance 2
    SLV(-1), REPEAT(1, 'bb_ph'),                                       # bridge
    SLV(-3), REPEAT(1, 'bv_ph'),                                       # verse 3
    GOTO('bp_ph'),                                                     # pre 3
    ADDTRA(2), REPEAT(3, 'bc_ph'), ADDTRA(-2),                         # lifted chorus
    RET(),
])

# ============================================================ DRUMS (CH3 NOISE)
track('k4',  [VOL(48), SLV(-12), DELAY(4), RET()])
track('s4',  [VOL(44), SLV(-5), DELAY(4), RET()])
track('h4',  [VOL(14), SLV(-7), DELAY(4), RET()])
track('o4',  [VOL(26), SLV(-3), DELAY(4), RET()])
track('b1',  [GOTO('k4'), REPEAT(2, 'h4'), RET()])                 # K h h h
track('b2',  [GOTO('s4'), REPEAT(1, 'h4'), GOTO('k4'), RET()])     # S h h K
track('b3',  [GOTO('k4'), GOTO('h4'), GOTO('o4'), GOTO('h4'), RET()])   # K h O h
track('b4',  [GOTO('s4'), GOTO('h4'), GOTO('k4'), GOTO('h4'), RET()])   # S h K h
track('bS',  [GOTO('s4'), REPEAT(2, 'h4'), RET()])                 # S h h h
track('bhh', [REPEAT(3, 'h4'), RET()])                             # h h h h
track('bf',  [GOTO('k4'), GOTO('h4'), GOTO('k4'), GOTO('h4'), RET()])   # four on the floor, K h K h
track('roll16', [VOL(10), SLV(2), TREM(6, 0), DELAY(16), TREMOFF, RET()])
track('roll64', [VOL(10), SLVADV(1, 1), TREM(6, 0), DELAY(64), TREMOFF, RET()])
track('bar_g',   [GOTO('b1'), GOTO('b2'), GOTO('b3'), GOTO('b4'), RET()])
track('bar_gf',  [GOTO('b1'), GOTO('b2'), GOTO('b3'), GOTO('roll16'), RET()])
track('bar_v',   [GOTO('b1'), GOTO('bS'), GOTO('b1'), GOTO('bS'), RET()])
track('bar_vf',  [GOTO('b1'), GOTO('bS'), GOTO('b1'), GOTO('roll16'), RET()])
track('bar_hat', [REPEAT(3, 'bhh'), RET()])
track('bar_df',  [REPEAT(3, 'bf'), RET()])
track('bar_dfill', [REPEAT(2, 'bf'), GOTO('roll16'), RET()])
track('bar_br',  [GOTO('b1'), REPEAT(2, 'bhh'), RET()])
track('dr4v', [REPEAT(2, 'bar_v'), GOTO('bar_vf'), RET()])
track('dr4g', [REPEAT(2, 'bar_g'), GOTO('bar_gf'), RET()])
track('dr4d', [REPEAT(2, 'bar_df'), GOTO('bar_dfill'), RET()])
track('dr_pre', [REPEAT(2, 'bar_g'), GOTO('roll64'), RET()])
track('dr_br',  [REPEAT(5, 'bar_br'), GOTO('bar_g'), GOTO('roll64'), RET()])

track('d_intro', [REPEAT(3, 'bar_hat'), REPEAT(2, 'bar_g'), GOTO('bar_gf'), RET()])
track('d_body', [
    REPEAT(1, 'dr4v'),                                    # verse 1
    GOTO('dr_pre'),                                       # pre 1
    REPEAT(1, 'dr4g'),                                    # chorus 1
    GOTO('dr4d'),                                         # dance 1
    REPEAT(1, 'dr4v'),                                    # verse 2
    GOTO('dr_pre'),                                       # pre 2
    REPEAT(1, 'dr4g'),                                    # chorus 2
    GOTO('dr4d'),                                         # dance 2
    GOTO('dr_br'),                                        # bridge
    REPEAT(1, 'dr4v'),                                    # verse 3
    GOTO('dr_pre'),                                       # pre 3
    REPEAT(3, 'dr4g'),                                    # lifted chorus (16 bars)
    RET(),
])


CH_ENTRY = ['lead', 'chords', 'bass', 'drums']
assert ORDER[0] == 'drums'


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
    song = [len(ORDER)]
    for o in offs:
        song += [o & 0xFF, o >> 8]
    song += [idx[e] for e in CH_ENTRY]
    for b in blobs:
        song += b
    return song, idx, offs, blobs


HEADER = '''#ifndef STARLIGHT_H
#define STARLIGHT_H

// ---------------------------------------------------------------------------
//  "Star Light" - an original K-pop style dance track for ATMlib          {total} bytes
//
//  Play with:   #include "starlight.h"      ATM.play(starLight);
//
//  NEVER ENDS: an 8-bar intro plays once, then the 84-bar body (about 2:50) loops forever.
//  The first pass is intro + body = 92 bars = about 3:00.  There is no ending.
//
//  Timing : ATM_SET_TEMPO({tempo}) (doubled) = about 124 BPM.  1 beat = 16 ticks, 1 bar = 64 ticks.
//  Loop   : every channel's entry track plays the intro once and then calls its LOOP track, which
//           calls the body and then calls ITSELF (ATM_GOTO to the track it is already in).  That
//           restarts in the same tick: no STOP, no ATM_GOTO_ADV, no silent tick, no hop.
//           Every channel's body is exactly 5376 ticks (84 bars) of delay.
//           The body sets volume, slide, arpeggio and transposition itself, so every pass is identical.
//
//  FORM                                                            bars
//     INTRO (once)   pad fades in, hook teaser, groove builds, roll   8
//     verse 1        Am F C G x2   (absolute melody, sparse bass)     8
//     pre-chorus 1   F G F G       (one rising motif, transposed)     4
//     chorus 1       C G Am F x2   (hook transposed per chord)        8
//     dance break 1  Am F C G      (16th riff, transposed per chord)  4
//     verse 2        same, melody an octave higher (ADD_TRA 12)       8
//     pre-chorus 2                                                    4
//     chorus 2                                                        8
//     dance break 2                                                   4
//     bridge         F G Em Am x2  (vibrato, slow pad, riser)         8
//     verse 3                                                         8
//     pre-chorus 3                                                    4
//     last chorus    D A Bm G x4   (key change: a whole step up)     16
//                    ends on G, which is chord V of the verse key: it falls back into Am
//
//  Channels (ATMlib defaults):
//     CH0 PULSE  - lead.  The hook, the pre-chorus motif and the dance riff are ONE track each
//                  and are transposed per chord with ATM_ADD_TRA; the key change is SET at the start.
//     CH1 SQUARE - arpeggio chords: slow pad (verse, bridge, intro) and 3+3+2 stabs (pre, chorus,
//                  dance).  Explicit roots (arpeggio + transposition on one channel would transpose twice).
//     CH2 SAW    - bass, transposed per chord, three patterns (sparse, driving, sustained)
//     CH3 NOISE  - dance groove, four-on-the-floor in the dance breaks, tremolo snare-roll fills
//  Note   : track 0 is only CH3's entry point and is never called from another track.
// ---------------------------------------------------------------------------

#include <ATMcmds.h>

#ifndef Song
#define Song const uint8_t PROGMEM
#endif


'''


def render_text(idx, offs, blobs, total):
    L = [HEADER.format(total=total, tempo=TEMPO), f'Song {SONG_VAR}[] = {{     // total song bytes = {total}',
         f'  0x{len(ORDER):02X},                       // Number of tracks']
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


I_BARS, B_BARS = 8, 84
SECTIONS = [('verse1', 8), ('pre1', 4), ('chorus1', 8), ('dance1', 4), ('verse2', 8), ('pre2', 4), ('chorus2', 8),
            ('dance2', 4), ('bridge', 8), ('verse3', 8), ('pre3', 4), ('last chorus', 16)]
assert sum(b for _, b in SECTIONS) == B_BARS

if __name__ == '__main__':
    song, idx, offs, blobs = assemble()
    total = len(song)
    print(f'tracks={len(ORDER)} total={total} bytes')
    I, B = I_BARS * 64, B_BARS * 64
    ev, restarts, stops, maxstack, tempo = simulate(song, I + B * 3 + 40)
    TL = atmsim.TIMELINE
    ok = True
    print('tempo', tempo, '| max call depth', maxstack)
    if restarts or [s for s in stops if s[0] > 0]:
        print('FAIL: a channel stopped or the song restarted (it must self-loop)'); ok = False
    if maxstack > 7:
        print('FAIL: call stack deeper than 7'); ok = False
    nz = lambda r: [(v, f if v else 0) for v, f in r]       # pitch of a silent channel is irrelevant
    # pass 1 and pass 2 must be identical tick by tick.  (Pass 0 may differ for a few ticks in the chord volume:
    # the intro's SL_VOL_ADV leaves its slide counter running.  Its note events are identical, see below.)
    for t in range(I + 2 * B, I + 3 * B):
        if nz(TL[t]) != nz(TL[t - B]):
            print(f'FAIL: tick {t} differs from tick {t - B}'); ok = False; break

    def body(k):
        a = I + B * k
        return [(e[0] - a,) + e[1:] for e in ev if a <= e[0] < a + B]
    if not (body(0) == body(1) == body(2)):
        print('FAIL: body passes differ (state leaking across the loop)'); ok = False
    else:
        print(f'body pass 0 == pass 1 == pass 2 ({len(body(0))} events each, period {B} ticks)')
    for n in range(3):
        ns = [e[3] for e in ev if e[1] == n and e[2] == 'note' and e[3] > 0]
        print(f'ch{n} note range {min(ns)}..{max(ns)}  ({note_name(min(ns))}..{note_name(max(ns))})')
        if min(ns) < 1 or max(ns) > 63:
            ok = False
    vmax = max(t[n][0] for t in TL for n in range(4))
    print('vol max', vmax)
    if vmax > 63:
        ok = False
    ev0 = body(0)
    bar = 0
    print()
    for nm, nb in SECTIONS:
        rows = []
        for b in range(bar, bar + nb):
            def first(ch):
                es = [e for e in ev0 if e[1] == ch and e[2] == 'note' and e[0] // 64 == b and e[3] > 0]
                return note_name(es[0][3]) if es else '--'
            rows.append(f'{first(0):>3}/{first(1):>3}/{first(2):>3}')
        print(f'{nm:12s} ' + ' '.join(rows))
        bar += nb
    print('(lead / chord-root / bass, first note of each bar)')
    print('\nOK' if ok else '\nPROBLEMS')
    if ok:
        open('./starlight.bin', 'wb').write(bytes(song))
        open('./starlight.h', 'w').write(render_text(idx, offs, blobs, total))
        print('wrote ./starlight.h and ./starlight.bin')
```

---

## Appendix F: `convert.py` (old STOP + GOTO_ADV loop to self-loop)

Turns a `song.h` written in the old style into the hop-free style. It works on the text (so comments, track names and the layout of the file survive), computes everything from the simulator and the real `ATMcmds.h`, and refuses to write a result that does not verify. After converting, re-read the preamble comment: it may still say "gapless via 511 ticks + restart tick" (the converter does not rewrite prose).

```bash
cd /path/with/atmsim.py
ATM_SRC=/path/to/ATMlib/src python3 convert.py song_old.h song_new.h
```

```python
#!/usr/bin/env python3
"""
convert.py - convert an old ATMlib loop (ATM_STOP_CHAN + ATM_GOTO_ADV, one silent tick per loop) to the
hop-free self-loop (every channel's entry track ends with ATM_GOTO(itself)).

Usage:   ATM_SRC=/path/to/ATMlib/src python3 convert.py old_song.h new_song.h
Needs:   g++, atmsim.py in the current folder.  The song.h must have one ATM_* command per line and
         '//"Track N"' or '// Track N:' marker lines (every file this project wrote does).
What it does
  1. compiles the old header against the real ATMcmds.h and checks its own parse reproduces the bytes
  2. simulates the old song to the first restart; finds, for each channel, the LAST delay it executed
     and the STOP in its entry track
  3. last delay + 1 (the old loop was L-1 ticks of delay + 1 restart tick), STOP -> ATM_GOTO(entry),
     deletes ATM_GOTO_ADV
  4. merges tracks that became identical (the old '_last' variants), renumbers, rewrites the address table
  5. verifies: no stop, no restart, same notes as the old first loop, timeline repeats with period L
"""
import re, subprocess, sys, os, tempfile
sys.path.insert(0, '.')
import atmsim

# Folder with ATMcmds.h (the real library header).  Run from the folder that holds atmsim.py.
SRC = os.environ.get('ATM_SRC', '../src')

def run_dump(cpp, tag):
    with tempfile.TemporaryDirectory() as d:
        src = f'{d}/a.cpp'
        open(src, 'w').write(cpp)
        r = subprocess.run(['g++', '-include', 'stdint.h', '-DPROGMEM=', f'-I{SRC}', src, '-o', f'{d}/a'],
                           capture_output=True, text=True)
        if r.returncode:
            raise Exception(r.stderr[:2000])
        return subprocess.run([f'{d}/a'], capture_output=True, text=True).stdout


def compile_song(path, name):
    cpp = f'#include "{os.path.abspath(path)}"\n#include <stdio.h>\nint main(){{for(unsigned i=0;i<sizeof({name});i++)printf("%d ",{name}[i]);}}'
    return [int(x) for x in run_dump(cpp, name).split()]

def compile_lines(lines):
    """each line = one ATM_ command text (no comment, trailing comma ok). returns list of byte lists."""
    cpp = '#include <ATMcmds.h>\n#include <stdio.h>\n#define Song const uint8_t\n'
    for i, l in enumerate(lines):
        cpp += f'static const uint8_t L{i}[] = {{ {l.rstrip().rstrip(",")} }};\n'
    cpp += 'int main(){\n'
    for i in range(len(lines)):
        cpp += f'printf("%d:", (int)sizeof(L{i})); for(unsigned k=0;k<sizeof(L{i});k++)printf(" %d", L{i}[k]); printf("\\n");\n'
    cpp += '}\n'
    out = run_dump(cpp, 'lines').strip().split('\n')
    res = []
    for o in out:
        a, b = o.split(':')
        res.append([int(x) for x in b.split()])
    return res

def code_of(line):
    return re.sub(r'//.*', '', line).strip()

MARK = re.compile(r'^\s*//\s*"?Track\s+(\d+)')

class Song:
    pass

def parse_file(path):
    L = open(path).read().split('\n')
    si = next(i for i, l in enumerate(L) if re.match(r'\s*Song\s+\w+\[\]', l))
    name = re.match(r'\s*Song\s+(\w+)\[\]', L[si]).group(1)
    ntr = int(re.match(r'\s*0x([0-9A-Fa-f]{2})', L[si + 1]).group(1), 16)
    s = Song(); s.name = name; s.pre = L[:si]; s.ntrline = L[si + 1]; s.songline = L[si]; s.ntr = ntr
    s.addr = L[si + 2: si + 2 + ntr]
    i = si + 2 + ntr
    while not re.match(r'\s*0x[0-9A-F]{2},\s*//', L[i]): i += 1
    s.mid = L[si + 2 + ntr:i]          # blank lines between table and entries
    s.entl = L[i:i + 4]
    i += 4
    end = next(k for k in range(i, len(L)) if L[k].startswith('};'))
    s.tail = L[end:]
    body = L[i:end]
    # split to blocks
    s.blocks = []   # dict: head(lines before cmds incl marker), cmds[(text)], trail(lines)
    cur = None
    for l in body:
        m = MARK.match(l)
        if m:
            cur = dict(idx=int(m.group(1)), head=[l], cmds=[], trail=[])
            s.blocks.append(cur)
        elif cur is None:
            s.pre_body = getattr(s, 'pre_body', []) + [l]
        elif code_of(l).startswith('ATM_'):
            assert not cur['trail'] or True
            if cur['trail']:       # command after trailing -> treat trailing as part of block? keep simple
                raise Exception('cmd after trail: ' + l)
            cur['cmds'].append(l)
        else:
            if cur['cmds']: cur['trail'].append(l)
            else: cur['head'].append(l)
    assert [b['idx'] for b in s.blocks] == list(range(ntr)), 'track marker order'
    return s

def blocks_bytes(s):
    allc = [code_of(c) for b in s.blocks for c in b['cmds']]
    bs = compile_lines(allc)
    k = 0
    for b in s.blocks:
        b['bytes'] = bs[k:k + len(b['cmds'])]; k += len(b['cmds'])

def name_of(b):
    m = re.search(r'Track\s+\d+"?:?\s+(\w+)', b['head'][0])
    return m.group(1)

def convert(path, outpath):
    s = parse_file(path)
    orig = compile_song(path, s.name)
    blocks_bytes(s)
    flat = [x for b in s.blocks for c in b['bytes'] for x in c]
    ntr, offs, entries, base = atmsim.parse(orig)
    assert orig[base:] == flat, 'body bytes mismatch'
    # offset -> (block, cmd idx)
    where = {}
    for b in s.blocks:
        p = base + offs[b['idx']]
        for ci, c in enumerate(b['bytes']):
            where[p] = (b['idx'], ci); p += len(c)
    # simulate original
    ev0, restarts, stops, mx, tempo = atmsim.simulate(orig, 3000)
    T = restarts[0][0]
    P = T + 1
    tl0 = [list(t) for t in atmsim.TIMELINE]
    dl = [d for d in atmsim.DELAYLOG if d[0] < T + 1]
    stp = [x for x in atmsim.STOPLOG if x[0] <= T]
    orig_period_events = [e for e in ev0 if e[0] < P]
    print(f'{s.name}: bytes={len(orig)} first restart at tick {T} tempo={tempo}')
    edits = {}
    for n in range(4):
        last = [d for d in dl if d[1] == n]
        lastptr = last[-1][2] if last else None
        sp = [x for x in stp if x[1] == n]
        if not sp:
            print('  ch', n, 'never stops'); continue
        sptr = sp[0][2]
        cnt = sum(1 for d in dl if d[2] == lastptr) if lastptr is not None else 0
        sb, sc = where[sptr]
        e = entries[n]
        print(f'  ch{n}: entry {e} stop in track {sb} (cmd {sc}), last delay in track {where[lastptr][0] if lastptr else None}, exec count {cnt}')
        assert sb == e, 'stop not in entry track'
        edits[n] = (lastptr, (sb, sc), e, cnt)
    # apply edits
    for n, (lp, (sb, sc), e, cnt) in edits.items():
        assert cnt == 1, f'last delay of ch{n} executed {cnt}x'
        tb, tc = where[lp]
        t = s.blocks[tb]['cmds'][tc]
        m = re.search(r'ATM_DELAY\((\d+)\)', t)
        d = int(m.group(1)); assert d < 64
        s.blocks[tb]['cmds'][tc] = t.replace(m.group(0), f'ATM_DELAY({d + 1})') + ''
        if '//' not in t: s.blocks[tb]['cmds'][tc] = s.blocks[tb]['cmds'][tc]
        # stop -> goto entry
        old = s.blocks[sb]['cmds'][sc]
        assert code_of(old).startswith('ATM_STOP_CHAN')
        ind = re.match(r'\s*', old).group(0)
        s.blocks[sb]['cmds'][sc] = f'{ind}ATM_GOTO({e}),   // loop: restart this track (no STOP, no gap)'
    # remove ADV
    for b in s.blocks:
        b['cmds'] = [c for c in b['cmds'] if not code_of(c).startswith('ATM_GOTO_ADV')]
    # merge duplicates
    names = {b['idx']: name_of(b) for b in s.blocks}
    entries = list(entries)
    def refs_re():
        return re.compile(r'ATM_(GOTO|REPEAT)\((?:(\d+)(\s*,\s*))?(\d+)\)')
    def sig(b, alive):
        out = []
        for c in b['cmds']:
            out.append(re.sub(r'\s*//.*', '', c).strip())
        return tuple(out)
    merged = []
    while True:
        sigs = {}
        found = None
        for b in s.blocks:
            if b['idx'] == 0: continue
            sg = sig(b, None)
            if sg in sigs:
                found = (sigs[sg], b['idx']); break
            sigs[sg] = b['idx']
        if not found: break
        keep, rem = found
        merged.append((names[rem], names[keep]))
        s.blocks = [b for b in s.blocks if b['idx'] != rem]
        def fix(i):
            i = keep if i == rem else i
            return i - 1 if i > rem else i
        rx = refs_re()
        for b in s.blocks:
            nc = []
            for c in b['cmds']:
                def sub(m):
                    a, tr = m.group(2), int(m.group(4))
                    nt = fix(tr)
                    return f'ATM_{m.group(1)}(' + (f'{a}{m.group(3)}' if a is not None else '') + f'{nt})'
                c2 = rx.sub(sub, c)
                if c2 != c:
                    c2 = re.sub(r'-> ' + re.escape(names[rem]) + r'\b', '-> ' + names[keep], c2)
                    c2 = re.sub(r'(\d+x )' + re.escape(names[rem]) + r'\b', r'\g<1>' + names[keep], c2)
                nc.append(c2)
            b['cmds'] = nc
        entries = [fix(x) for x in entries]
        # renumber
        oldidx = [b['idx'] for b in s.blocks]
        for b in s.blocks:
            b['newidx'] = fix(b['idx'])
        for b in s.blocks:
            b['idx'] = b['newidx']
            def rn(m, b=b):
                tot = len(m.group(2)) + len(m.group(3)); d = str(b['idx'])
                return m.group(1) + ' ' * max(1, tot - len(d)) + d
            b['head'][0] = re.sub(r'(Track)(\s+)(\d+)', rn, b['head'][0], count=1)
        names = {b['idx']: name_of(b) for b in s.blocks}
        # re-align escaper style "Track  1:" widths
    # recompute bytes
    blocks_bytes(s)
    for b in s.blocks:
        sz = sum(len(c) for c in b['bytes'])
        b['head'][0] = re.sub(r'\[\d+b\]', f'[{sz}b]', b['head'][0])
    ntr2 = len(s.blocks)
    offs2 = []; p = 0
    for b in s.blocks:
        offs2.append(p); p += sum(len(c) for c in b['bytes'])
    total = 1 + 2 * ntr2 + 4 + p
    # build text
    out = list(s.pre)
    # preamble sizes unchanged here; handled later
    sl = re.sub(r'total song bytes = \d+', f'total song bytes = {total}', s.songline)
    out.append(sl)
    out.append(re.sub(r'0x[0-9A-Fa-f]{2}', f'0x{ntr2:02X}', s.ntrline, count=1))
    fmtB = 'Track' in s.addr[0] and '@' in s.addr[0]
    for i, b in enumerate(s.blocks):
        nm = names[i]
        o = offs2[i]
        if fmtB:
            cm = f'// Track {i:<2} @ {o:>4}  ({nm})'
        else:
            cm = f'// Address of track {i:<2} {o:>5}   {nm}'
        out.append(f'  0x{o & 255:02X}, 0x{o >> 8:02X},                 {cm}')
    out.extend(s.mid)
    for n in range(4):
        out.append(re.sub(r'0x[0-9A-F]{2}', f'0x{entries[n]:02X}', s.entl[n], count=1))
    # entry comment track numbers
    for k in range(4):
        out[-4 + k] = re.sub(r'(track )(\d+)', lambda m: m.group(1) + str(entries[k]), out[-4 + k])
    out.extend(getattr(s, 'pre_body', []))
    for b in s.blocks:
        out.extend(b['head']); out.extend(b['cmds']); out.extend(b['trail'])
    out.extend(s.tail)
    open(outpath, 'w').write('\n'.join(out))
    print('  merged:', merged)
    # verify
    new = compile_song(outpath, s.name)
    assert len(new) == total, (len(new), total)
    ev1, rs1, st1, mx1, tempo1 = atmsim.simulate(new, P * 3)
    tl1 = [list(t) for t in atmsim.TIMELINE]
    st_bad = [x for x in st1 if x[0] > 0]
    assert not rs1, 'restarts'
    assert not st_bad, st_bad
    nz = lambda r: [(v, f if v else 0) for v, f in r]
    for t in range(P, 3 * P):
        assert nz(tl1[t]) == nz(tl1[t - P]), f"timeline differs at {t}"
    for t in range(T):
        assert tl1[t] == tl0[t], f'new != orig at {t}: {tl1[t]} {tl0[t]}'
    e0 = [(a, b, c, d, e) for (a, b, c, d, e, *_) in orig_period_events if c == 'note']
    e1 = [(a, b, c, d, e) for (a, b, c, d, e, *_) in ev1 if a < P and c == 'note']
    assert e0 == e1, 'note events differ'
    print(f'  OK  {len(orig)} -> {total} bytes, period {P} ticks, max stack {mx1}, tempo {tempo1}')
    return total

if __name__ == '__main__':
    convert(sys.argv[1], sys.argv[2])
```
