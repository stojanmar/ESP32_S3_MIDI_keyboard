//Midi za AtomS3 na UnitMIDI od M5stack izbereš board M5stack-ATOMS3
// Prepared by Stojan Markic Aug 2025
// Ver01 kot starting vendar drugačno čitanje keyborda in hkrati več tipk
// Ver02 dodam igranje akordov z eno tipko
// Ver03 akordi na eno dodatno tipko kot kateri interval
// Ver04 izbira instrumenta z L in R arrow dodana in popravek grafike.
// Ver05 več akordov in instrumenti preset na 7 tipkah 0...6
// Ver06 Dodam velocity na master volumen
// Ver07 Dodam se akorda maj7 , m6 in +aug
#include "M5AtomS3.h"
#include "EspUsbHost.h"
#include "M5UnitSynth.h"

//#define KEYCODE_A 0x04 // USB HID code for 'a'

#define MAX_KEYS 6 // USB HID standard: max 6 simultaneous keys
#define BASE_OCTAVE 4

// Current states
int octaveShift = 0;
unsigned long timer = 0;
uint8_t chordMode = 0; // 0 = Major, 1 = Minor, 2 = 7, 3 = m7 ...
uint8_t currentInstrument = 0; // 0 to 127
uint8_t mastvolume = 107; // 0 to 127
// HID keycodes
#define KEY_UP     0x52
#define KEY_DOWN   0x51
#define KEY_LEFT   0x50
#define KEY_RIGHT  0x4F
#define KEY_Q      0x14
#define KEY_W      0x1A
#define KEY_E      0x08
#define KEY_R      0x15
#define KEY_T      0x17
#define KEY_Y      0x1C
#define KEY_U      0x18
#define KEY_I      0x0C
#define KEY_O      0x12
#define KEY_P      0x13
// Numpad keys for instant instrument change
#define KEY_NUM_0  0x62
#define KEY_NUM_1  0x59
#define KEY_NUM_2  0x5A
#define KEY_NUM_3  0x5B
#define KEY_NUM_4  0x5C
#define KEY_NUM_5  0x5D
#define KEY_NUM_6  0x5E
// Numpad keys for master volume
#define KEY_PGUP 0x4B
#define KEY_PGDOWN  0x4E

const uint8_t instantInstrumentKeys[] = {KEY_NUM_0, KEY_NUM_1, KEY_NUM_2, KEY_NUM_3, KEY_NUM_4, KEY_NUM_5, KEY_NUM_6};
const uint8_t instantInstruments[] = {0, 16, 33, 57, 73, 89, 48};

// Chord key definitions (keycode, root MIDI note without octave)
struct KeyChord {
  uint8_t keycode;
  uint8_t baseNote; // e.g. C = 0, C# = 1, ...
};

KeyChord chordKeys[] = {
  { 0x1D, 0 },  // 'z' = C
  { 0x1B, 2 },  // 'x' = D
  { 0x06, 4 },  // 'c' = E
  { 0x19, 5 },  // 'v' = F
  { 0x05, 7 },  // 'b' = G
  { 0x11, 9 },  // 'n' = A
  { 0x10, 11 }, // 'm' = B
  { 0x36, 12 },  // ',' = C2
  { 0x37, 14 },  // '.' = D2
  { 0x38, 16 }, // '-' = E2


  { 0x16, 1 },  // 's' = C#
  { 0x07, 3 },  // 'd' = D#
  { 0x0A, 6 },  // 'g' = F#
  { 0x0B, 8 },  // 'h' = G#
  { 0x0D, 10 }, // 'j' = A#
  { 0x0F, 13 }, // 'L' = C#2
  { 0x33, 15 }, // 'Č' = D#2
};

// Mode intervals
const int CHORD_INTERVALS[11][4] = {
  {0},       // Ena nota ni akord
  {0, 4, 7},       // dur
  {0, 3, 7},       // mol
  {0, 4, 7, 10},   // dur7
  {0, 3, 7, 10},    // m7
  {0, 4, 7, 9},    // 6
  {0, 2, 4, 10},    // 9
  {0, 3, 6, 9},    // dim7
  {0, 4, 7, 11},    // maj7
  {0, 3, 7, 9},    // m6
  {0, 4, 8}    // +aug
};
const uint8_t CHORD_LENGTH[11] = {1,3, 3, 4, 4,4,4,4,4,4,3};

// Track held chord keys
#define MAX_HELD 10
uint8_t heldChordKeys[MAX_HELD];
uint8_t heldChordCount = 0;

// Track last played notes per held key
struct ActiveChord {
  uint8_t keycode;
  uint8_t notes[4];
  uint8_t noteCount;
};

