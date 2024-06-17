// Simple WAV file player example
//
// Three types of output may be used, by configuring the code below.
//
//   1: Digital I2S - Normally used with the audio shield:
//         http://www.pjrc.com/store/teensy3_audio.html
//
//   2: Digital S/PDIF - Connect pin 22 to a S/PDIF transmitter
//         https://www.oshpark.com/shared_projects/KcDBKHta
//
//   3: Analog DAC - Connect the DAC pin to an amplified speaker
//         http://www.pjrc.com/teensy/gui/?info=AudioOutputAnalog
//
// To configure the output type, first uncomment one of the three
// output objects.  If not using the audio shield, comment out
// the sgtl5000_1 lines in setup(), so it does not wait forever
// trying to configure the SGTL5000 codec chip.
//
// The SD card may connect to different pins, depending on the
// hardware you are using.  Uncomment or configure the SD card
// pins to match your hardware.
//
// Data files to put on your SD card can be downloaded here:
//   http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


// audio

AudioPlaySdWav           playWav1;
// Use one of these 3 output types: Digital I2S, Digital S/PDIF, or Analog DAC
AudioOutputI2S           audioOutput;
//AudioOutputSPDIF       audioOutput;
// AudioOutputAnalog      audioOutput;
//On Teensy LC, use this for the Teensy Audio Shield:
//AudioOutputI2Sslave    audioOutput;

AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Use these with the Teensy 3.5 & 3.6 SD card
// #define SDCARD_CS_PIN    BUILTIN_SDCARD
// #define SDCARD_MOSI_PIN  11  // not actually used
// #define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
//#define SDCARD_CS_PIN    4
//#define SDCARD_MOSI_PIN  11
//#define SDCARD_SCK_PIN   13

unsigned long lastPlayTime = 0;
unsigned long quietDelay = 1000; // in ms, divide by 60000 for minutes

// GPIO output to ESP32
#define ESP32_GPIO_OUT 1
#define FADE_IN_TIME 10000
#define FADE_OUT_TIME 20000

// Monitoring
unsigned long lastMonitorTime = 0;
unsigned long MonitorDelay = 1000;


void setup() {
    Serial.begin(115200);
    Serial.println("Serial Port Started.");
    
    // Audio connections require memory to work.  For more
    // detailed information, see the MemoryAndCpuUsage example
    AudioMemory(8);

    // Comment these out if not using the audio adaptor board.
    // This may wait forever if the SDA & SCL pins lack
    // pullup resistors
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.7);

    SPI.setMOSI(SDCARD_MOSI_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
    if (!(SD.begin(SDCARD_CS_PIN))) {
        // stop here, but print a message repetitively
        while (1) {
            Serial.println("Unable to access the SD card, fix and press reset button");
            delay(500);
        }
    }
    Serial.println("Audio setup done.");

    pinMode(ESP32_GPIO_OUT, OUTPUT);
}

void playFile(const char *filename)
{
    Serial.print("Playing file: ");
    Serial.println(filename);

    // Start playing the file.  This sketch continues to
    // run while the file plays.
    playWav1.play(filename);

    // A brief delay for the library read WAV info
    // delay(25);

    // Simply wait for the file to finish playing.
    // while (playWav1.isPlaying()) {
        // uncomment these lines if you audio shield
        // has the optional volume pot soldered
        //float vol = analogRead(15);
        //vol = vol / 1024;
        // sgtl5000_1.volume(vol);
    // }
}

void stopFile()
{
    Serial.println(F("Stopping file"));
    playWav1.stop();
}

void loop() {

  // When file reaches designated time, send signal to ESP32
  if (playWav1.positionMillis()%30000 > FADE_IN_TIME && playWav1.positionMillis()%30000 < FADE_OUT_TIME) {
    digitalWrite(ESP32_GPIO_OUT, HIGH);
  } else {
    digitalWrite(ESP32_GPIO_OUT, LOW);
  }

  // If no file is playing, play default audio file
  if (!playWav1.isPlaying()) {
    if((millis() - lastPlayTime) > quietDelay) {
      playFile("1.wav");
      lastPlayTime = millis();
    }
  }

  // Monitoring, not a must, just for debugging
  if((millis() - lastMonitorTime) > MonitorDelay) {
      Serial.print("Audio teensy is ");
      if (playWav1.isPlaying()) {
          Serial.print("playing audio file, current positionMillis: "); Serial.println(playWav1.positionMillis());
          Serial.print("Fade state is: "); Serial.println(digitalRead(ESP32_GPIO_OUT));
      } else {
          Serial.println("NO audio file playing");
      }
      lastMonitorTime = millis();
  }

  // delay as we don't need to change things too often, can be removed
  delay(10);
}
