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

AudioPlaySdWav playWav1;
// Use one of these 3 output types: Digital I2S, Digital S/PDIF, or Analog DAC
AudioOutputI2S audioOutput;
// AudioOutputSPDIF       audioOutput;
//  AudioOutputAnalog      audioOutput;
// On Teensy LC, use this for the Teensy Audio Shield:
// AudioOutputI2Sslave    audioOutput;

AudioConnection patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000 sgtl5000_1;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN 10
#define SDCARD_MOSI_PIN 7
#define SDCARD_SCK_PIN 14

// Use these with the Teensy 3.5 & 3.6 SD card
// #define SDCARD_CS_PIN    BUILTIN_SDCARD
// #define SDCARD_MOSI_PIN  11  // not actually used
// #define SDCARD_SCK_PIN   13  // not actually used

// Use these for the SD+Wiz820 or other adaptors
// #define SDCARD_CS_PIN    4
// #define SDCARD_MOSI_PIN  11
// #define SDCARD_SCK_PIN   13

unsigned long lastPlayTime = 0;
unsigned long quietDelay = 1000; // in ms, divide by 60000 for minutes

// GPIO output to ESP32
#define ESP32_QUE_IO 0
#define ESP32_START_IO 1

#define QUE_OPENING_TIME_DARK 2000       // 28 secs duration event 1
#define QUE_OPENING_TIME_LIGHT 30000     // 31 secs duration event 2
#define QUE_OPENING_END_TIME_LIGHT 61000 // 57 secs duration 3
#define QUE_OPENING_END_TIME_DARK 118000 // 19 secs duration 4
#define QUE_GROWING_TIME_DARK 137000     // 30 secs duration 5
#define QUE_GROWING_TIME_LIGHT 167000    // 23 secs duration 6
#define QUE_KNIFE_TIME_LIGHT 190000      // 57 secs duration 7
#define QUE_KNIFE_TIME_DARK 247000       // 20 secs duration 8
#define QUE_BALLS_TIME_DARK 267000       // 20 secs duration 9
#define QUE_BALLS_TIME_LIGHT 287000      // 41 secs duration 10
#define QUE_GET_UP_TIME_LIGHT 328000     // 80 secs duration 11
#define QUE_GET_UP_TIME_DARK 408000      // 20 secs duration 12
#define QUE_CAVE_TIME_DARK 428000        // 20 secs duration 13
#define QUE_CAVE_TIME_LIGHT 448000       // 34 secs duration 14
#define QUE_MICRO_TIME_LIGHT 482000      // 75 secs duration 15
#define QUE_MICRO_TIME_DARK 557000       // 20 secs duration 16
#define QUE_DREAMS_TIME_DARK 577000      // 20 secs duration 17
#define QUE_DREAMS_TIME_LIGHT 597000     // 50 secs duration 18
#define QUE_HEART_TIME_LIGHT 647000      // 40 secs duration 19
#define QUE_HEART_TIME_DARK 687000       // 20 secs duration 20
#define QUE_TEMPLE_TIME_DARK 707000      // 20 secs duration 21
#define QUE_TEMPLE_TIME_LIGHT 727000     // 25 secs duration 22
#define QUE_YEARS_TIME_LIGHT 752000      // 27 secs duration 23
#define QUE_YEARS_TIME_DARK 779000       // 10 secs duration 24
#define QUE_MUSIC_TIME_DARK 789000       // 20 secs duration 25
#define QUE_MUSIC_TIME_LIGHT 809000      // 44 secs duration 26
#define QUE_LOVE_TIME_LIGHT 853000       // 65 secs duration 27
#define QUE_LOVE_TIME_DARK 918000        // 20 secs duration 28
// #define QUE_END_TIME    938000        //  29
#define MAX_EVENT_CNT 29
uint8_t eventCnt = 0;

// Monitoring
unsigned long lastMonitorTime = 0;
unsigned long MonitorDelay = 1000;

void setup()
{
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
    if (!(SD.begin(SDCARD_CS_PIN)))
    {
        // stop here, but print a message repetitively
        while (1)
        {
            Serial.println("Unable to access the SD card, fix and press reset button");
            delay(500);
        }
    }
    Serial.println("Audio setup done.");

    pinMode(ESP32_START_IO, OUTPUT);
    pinMode(ESP32_QUE_IO, OUTPUT);
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
    // float vol = analogRead(15);
    // vol = vol / 1024;
    // sgtl5000_1.volume(vol);
    // }
}

void stopFile()
{
    Serial.println(F("Stopping file"));
    playWav1.stop();
}