ActiveChord activeChords[MAX_HELD];
uint8_t activeChordCount = 0;

bool isInArray(uint8_t key, const uint8_t* arr) {
  for (int i = 0; i < MAX_KEYS; i++) {
    if (arr[i] == key) return true;
  }
  return false;
}

bool isChordKey(uint8_t keycode) {
  for (int i = 0; i < sizeof(chordKeys) / sizeof(KeyChord); i++) {
    if (chordKeys[i].keycode == keycode) return true;
  }
  return false;
}

void addHeldChordKey(uint8_t keycode) {
  if (heldChordCount >= MAX_HELD) return;
  for (int i = 0; i < heldChordCount; i++) {
    if (heldChordKeys[i] == keycode) return;
  }
  heldChordKeys[heldChordCount++] = keycode;
}

void removeHeldChordKey(uint8_t keycode) {
  for (int i = 0; i < heldChordCount; i++) {
    if (heldChordKeys[i] == keycode) {
      for (int j = i; j < heldChordCount - 1; j++) {
        heldChordKeys[j] = heldChordKeys[j + 1];
      }
      heldChordCount--;
      return;
    }
  }
}

M5Canvas canvas(&M5.Lcd);

//colors
unsigned short myblue=0x020A;
unsigned short arduinoCol=0x0B8D;
unsigned short gray=0xBDF7;
unsigned short darkGra=0x3186;
uint8_t note = 60;
int n=0;  //keycode for draw
//int ins=0;  //instrument

//int duration[6]={100,200,300,400,600,800};
//int dur=2; // chosen duration
M5UnitSynth synth;
unsigned long timePlayed=0; bool playing=false; bool found=0;

String instrumentNames[] = {
    "GrandPiano_1",
    "BrightPiano_2",
    "ElGrdPiano_3",
    "HonkyTonkPiano",
    "ElPiano1",
    "ElPiano2",
    "Harpsichord",
    "Clavi",
    "Celesta",
    "Glockenspiel",
    "MusicBox",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "TubularBells",
    "Santur",
    "DrawbarOrgan",
    "PercussiveOrgan",
    "RockOrgan",
    "ChurchOrgan",
    "ReedOrgan",
    "AccordionFrench",
    "Harmonica",
    "TangoAccordion",
    "AcGuitarNylon",
    "AcGuitarSteel",
    "AcGuitarJazz",
    "AcGuitarClean",
    "AcGuitarMuted",
    "OverdrivenGuitar",
    "DistortionGuitar",
    "GuitarHarmonics",
    "AcousticBass",
    "FingerBass",
    "PickedBass",
    "FretlessBass",
    "SlapBass1",
    "SlapBass2",
    "SynthBass1",
    "SynthBass2",
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "TremoloStrings",
    "PizzicatoStrings",
    "OrchestralHarp",
    "Timpani",
    "StringEnsemble1",
    "StringEnsemble2",
    "SynthStrings1",
    "SynthStrings2",
    "ChoirAahs",
    "VoiceOohs",
    "SynthVoice",
    "OrchestraHit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "MutedTrumpet",
    "FrenchHorn",
    "BrassSection",
    "SynthBrass1",
    "SynthBrass2",
    "SopranoSax",
    "AltoSax",
    "TenorSax",
    "BaritoneSax",
    "Oboe",
    "EnglishHorn",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "PanFlute",
    "BlownBottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Lead1Square",
    "Lead2Sawtooth",
    "Lead3Calliope",
    "Lead4Chiff",
    "Lead5Charang",
    "Lead6Voice",
    "Lead7Fifths",
    "Lead8BassLead",
    "Pad1Fantasia",
    "Pad2Warm",
    "Pad3PolySynth",
    "Pad4Choir",
    "Pad5Bowed",
    "Pad6Metallic",
    "Pad7Halo",
    "Pad8Sweep",
    "FX1Rain",
    "FX2Soundtrack",
    "FX3Crystal",
    "FX4Atmosphere",
    "FX5Brightness",
    "FX6Goblins",
    "FX7Echoes",
    "FX8SciFi",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "BagPipe",
    "Fiddle",
    "Shanai",
    "TinkleBell",
    "Agogo",
    "SteelDrums",
    "Woodblock",
    "TaikoDrum",
    "MelodicTom",
    "SynthDrum",
    "ReverseCymbal",
    "GtFretNoise",
    "BreathNoise",
    "Seashore",
    "BirdTweet",
    "TelephRing",
    "Helicopter",
    "Applause",
    "Gunshot"
};


