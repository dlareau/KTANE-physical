#pragma once
#include "Arduino.h"

#define DOT_TIME 300

char freqs[16][4] = {
  "505",
  "515",
  "522",
  "532",
  "535",
  "542",
  "545",
  "552",
  "555",
  "565",
  "572",
  "575",
  "582",
  "592",
  "595",
  "600"
};

char words[16][7] = {
  "shell",
  "halls",
  "slick",
  "trick",
  "boxes",
  "leaks",
  "strobe",
  "bistro",
  "flick",
  "bombs",
  "break",
  "brick",
  "steak",
  "sting",
  "vector",
  "beats"
};

char morse[26][5] = {
  ".-",   // a
  "-...", // b
  "-.-.", // c
  "-..",  // d (not used)
  ".",    // e
  "..-.", // f
  "--.",  // g
  "....", // h
  "..",   // i
  ".---", // j (not used)
  "-.-",  // k
  ".-..", // l
  "--",   // m
  "-.",   // n
  "---",  // o
  ".--.", // p (not used)
  "--.-", // q (not used)
  ".-.",  // r
  "...",  // s
  "-",    // t
  "..-",  // u (not used)
  "...-", // v
  ".--",  // w (not used)
  "-..-", // x
  "-.--", // y (not used)
  "--.."  // z (not used)
};