void loop()
{
    // Start IO is used to signal ESP32 audio playing just started
    digitalWrite(ESP32_START_IO, LOW);

    // When file reaches que times, send signal to ESP32
    if (playWav1.positionMillis() > QUE_OPENING_TIME_DARK && playWav1.positionMillis() < QUE_OPENING_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 1;
    }
    else if (playWav1.positionMillis() > QUE_OPENING_TIME_LIGHT && playWav1.positionMillis() < QUE_OPENING_END_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 2;
    }
    else if (playWav1.positionMillis() > QUE_OPENING_END_TIME_LIGHT && playWav1.positionMillis() < QUE_OPENING_END_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 3;
    }
    else if (playWav1.positionMillis() > QUE_OPENING_END_TIME_DARK && playWav1.positionMillis() < QUE_GROWING_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 4;
    }
    else if (playWav1.positionMillis() > QUE_GROWING_TIME_DARK && playWav1.positionMillis() < QUE_GROWING_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 5;
    }
    else if (playWav1.positionMillis() > QUE_GROWING_TIME_LIGHT && playWav1.positionMillis() < QUE_KNIFE_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 6;
    }
    else if (playWav1.positionMillis() > QUE_KNIFE_TIME_LIGHT && playWav1.positionMillis() < QUE_KNIFE_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 7;
    }
    else if (playWav1.positionMillis() > QUE_KNIFE_TIME_DARK && playWav1.positionMillis() < QUE_BALLS_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 8;
    }
    else if (playWav1.positionMillis() > QUE_BALLS_TIME_DARK && playWav1.positionMillis() < QUE_BALLS_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 9;
    }
    else if (playWav1.positionMillis() > QUE_BALLS_TIME_LIGHT && playWav1.positionMillis() < QUE_GET_UP_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 10;
    }
    else if (playWav1.positionMillis() > QUE_GET_UP_TIME_LIGHT && playWav1.positionMillis() < QUE_GET_UP_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 11;
    }
    else if (playWav1.positionMillis() > QUE_GET_UP_TIME_DARK && playWav1.positionMillis() < QUE_CAVE_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 12;
    }
    else if (playWav1.positionMillis() > QUE_CAVE_TIME_DARK && playWav1.positionMillis() < QUE_CAVE_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 13;
    }
    else if (playWav1.positionMillis() > QUE_CAVE_TIME_LIGHT && playWav1.positionMillis() < QUE_MICRO_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 14;
    }
    else if (playWav1.positionMillis() > QUE_MICRO_TIME_LIGHT && playWav1.positionMillis() < QUE_MICRO_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 15;
    }
    else if (playWav1.positionMillis() > QUE_MICRO_TIME_DARK && playWav1.positionMillis() < QUE_DREAMS_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 16;
    }
    else if (playWav1.positionMillis() > QUE_DREAMS_TIME_DARK && playWav1.positionMillis() < QUE_DREAMS_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 17;
    }
    else if (playWav1.positionMillis() > QUE_DREAMS_TIME_LIGHT && playWav1.positionMillis() < QUE_HEART_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 18;
    }
    else if (playWav1.positionMillis() > QUE_HEART_TIME_LIGHT && playWav1.positionMillis() < QUE_HEART_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 19;
    }
    else if (playWav1.positionMillis() > QUE_HEART_TIME_DARK && playWav1.positionMillis() < QUE_TEMPLE_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 20;
    }
    else if (playWav1.positionMillis() > QUE_TEMPLE_TIME_DARK && playWav1.positionMillis() < QUE_TEMPLE_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 21;
    }
    else if (playWav1.positionMillis() > QUE_TEMPLE_TIME_LIGHT && playWav1.positionMillis() < QUE_YEARS_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 22;
    }
    else if (playWav1.positionMillis() > QUE_YEARS_TIME_LIGHT && playWav1.positionMillis() < QUE_YEARS_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 23;
    }
    else if (playWav1.positionMillis() > QUE_YEARS_TIME_DARK && playWav1.positionMillis() < QUE_MUSIC_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 24;
    }
    else if (playWav1.positionMillis() > QUE_MUSIC_TIME_DARK && playWav1.positionMillis() < QUE_MUSIC_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 25;
    }
    else if (playWav1.positionMillis() > QUE_MUSIC_TIME_LIGHT && playWav1.positionMillis() < QUE_LOVE_TIME_LIGHT)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 26;
    }
    else if (playWav1.positionMillis() > QUE_LOVE_TIME_LIGHT && playWav1.positionMillis() < QUE_LOVE_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, HIGH);
        eventCnt = 27;
    }
    else if (playWav1.positionMillis() > QUE_LOVE_TIME_DARK)
    {
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 28;
    }

    // If no file is playing, play default audio file
    if (!playWav1.isPlaying())
    {
        digitalWrite(ESP32_START_IO, HIGH);
        digitalWrite(ESP32_QUE_IO, LOW);
        eventCnt = 0;
        if ((millis() - lastPlayTime) > quietDelay)
        {
            playFile("1.wav");
            lastPlayTime = millis();
            Serial.println("No audio for more than quietDelay. Playing default audio file.");
        }
    }

    // Monitoring, not a must, just for debugging
    if ((millis() - lastMonitorTime) > MonitorDelay)
    {
        Serial.print("Audio teensy is ");
        if (playWav1.isPlaying())
        {
            Serial.print("playing audio file, current positionMillis: ");
            Serial.println(playWav1.positionMillis());
            Serial.print("Event count: ");
            Serial.println(eventCnt);
            Serial.print("Start IO state is: ");
            Serial.println(digitalRead(ESP32_START_IO));
            Serial.print("Que IO state is: ");
            Serial.println(digitalRead(ESP32_QUE_IO));
        }
        else
        {
            Serial.println("NO audio file playing");
        }
        lastMonitorTime = millis();
    }

    if (eventCnt >= MAX_EVENT_CNT)
    {
        Serial.println("Event count reached maximum. starting over.");
        eventCnt = 0;
    }

    // delay as we don't need to change things too often, can be removed
    delay(10);
}