void draw()
{
   canvas.fillScreen(BLACK);
   canvas.setTextColor(MAGENTA,BLACK);
   canvas.drawString("2025",100,6);
   canvas.setTextColor(gray,BLACK);
   canvas.drawString("S52UV Synth",10,2,2);
   canvas.setTextColor(ORANGE,BLACK);
   canvas.drawString("INST: "+String(currentInstrument),10,30,4);
   canvas.setTextColor(gray,BLACK);
   canvas.drawString(instrumentNames[currentInstrument],10,54,2);

   canvas.fillRect(8,20,116,4,darkGra);
   canvas.fillRect(6,77,68,50,myblue);
   canvas.fillRect(78,77,48,50,arduinoCol);
   canvas.setTextColor(WHITE,myblue);
   canvas.drawString("OKTAVA",10,80,2);
   canvas.drawString(String(octaveShift),10,100,4);
   
   canvas.setTextColor(WHITE,arduinoCol);
   canvas.drawString("AKORD",84,80,2);
   canvas.drawString(String(chordMode),84,100,4);
   canvas.pushSprite(0, 0);
    
}

class MyEspUsbHost : public EspUsbHost {
  // Main keyboard handler — called by the USB host when keyboard report updates
void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t last_report) {
  // Modifier key check for chord mode
  bool modeKeyQ = false, modeKeyW = false, modeKeyE = false, modeKeyR = false, modeKeyT = false, modeKeyY = false, modeKeyU = false;
  bool modeKeyI = false, modeKeyO = false, modeKeyP = false;
  // Check if mode keys are held
  for (int i = 0; i < MAX_KEYS; i++) {
    if (report.keycode[i] == KEY_Q) modeKeyQ = true;
    if (report.keycode[i] == KEY_W) modeKeyW = true;
    if (report.keycode[i] == KEY_E) modeKeyE = true;
    if (report.keycode[i] == KEY_R) modeKeyR = true;
    if (report.keycode[i] == KEY_T) modeKeyT = true;
    if (report.keycode[i] == KEY_Y) modeKeyY = true;
    if (report.keycode[i] == KEY_U) modeKeyU = true;
    if (report.keycode[i] == KEY_I) modeKeyI = true;
    if (report.keycode[i] == KEY_O) modeKeyO = true;
    if (report.keycode[i] == KEY_P) modeKeyP = true;
  }

  uint8_t newMode = 0; // Default to Single note or Major or....
  if (modeKeyQ) newMode = 1;
  else if (modeKeyW) newMode = 2;
  else if (modeKeyE) newMode = 3;
  else if (modeKeyR) newMode = 4;
  else if (modeKeyT) newMode = 5;
  else if (modeKeyY) newMode = 6;
  else if (modeKeyU) newMode = 7;
  else if (modeKeyI) newMode = 8;
  else if (modeKeyO) newMode = 9;
  else if (modeKeyP) newMode = 10;
  // Detect new key presses
  for (int i = 0; i < MAX_KEYS; i++) {
    uint8_t key = report.keycode[i];
    if (key && !isInArray(key, last_report.keycode)) {
      if (key == KEY_UP) octaveShift++;
      else if (key == KEY_DOWN) octaveShift--;

      else if (key == KEY_PGDOWN) {
        if (mastvolume > 46) mastvolume-=20;
        //Serial.printf("Instrument = %d\n", currentInstrument);
        synth.setMasterVolume(mastvolume);
      }
      else if (key == KEY_PGUP) {
        if (mastvolume < 108) mastvolume+=20;
        //Serial.printf("Instrument = %d\n", currentInstrument);
        synth.setMasterVolume(mastvolume); 
      }

      else if (key == KEY_LEFT) {
        if (currentInstrument > 0) currentInstrument--;
        //Serial.printf("Instrument = %d\n", currentInstrument);
        synth.setInstrument(0, 0, currentInstrument);
      }
      else if (key == KEY_RIGHT) {
        if (currentInstrument < 127) currentInstrument++;
        //Serial.printf("Instrument = %d\n", currentInstrument);
        synth.setInstrument(0, 0, currentInstrument);
      }
      else {
        for (int j = 0; j < sizeof(instantInstrumentKeys); j++) {
          if (key == instantInstrumentKeys[j]) {
            currentInstrument = instantInstruments[j];
            synth.setInstrument(0, 0, currentInstrument);
            break;
          }
        }
      }
      
       if (isChordKey(key)) {
        addHeldChordKey(key);
        playChordKey(key, newMode, true);
      }
    }
  }

  // Detect released keys
  for (int i = 0; i < MAX_KEYS; i++) {
    uint8_t key = last_report.keycode[i];
    if (key && !isInArray(key, report.keycode)) {
      if (isChordKey(key)) {
        removeHeldChordKey(key);
        playChordKey(key, chordMode, false);
      }
    }
  }

  // If modifier keys changed, re-send all held chord keys
  static bool lastQ = false, lastW = false, lastE = false, lastR = false, lastT = false, lastY = false, lastU, lastI = false, lastO = false, lastP = false;
  if (modeKeyQ != lastQ || modeKeyW != lastW || modeKeyE != lastE || modeKeyR != lastR || modeKeyT != lastT || modeKeyY != lastY || modeKeyU != lastU || modeKeyI != lastI || modeKeyO != lastO || modeKeyP != lastP) {
    for (int i = 0; i < heldChordCount; i++) {
      playChordKey(heldChordKeys[i], chordMode, false); // turn off old mode
    }
    for (int i = 0; i < heldChordCount; i++) {
      playChordKey(heldChordKeys[i], newMode, true); // turn on new mode
    }
    chordMode = newMode;
  }
  lastQ = modeKeyQ;
  lastW = modeKeyW;
  lastE = modeKeyE;
  lastR = modeKeyR;
  lastT = modeKeyT;
  lastY = modeKeyY;
  lastU = modeKeyU;
  lastI = modeKeyI;
  lastO = modeKeyO;
  lastP = modeKeyP;
}

