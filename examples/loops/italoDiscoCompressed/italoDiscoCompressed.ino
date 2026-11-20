// Plays on the Arduboy (with the ARDUBOY intro and T-arg logo) AND on an Arduino Uno / Duemilanove (ATmega328P):
// on the Uno the song just plays. Speaker: pin 11 -> 100..220 ohm resistor -> speaker -> GND (a piezo can go straight on),
// or with ATM_ALT_WIRING 1 in ATMconfig.h between pin 11 and pin 3. Board: Arduino Uno or Duemilanove (ATmega328P).
#include <Arduino.h>
#include "ATMconfig.h"
#include <ATMlib.h>
#include "song.h"

ATMsynth ATM;

#if defined(__AVR_ATmega32U4__)

#include <Arduboy2.h>
#include "bitmaps.h"


Arduboy2Base arduboy;
Sprites sprites;

#include "arduboy_intro.h"          // the ARDUBOY letters, a note for every letter

byte introFrame = 0;

void setup() {
  arduboy.boot();
  // set the framerate of the game at 60 fps
  arduboy.setFrameRate(60);
  // let's make sure the sound was not muted in a previous sketch
  arduboy.audio.on();
}

void loop() {

  if (!(arduboy.nextFrame())) return;
  arduboy.pollButtons();
  arduboy.clear();
  if (introFrame < INTRO_FRAMES) arduboyIntro(introFrame++);        // ARDUBOY ...
  else
  {
    // ... then the T-arg logo and the song
    if (introFrame == INTRO_FRAMES)
    {
      ATM.play(italoDiscoCompressed);
      introFrame++;
    }
    sprites.drawSelfMasked(49, 20, T_arg, 0);
    if (arduboy.justPressed(A_BUTTON | B_BUTTON)) ATM.playPause();
  }
  arduboy.display();
}

#else   // Uno, Duemilanove, Nano (ATmega328P): no screen or buttons, just the music

void setup()
{
  ATM.play(italoDiscoCompressed);
}

void loop()
{
}

#endif