void playChordKey(uint8_t keycode, uint8_t mode, bool on) {
  for (int i = 0; i < sizeof(chordKeys) / sizeof(KeyChord); i++) {
    if (chordKeys[i].keycode == keycode) {
      uint8_t root = chordKeys[i].baseNote;
      uint8_t octave = BASE_OCTAVE + octaveShift;
      uint8_t baseNote = root + 12 * octave;

      if (on) {
        ActiveChord* slot = nullptr;
        for (int j = 0; j < activeChordCount; j++) {
          if (activeChords[j].keycode == keycode) {
            slot = &activeChords[j];
            break;
          }
        }
        if (!slot && activeChordCount < MAX_HELD) {
          slot = &activeChords[activeChordCount++];
          slot->keycode = keycode;
        }
        if (slot) {
          slot->noteCount = CHORD_LENGTH[mode];
          for (int k = 0; k < CHORD_LENGTH[mode]; k++) {
            note = baseNote + CHORD_INTERVALS[mode][k];
            synth.setNoteOn(0, note, 127);
            slot->notes[k] = note;
          }
        }
      } else {
        for (int j = 0; j < activeChordCount; j++) {
          if (activeChords[j].keycode == keycode) {
            for (int k = 0; k < activeChords[j].noteCount; k++) {
              synth.setNoteOff(0,activeChords[j].notes[k], 0);
            }
            // Remove this entry
            for (int l = j; l < activeChordCount - 1; l++) {
              activeChords[l] = activeChords[l + 1];
            }
            activeChordCount--;
            break;
          }
        }
      }
      return;
    }
  }
  //draw();
 };

};

/*void playChord(uint8_t root, uint8_t mode, bool on) {
  uint8_t octave = BASE_OCTAVE + octaveShift;
  uint8_t baseNote = root + 12 * octave;
  for (int i = 0; i < CHORD_LENGTH[mode]; i++) {
    uint8_t note = baseNote + CHORD_INTERVALS[mode][i];
    if (on)
      synth.setNoteOn(0, note, 127);
    else
      synth.setNoteOff(0, note, 0);
  }
}*/

MyEspUsbHost usbHost;
void setup() {
    auto cfg = M5.config();
    AtomS3.begin(cfg);
    AtomS3.Display.setBrightness(60);
    synth.begin(&Serial2, UNIT_SYNTH_BAUD, 1, 2);
    synth.setMasterVolume(mastvolume); //107 kot default potem po +- 20
    synth.setInstrument(0, 0, currentInstrument);
    synth.setAllNotesOff(0); 
    usbHost.begin();
    usbHost.setHIDLocal(HID_LOCAL_Japan_Katakana);
    canvas.createSprite(128, 128);
    draw();

}

void loop() {
 usbHost.task(); 

 AtomS3.update();
 if (AtomS3.BtnA.wasPressed())
 {
  currentInstrument = 16;
  synth.setAllNotesOff(0); 
  synth.setInstrument(0, 0, currentInstrument);
  mastvolume = 107;
  synth.setMasterVolume(mastvolume); 
  //draw();
 }

 if((millis()-timer)>100){ // print data every 100ms
  draw();
	timer = millis();  
  }

}
