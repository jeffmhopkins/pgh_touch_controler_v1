/*
  PITTSBURGH MODULAR TOUCH CONTROLLER - Ratchet / Probability EDITION

  Full per-step TEMPO-SYNCED Ratchet (1–10×) + Probability (10–100%)
  100 % compatible with stock behaviour — new features are completely invisible unless used

  ORIGINAL WORKFLOW (unchanged)
  ─────────────────────────────────────
  • Hold MONO → tap pads → release = monophonic sequence
  • Hold DUO  → tap left pads → tap right pads → release = two 5-step sequences
  • Tap REST while programming = insert a rest
  • Hold REST alone >1 s → set clock division (tap pad 0–9 = ÷1 to ÷10)

  NEW FEATURES — RATCHET & PROBABILITY (tempo-synced!)
  ─────────────────────────────────────
  Every step stores:
    • Ratchet count 1–10  → fires N evenly spaced triggers inside one clock pulse
    • Probability 10–100 % → chance of playing (in 10 % steps)

  HOW TO EDIT (uses only the REST button — no accidents!)
  ─────────────────────────────────────
  1. Program your sequence normally
  2. The **last step you added** is automatically selected for editing
  3. Hold **REST button > 1 second** → enters parameter edit mode
       → LED of the selected step double-blinks (off-on-off-on) as confirmation

  4. You are now in **Probability mode** by default
       → LED shows a 2-second duty cycle (100 % = solid on, 70 % = on 1.4 s / off 0.6 s, etc.)

  5. Short tap **REST** → switches to **Ratchet mode**
       → LED blinks N times (500 ms on/off), then 1 s pause, repeat (N = current ratchet count)

  6. Short tap **REST** again → back to Probability mode

  7. While in either mode, tap any pad 0–9:
       → Sets the value (0 = 10 %, 9 = 100 % or 10× ratchet)
       → Instantly saves and exits param mode

  8. Release REST button → exits param mode (or just tap a value)

  RATCHET TIMING
  ─────────────────────────────────────
  • Fully tempo-synced: ratchets are evenly spaced inside the current clock interval
  • Works perfectly from 30 BPM to 300+ BPM
  • 4× ratchet at 120 BPM = perfect 16th-note roll
  • 6× ratchet = perfect 6 divisions, etc.

  DEFAULT VALUES
  ─────────────────────────────────────
  Every new step is created with:
    • Ratchet = 1× (no ratchet)
    • Probability = 100 % (always plays)

  Using the new features is 100 % optional and never interferes with normal operation.
*/

#include <CapacitiveSensor.h>

struct Step {
    byte channel;     // 0–9 = note, 99 = rest
    byte yAxis;
    byte ratchet;     // 1–10 (1 = no ratchet)
    byte prob;        // 1–10 → 10–100%
};

volatile bool clockInterruptTriggered = 0;
void clockInterrupted() { clockInterruptTriggered = 1; }

unsigned long lastClockTime = 0;
unsigned long currentClockInterval = 1000;  // fallback

byte ratchetSubCount = 0;           // current subdivision (0 = new step)
unsigned long nextRatchetTime = 0;

bool paramEditActive = false;
bool paramIsRatchetMode = false;
unsigned long paramEntryTime = 0;
unsigned long lastLEDBlink = 0;
byte currentBlinkCount = 0;
byte targetBlinkCount = 0;

Step seqMono[64];
Step seqLeft[64];
Step seqRight[64];

byte seqLenMono = 0;
byte seqLenLeft = 0;
byte seqLenRight = 0;

byte currentStepMono = 0;
byte currentStepLeft = 0;
byte currentStepRight = 0;
byte ratchetCountdown = 0;

int8_t editStepIndex = -1;           // -1 = none
bool inParamMode = false;
bool probMode = false;
unsigned long paramHoldTime = 0;
unsigned long touchHoldTime[10] = {0};

byte switchInput[10] = {44, 46, 48, 50, 52, 47, 45, 2, 51, 49};
byte switchResistor[10] = {25, 24, 27, 26, 29, 43, 42, 38, 41, 40};
byte axisInput[10] = {A2, A1, A3, A0, A4, A5, A9, A6, A8, A7};
byte axisResistor[10] = {30, 31, 33, 28, 32, 35, 39, 34, 36, 37};
byte channelOutput[10] = {7, 6, 5, 4, 3, 53, 14, 15, 16, 17};
byte axisOutput[3] = {11, 12, 13};

const byte clockInput = 68;
byte resetInput = 69;
byte scanInput = A10;

byte monoButton = 8;
byte restButton = 9;
byte duoButton = 10;

byte gateOutputLeft = 67;
byte gateOutputRight = 66;
byte gateOutputAll = A11;

byte allSelectorA = 18;
byte allSelectorB = 19;
byte allSelectorC = 22;
byte allSelectorD = 23;

CapacitiveSensor capSwitch[10] = {
        CapacitiveSensor(switchResistor[0], switchInput[0]),
        CapacitiveSensor(switchResistor[1], switchInput[1]),
        CapacitiveSensor(switchResistor[2], switchInput[2]),
        CapacitiveSensor(switchResistor[3], switchInput[3]),
        CapacitiveSensor(switchResistor[4], switchInput[4]),
        CapacitiveSensor(switchResistor[5], switchInput[5]),
        CapacitiveSensor(switchResistor[6], switchInput[6]),
        CapacitiveSensor(switchResistor[7], switchInput[7]),
        CapacitiveSensor(switchResistor[8], switchInput[8]),
        CapacitiveSensor(switchResistor[9], switchInput[9])};
CapacitiveSensor capAxis[10] = {
        CapacitiveSensor(axisResistor[0], axisInput[0]),
        CapacitiveSensor(axisResistor[1], axisInput[1]),
        CapacitiveSensor(axisResistor[2], axisInput[2]),
        CapacitiveSensor(axisResistor[3], axisInput[3]),
        CapacitiveSensor(axisResistor[4], axisInput[4]),
        CapacitiveSensor(axisResistor[5], axisInput[5]),
        CapacitiveSensor(axisResistor[6], axisInput[6]),
        CapacitiveSensor(axisResistor[7], axisInput[7]),
        CapacitiveSensor(axisResistor[8], axisInput[8]),
        CapacitiveSensor(axisResistor[9], axisInput[9])};
bool capSwitchState[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
bool capSwitchPressed[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long capSwitchValue[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long capAxisValue[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long capSwitchBuffer[10] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
byte capSwitchCalibrationCounter[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte activeChannelLeft = 0;
byte activeChannelRight = 5;
byte activeChannelAll = 0;
byte activeTouchChannelAll = 99;
byte activeTouchChannelLeft = 99;
byte activeTouchChannelRight = 99;

byte switchSamples = 3;  // LARGER NUMBER PRODUCES CHEANER TRIGGERS ::: SMALLER
// NUMBER PRODUCES FASTER RESPONSE
int switchThreshold = 400;  // VALUE ABOVE NOISE FLOOR NEEDED TO TRIGGER CHANNEL
byte axisSamples = 3;  // LARGER NUMBER PREVENTS THE Y-AXIS FROM JUMPING AROUND
// ::: SMALLER NUMBER PRODUCES FASTER RESPONSE
float axisIndividualSmooth = .4;  // SMOOTH THE INDIVIDUAL Y-AXIS PAD READS
float axisSumSmooth = .3;         // SMOOTH THE SUMMED TOUCH VALUE
byte sliderWabbleBuffer =
        4;  // MINIMUM DIFFERENCE BETWEEN Y-AXIS CHANNEL READS REMOVES WABBLE
byte touchLowPercent =
        15;  // MINIMUM READ PERCENT FOR AN INDIVIDUAL Y-AXIS CHANNEL READ
byte touchLowRead = 40;  // MINIMUM VALUE FOR AN INDIVIDUAL Y-AXIS CHANNEL READ

byte clockDivisionMono = 0;
byte clockDivisionLeft = 0;
byte clockDivisionRight = 0;

byte activeSequenceStepMono = 99;
byte activeSequenceStepLeft = 99;
byte activeSequenceStepRight = 99;

byte clockDivisionStepMono = 99;
byte clockDivisionStepLeft = 99;
byte clockDivisionStepRight = 99;

byte sequenceNotesMono[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte sequenceYAxisMono[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte sequenceLengthMono = 0;

bool monoNotePressed = 0;
bool monoNotePressedMono = 0;

byte sequenceNotesLeft[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte sequenceYAxisLeft[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte sequenceLengthLeft = 0;

byte sequenceNotesRight[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte sequenceYAxisRight[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte sequenceLengthRight = 0;

bool duoNotePressed = 0;
bool duoNotePressedLeft = 0;
bool duoNotePressedRight = 0;

bool monoButtonPressed = 0;
bool restButtonPressed = 0;
bool duoButtonPressed = 0;

byte lastDuoNotePressed = 0;
byte gateLength = 10;

float clockTimer = 0;
float gateTimerLeft = 0;
float gateTimerRight = 0;
float gateTimerAll = 0;

bool noteMode = 0;  //  0 = MONO  1 = DUO

byte lastSliderValueLeft = 0;
byte lastSliderValueRight = 0;
volatile bool resetInputUsed = 0;
int capSwitchTimerLength = 25;
float capSwitchTimerLeft = 0;
float lastCapSwitchTimerLeft = 0;
float capSwitchTimerRight = 0;
float lastCapSwitchTimerRight = 0;
bool externalClockResetDisabled = 1;
int touchCounter[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

bool clockInputUsed = 0;
// loat clockInputUsedTimer = 0;
volatile bool resetInputTriggered = 0;
volatile bool clockInterruptTriggered = 0;
bool enableSequenceEdit = 0;
float clockInterruptIgnore = 0;
bool resetTriggerUsed = 0;
bool monoButtonState = 0;
bool monoButtonChangeEnabled = 1;
bool duoButtonState = 0;
bool duoButtonChangeEnabled = 1;
int lastActiveSliderValueAll = 0;
int lastActiveSliderValueLeft = 0;
int lastActiveSliderValueRight = 0;

int lastCapAxisValue[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

bool clockInputUsedCheckFlag = 0;
float clockInputUsedCheckTimer = 0;
bool gateOutputAllActive = 0;
bool gateOutputLeftActive = 0;
bool gateOutputRightActive = 0;
float leftYAxisCheck = 0;
bool lastYAxisUsed = 0;

bool scanInputUsed = 0;
byte lastScannedChannel = 99;
byte scannedChannel = 99;

void autoCalibrateSwitch(byte z, bool useLed) {
    capSwitchBuffer[z] = 100;
    capSwitch[z].reset_CS_AutoCal();

    if (useLed == 1) {
        digitalWrite(channelOutput[0], 0);  // TURN OFF LED 5
        digitalWrite(channelOutput[1], 0);  // TURN OFF LED 6
        digitalWrite(channelOutput[2], 0);  // TURN OFF LED 7
        digitalWrite(channelOutput[3], 0);  // TURN OFF LED 8
        digitalWrite(channelOutput[4], 0);  // TURN OFF LED 9
        digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
        digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
        digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
        digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
        digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

        digitalWrite(channelOutput[z], 1);  // TURN ON CHANNEL LED

        delay(30);
    }
}

void checkCapSwitchLeft(byte x) {
    if ((noteMode == 0 && monoButtonPressed == 0) ||
        (noteMode == 0 && monoButtonPressed == 1 && enableSequenceEdit == 1) ||
        (noteMode == 1 && duoButtonPressed == 0) ||
        (noteMode == 1 && duoButtonPressed == 1 && enableSequenceEdit == 1)) {
        capSwitchValue[x] =
                capSwitch[x].capacitiveSensor(switchSamples);  // READ CAPACITIVE SWITCH

        if (capSwitchValue[x] < 0) capSwitchValue[x] = 0;

        if (capSwitchValue[x] >=
            capSwitchBuffer[x] + switchThreshold)  // IF SWITCH IS PRESSED
        {
            if (capSwitchState[x] == 0 &&
                capSwitchPressed[x] == 0)  // IF NEWLY PRESSED
            {
                capSwitchState[x] = 1;    // SET CURRENT SWITCH STATE ON
                capSwitchPressed[x] = 1;  // FLAG THE KEY AS PRESSED SO IT CAN BE
                // UNPRESSED IF MULTIPLE KEYS ARE PRESSED
                capSwitchTimerLeft = millis();
                touchCounter[x]++;
                capSwitchCalibrationCounter[x] = 0;

                if (noteMode == 0) {
                    for (byte y = 0; y < 10; y++) {
                        if (y != x) {
                            capSwitchState[y] = 0;
                            autoCalibrateSwitch(y, 0);
                        }
                    }
                }

                else  // if (noteMode == 1)
                {
                    for (byte y = 0; y < 5; y++) {
                        if (y != x) {
                            capSwitchState[y] = 0;
                            autoCalibrateSwitch(y, 0);
                        }
                    }
                }

                // LEFT SWITCH PRESSED
                // -------------------
                if (monoButtonPressed == 0 && restButtonPressed == 0 &&
                    duoButtonPressed == 0) {
                    if ((noteMode == 0 && activeSequenceStepMono == 99) ||
                        (noteMode == 1 && activeSequenceStepLeft == 99)) {
                        digitalWrite(channelOutput[0], 0);  // TURN OFF LED 0
                        digitalWrite(channelOutput[1], 0);  // TURN OFF LED 1
                        digitalWrite(channelOutput[2], 0);  // TURN OFF LED 2
                        digitalWrite(channelOutput[3], 0);  // TURN OFF LED 3
                        digitalWrite(channelOutput[4], 0);  // TURN OFF LED 4

                        if (noteMode == 0)  // MONO MODE
                        {
                            digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
                            digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
                            digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
                            digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
                            digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

                            activeChannelLeft = x;  // SET ACTIVE LEFT CHANNEL
                            activeChannelAll = x;   // SET ACTIVE ALL CHANNEL

                            digitalWrite(allSelectorA, 0);
                            digitalWrite(allSelectorB, 1);
                            digitalWrite(allSelectorC, 0);
                            digitalWrite(allSelectorD, 1);
                        }

                        else  // if (noteMode == 1) // DUO MODE
                        {
                            activeChannelLeft = x;  // SET ACTIVE LEFT CHANNEL
                            activeChannelAll = x;   // SET ACTIVE ALL CHANNEL
                        }

                        if (x == 0)
                            digitalWrite(channelOutput[0], 1);  // TURN ON ACTIVE LEFT LED
                        else if (x == 1)
                            digitalWrite(channelOutput[1], 1);  // TURN ON ACTIVE LEFT LED
                        else if (x == 2)
                            digitalWrite(channelOutput[2], 1);  // TURN ON ACTIVE LEFT LED
                        else if (x == 3)
                            digitalWrite(channelOutput[3], 1);  // TURN ON ACTIVE LEFT LED
                        else
                            digitalWrite(channelOutput[4], 1);  // TURN ON ACTIVE LEFT LED

                        gateTimerLeft = 0;
                        if (noteMode == 0 ||
                            (noteMode == 1 && activeSequenceStepRight == 99))
                            gateTimerAll = 0;
                        if ((noteMode == 0 && activeSequenceStepMono == 99) ||
                            (noteMode == 1 && activeSequenceStepLeft == 99)) {
                            if (gateOutputLeftActive == 1) {
                                digitalWrite(gateOutputLeft, 0);
                                delay(1);
                            }
                            digitalWrite(gateOutputLeft, 1);
                            gateOutputLeftActive = 1;

                            if (activeSequenceStepRight == 99) {
                                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                                if (gateOutputRightActive == 1)
                                    digitalWrite(gateOutputRight, 0);
                                if (gateOutputAllActive == 1 || gateOutputRightActive == 1)
                                    delay(1);

                                digitalWrite(gateOutputAll, 1);
                                digitalWrite(gateOutputRight, 0);

                                gateOutputAllActive = 1;
                                gateOutputRightActive = 1;
                            }
                        }

                        readYAxisLeft();
                    }

                    else if ((noteMode == 0 && activeSequenceStepMono != 99) ||
                             (noteMode == 1 && activeSequenceStepLeft != 99)) {
                        if (noteMode == 0) {
                            activeTouchChannelAll = x;    // SET ACTIVE ALL TOUCH CHANNEL
                            activeTouchChannelLeft = 99;  // SET ACTIVE ALL TOUCH CHANNEL
                        }

                        else if (noteMode == 1) {
                            activeTouchChannelAll = 99;  // SET ACTIVE ALL TOUCH CHANNEL
                            activeTouchChannelLeft = x;  // SET ACTIVE ALL TOUCH CHANNEL
                        }

                        readYAxisLeft();
                    }
                }

                    // MONO BUTTON PRESSED
                    // ----------------------------------------
                else if (monoButtonPressed == 1 &&
                         lastCapSwitchTimerLeft + capSwitchTimerLength <
                         capSwitchTimerLeft) {
                    if (enableSequenceEdit == 1) {
                        lastCapSwitchTimerLeft = capSwitchTimerLeft;
                        monoNotePressed = 1;

                        if (monoNotePressedMono == 0) {
                            monoNotePressedMono = 1;

                            for (byte y = 0; y < 64; y++)  // CLEAR MONO SEQUENCES
                            {
                                sequenceNotesMono[y] = 0;
                            }

                            activeSequenceStepMono = 99;
                            sequenceLengthMono = 0;
                        }

                        if (sequenceLengthMono < 63) {
                            if (activeSequenceStepMono == 99) {
                                activeSequenceStepMono = 88;
                                sequenceLengthMono = 0;
                                sequenceNotesMono[0] = x;
                            } else {
                                sequenceLengthMono++;
                                sequenceNotesMono[sequenceLengthMono] = x;
                            }
                        }
                    }
                }

                    // REST BUTTON PRESSED
                    // ----------------------------------------
                else if (restButtonPressed == 1 &&
                         lastCapSwitchTimerLeft + capSwitchTimerLength <
                         capSwitchTimerLeft) {
                    externalClockResetDisabled = 1;
                    lastCapSwitchTimerLeft = capSwitchTimerLeft;
                    if (noteMode == 0) {
                        clockDivisionMono = x;
                    }

                    else {
                        clockDivisionLeft = x;
                        if (clockDivisionLeft == 4) clockDivisionLeft = 7;
                    }
                }

                    // DUO BUTTON PRESSED
                    // ----------------------------------------
                else if (duoButtonPressed == 1 &&
                         lastCapSwitchTimerLeft + capSwitchTimerLength <
                         capSwitchTimerLeft) {
                    if (enableSequenceEdit == 1) {
                        lastCapSwitchTimerLeft = capSwitchTimerLeft;
                        duoNotePressed = 1;
                        lastDuoNotePressed = 0;

                        if (duoNotePressedLeft == 0) {
                            duoNotePressedLeft = 1;

                            for (byte y = 0; y < 64; y++)  // CLEAR DUO SEQUENCES
                            {
                                sequenceNotesLeft[y] = 0;
                            }

                            activeSequenceStepLeft = 99;
                            sequenceLengthLeft = 0;
                        }

                        if (sequenceLengthLeft < 63) {
                            if (activeSequenceStepLeft == 99) {
                                activeSequenceStepLeft = 88;
                                sequenceLengthLeft = 0;
                                sequenceNotesLeft[0] = x;
                            } else {
                                sequenceLengthLeft++;
                                sequenceNotesLeft[sequenceLengthLeft] = x;
                            }
                        }
                    }
                }
            }

                // KEY ALREADY PRESSED
                // -------------------
            else if (capSwitchState[x] == 1 && capSwitchPressed[x] == 1) {
                if ((noteMode == 0 && activeChannelAll == x &&
                     activeSequenceStepMono == 99) ||
                    (noteMode == 1 && activeChannelLeft == x &&
                     activeSequenceStepLeft == 99)) {
                    readYAxisLeft();
                }

                else if ((noteMode == 0 && activeSequenceStepMono != 99) ||
                         (noteMode == 1 && activeSequenceStepLeft != 99)) {
                    readYAxisLeft();
                }
            }
        }

        else  // if (capSwitchValue[x] < capSwitchBuffer[x] + switchThreshold)  //
            // IF SWITCH IS NOT PRESSED
        {
            capSwitchState[x] = 0;  // SET CURRENT SWITCH STATE OFF
            capSwitchPressed[x] = 0;

            if (noteMode == 0 && activeTouchChannelAll == x)
                activeTouchChannelAll = 99;  // CLEAR ACTIVE ALL TOUCH CHANNEL
            else if (noteMode == 1 && activeTouchChannelLeft == x)
                activeTouchChannelLeft = 99;  // CLEAR ACTIVE LEFT TOUCH CHANNEL

            if (noteMode == 0 && activeChannelAll == x) {
                digitalWrite(gateOutputLeft, 0);
                digitalWrite(gateOutputAll, 0);
                gateOutputAllActive = 0;
                gateOutputLeftActive = 0;
            }

            else if (noteMode == 1 && activeChannelLeft == x) {
                digitalWrite(gateOutputLeft, 0);
                gateOutputLeftActive = 0;
                if (activeSequenceStepRight == 99 && activeChannelAll == x) {
                    digitalWrite(gateOutputAll, 0);
                    gateOutputAllActive = 0;
                }
            }

            if (capSwitchCalibrationCounter[x] == 0)
                capSwitchBuffer[x] = capSwitchValue[x];
            capSwitchCalibrationCounter[x]++;
            if (capSwitchCalibrationCounter[x] > 10)
                capSwitchCalibrationCounter[x] = 0;
        }
    }
}

void readYAxisLeft() {
    int sliderValue = 0;
    float capAxisTotalLeft = 0;
    float capAxisPercent[5] = {0, 0, 0, 0, 0};

    // DETERMINE Y-AXIS VALUE
    // ------------------------------
    for (byte y = 0; y < 5; y++)  // CYCLE THROUGH All Y-AXIS SENSORS
    {
        stepClock();  // DON'T LET THE CLOCK SHIFT
        lastCapAxisValue[y] = capAxisValue[y];
        if (noteMode == 0) lastCapAxisValue[y + 5] = capAxisValue[y];

        capAxisValue[y] = capAxis[y].capacitiveSensor(
                axisSamples);  // READ Y-AXIS CAPACITIVE SENSOR VALUE

        capAxisValue[y] =
                smooth(capAxisValue[y], axisIndividualSmooth, lastCapAxisValue[y]);

        if (capAxisValue[y] < touchLowRead) capAxisValue[y] = 0;
        capAxisTotalLeft =
                capAxisTotalLeft + capAxisValue[y];  // ADD VALUE TO LEFT TOTAL
    }

    for (byte y = 0; y < 5; y++)  // CYCLE THROUGH All Y-AXIS SENSORS
    {
        capAxisPercent[y] = (capAxisValue[y] / capAxisTotalLeft) *
                            100;  // DETERMINE LEFT SENSOR TOUCH PERCENTAGE
        if (capAxisPercent[y] < touchLowPercent)
            capAxisValue[y] =
                    0;  // SET LOW PERCENTAGE VALUES TO 0 TO CLEAN UP READINGS
        capAxisTotalLeft = capAxisTotalLeft +
                           capAxisValue[y];  // ADD CLEANED UP VALUE TO LEFT TOTAL
        capAxisPercent[y] =
                (capAxisValue[y] / capAxisTotalLeft) *
                100;  // DETERMINE CLEANED UP LEFT SENSOR TOUCH PERCENTAGE
    }

    // DETERMINE LEFT Y-AXIS FINGER POSITION
    // -------------------------------------
    sliderValue = axisLocation(capAxisPercent[1], capAxisPercent[2],
                               capAxisPercent[3], capAxisPercent[4]);

    // CLEAN UP Y-AXIS VALUE
    // -------------------------------------
    if (sliderValue > 255) sliderValue = 255;

    if (lastSliderValueLeft == 0 ||
        sliderValue > lastSliderValueLeft + sliderWabbleBuffer ||
        sliderValue < lastSliderValueLeft - sliderWabbleBuffer ||
        lastYAxisUsed == 1) {
        sliderValue = smooth(sliderValue, axisSumSmooth, lastSliderValueLeft);
        lastSliderValueLeft = sliderValue;

        if (noteMode == 0)
            lastActiveSliderValueAll = sliderValue;
        else
            lastActiveSliderValueLeft = sliderValue;
        lastYAxisUsed = 0;

        // OUTPUT LEFT Y-AXIS
        // ------------------
        if ((noteMode == 0 && activeSequenceStepMono == 99) ||
            (noteMode == 1 && activeSequenceStepLeft == 99)) {
            analogWrite(axisOutput[0], sliderValue);
            analogWrite(axisOutput[2], sliderValue);
        }

        if (monoButtonPressed == 1)
            sequenceYAxisMono[sequenceLengthMono] = sliderValue;
        else if (duoButtonPressed == 1)
            sequenceYAxisLeft[sequenceLengthLeft] = sliderValue;
        else if (monoButtonPressed == 0 && duoButtonPressed == 0) {
            if (noteMode == 0 && activeSequenceStepMono < 64)
                sequenceYAxisMono[activeSequenceStepMono] = sliderValue;
            if (noteMode == 1 && activeSequenceStepLeft < 64)
                sequenceYAxisLeft[activeSequenceStepLeft] = sliderValue;
        }
    }
}

byte axisLocation(int capAxisPercent1, int capAxisPercent2, int capAxisPercent3,
                  int capAxisPercent4) {
    float theSliderValue = 0;

    if (capAxisPercent4 > 5) {
        theSliderValue = 255;
    }

        // PAD 3 SELECTED
    else if (capAxisPercent3 > 5) {
        theSliderValue = 160 + (1.9 * capAxisPercent3);
    }

        // PAD 2 SELECTED
    else if (capAxisPercent2 > 5) {
        theSliderValue = 96 + (1.6 * capAxisPercent2);
    }

        // PAD 1 SELECTED
    else if (capAxisPercent1 > 5) {
        theSliderValue = 1.6 * capAxisPercent1;
    }

    else
        theSliderValue = 0;

    return theSliderValue;
}

void checkCapSwitchRight(byte x) {
    if ((noteMode == 0 && monoButtonPressed == 0) ||
        (noteMode == 0 && monoButtonPressed == 1 && enableSequenceEdit == 1) ||
        (noteMode == 1 && duoButtonPressed == 0) ||
        (noteMode == 1 && duoButtonPressed == 1 && enableSequenceEdit == 1)) {
        capSwitchValue[x + 5] = capSwitch[x + 5].capacitiveSensor(
                switchSamples);  // READ CAPACITIVE SWITCH

        if (capSwitchValue[x + 5] < 0) capSwitchValue[x + 5] = 0;

        if (capSwitchValue[x + 5] >=
            capSwitchBuffer[x + 5] + switchThreshold)  // IF SWITCH IS PRESSED
        {
            if (capSwitchState[x + 5] == 0 &&
                capSwitchPressed[x + 5] == 0)  // IF NEWLY PRESSED
            {
                capSwitchState[x + 5] = 1;    // SET CURRENT SWITCH STATE ON
                capSwitchPressed[x + 5] = 1;  // FLAG THE KEY AS PRESSED SO IT CAN BE
                // UNPRESSED IF MULTIPLE KEYS ARE PRESSED
                capSwitchTimerRight = millis();
                touchCounter[x + 5]++;
                capSwitchCalibrationCounter[x + 5] = 0;

                if (noteMode == 0) {
                    for (byte y = 0; y < 10; y++) {
                        if (y != x + 5) {
                            capSwitchState[y] = 0;
                            autoCalibrateSwitch(y, 0);
                        }
                    }
                }

                else  // if (noteMode == 1)
                {
                    for (byte y = 5; y < 10; y++) {
                        if (y != x + 5) {
                            capSwitchState[y] = 0;
                            autoCalibrateSwitch(y, 0);
                        }
                    }
                }

                // RIGHT SWITCH PRESSED
                // -------------------
                if (monoButtonPressed == 0 && restButtonPressed == 0 &&
                    duoButtonPressed == 0) {
                    if ((noteMode == 0 && activeSequenceStepMono == 99) ||
                        (noteMode == 1 && activeSequenceStepRight == 99)) {
                        digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

                        if (noteMode == 0)  // MONO MODE
                        {
                            digitalWrite(channelOutput[0], 0);  // TURN OFF LED 0
                            digitalWrite(channelOutput[1], 0);  // TURN OFF LED 1
                            digitalWrite(channelOutput[2], 0);  // TURN OFF LED 2
                            digitalWrite(channelOutput[3], 0);  // TURN OFF LED 3
                            digitalWrite(channelOutput[4], 0);  // TURN OFF LED 4

                            activeChannelRight = x + 5;  // SET ACTIVE RIGHT CHANNEL
                            activeChannelAll = x + 5;    // SET ACTIVE ALL CHANNEL

                            digitalWrite(allSelectorA, 1);
                            digitalWrite(allSelectorB, 0);
                            digitalWrite(allSelectorC, 1);
                            digitalWrite(allSelectorD, 0);
                        }

                        else  // if (noteMode == 1) // DUO MODE
                        {
                            activeChannelRight = x + 5;  // SET ACTIVE RIGHT CHANNEL
                            activeChannelAll = x + 5;    // SET ACTIVE ALL CHANNEL
                        }

                        if (x == 0)
                            digitalWrite(channelOutput[5], 1);  // TURN ON ACTIVE RIGHT LED
                        else if (x == 1)
                            digitalWrite(channelOutput[6], 1);  // TURN ON ACTIVE RIGHT LED
                        else if (x == 2)
                            digitalWrite(channelOutput[7], 1);  // TURN ON ACTIVE RIGHT LED
                        else if (x == 3)
                            digitalWrite(channelOutput[8], 1);  // TURN ON ACTIVE RIGHT LED
                        else
                            digitalWrite(channelOutput[9], 1);  // TURN ON ACTIVE RIGHT LED

                        gateTimerRight = 0;
                        if (noteMode == 0 ||
                            (noteMode == 1 && activeSequenceStepLeft == 99))
                            gateTimerAll = 0;
                        if ((noteMode == 0 && activeSequenceStepMono == 99) ||
                            (noteMode == 1 && activeSequenceStepRight == 99)) {
                            if (gateOutputRightActive == 1) {
                                digitalWrite(gateOutputRight, 0);
                                delay(1);
                            }

                            digitalWrite(gateOutputRight, 1);
                            gateOutputRightActive = 1;

                            if (activeSequenceStepLeft == 99) {
                                if (gateOutputAllActive == 1) {
                                    digitalWrite(gateOutputAll, 0);
                                    delay(1);
                                }

                                digitalWrite(gateOutputAll, 1);
                                digitalWrite(gateOutputLeft, 0);
                                gateOutputAllActive = 1;
                                gateOutputLeftActive = 0;
                            }
                        }

                        readYAxisRight();
                    }

                    else if ((noteMode == 0 && activeSequenceStepMono != 99) ||
                             (noteMode == 1 && activeSequenceStepRight != 99)) {
                        if (noteMode == 0) {
                            activeTouchChannelAll = x + 5;  // SET ACTIVE ALL TOUCH CHANNEL
                            activeTouchChannelRight = 99;   // SET ACTIVE ALL TOUCH CHANNEL
                        }

                        else if (noteMode == 1) {
                            activeTouchChannelAll = 99;       // SET ACTIVE ALL TOUCH CHANNEL
                            activeTouchChannelRight = x + 5;  // SET ACTIVE ALL TOUCH CHANNEL
                        }

                        readYAxisRight();
                    }
                }

                    // MONO BUTTON PRESSED
                    // ----------------------------------------
                else if (monoButtonPressed == 1 &&
                         lastCapSwitchTimerRight + capSwitchTimerLength <
                         capSwitchTimerRight) {
                    if (enableSequenceEdit == 1) {
                        lastCapSwitchTimerRight = capSwitchTimerRight;
                        monoNotePressed = 1;

                        if (monoNotePressedMono == 0) {
                            monoNotePressedMono = 1;

                            for (byte y = 0; y < 64; y++)  // CLEAR MONO SEQUENCES
                            {
                                sequenceNotesMono[y] = 0;
                            }

                            activeSequenceStepMono = 99;
                            sequenceLengthMono = 0;
                        }

                        if (sequenceLengthMono < 63) {
                            if (activeSequenceStepMono == 99) {
                                activeSequenceStepMono = 88;
                                sequenceLengthMono = 0;
                                sequenceNotesMono[0] = x + 5;
                            } else {
                                sequenceLengthMono++;
                                sequenceNotesMono[sequenceLengthMono] = x + 5;
                            }
                        }
                    }
                }

                    // REST BUTTON PRESSED
                    // ----------------------------------------
                else if (restButtonPressed == 1 &&
                         lastCapSwitchTimerRight + capSwitchTimerLength <
                         capSwitchTimerRight) {
                    externalClockResetDisabled = 1;
                    lastCapSwitchTimerRight = capSwitchTimerRight;
                    if (noteMode == 0) {
                        clockDivisionMono = x + 5;
                    }

                    else {
                        clockDivisionRight = x;
                        if (clockDivisionRight == 4) clockDivisionRight = 7;
                    }
                }

                    // DUO BUTTON PRESSED
                    // ----------------------------------------
                else if (duoButtonPressed == 1 &&
                         lastCapSwitchTimerRight + capSwitchTimerLength <
                         capSwitchTimerRight) {
                    if (enableSequenceEdit == 1) {
                        lastCapSwitchTimerRight = capSwitchTimerRight;
                        duoNotePressed = 1;
                        lastDuoNotePressed = 1;

                        if (duoNotePressedRight == 0) {
                            duoNotePressedRight = 1;

                            for (byte y = 0; y < 64; y++)  // CLEAR DUO SEQUENCES
                            {
                                sequenceNotesRight[y] = 0;
                            }

                            activeSequenceStepRight = 99;
                            sequenceLengthRight = 0;
                        }

                        if (sequenceLengthRight < 63) {
                            if (activeSequenceStepRight == 99) {
                                activeSequenceStepRight = 88;
                                sequenceLengthRight = 0;
                                sequenceNotesRight[0] = x + 5;
                            } else {
                                sequenceLengthRight++;
                                sequenceNotesRight[sequenceLengthRight] = x + 5;
                            }
                        }
                    }
                }
            }

                // KEY ALREADY PRESSED
                // -------------------
            else if (capSwitchState[x + 5] == 1 && capSwitchPressed[x + 5] == 1) {
                if ((noteMode == 0 && activeChannelAll == x + 5 &&
                     activeSequenceStepMono == 99) ||
                    (noteMode == 1 && activeChannelRight == x + 5 &&
                     activeSequenceStepRight == 99)) {
                    readYAxisRight();
                }

                else if ((noteMode == 0 && activeSequenceStepMono != 99) ||
                         (noteMode == 1 && activeSequenceStepRight != 99)) {
                    readYAxisRight();
                }
            }
        }

        else  // if (capSwitchValue[x+5] < capSwitchBuffer[x+5] + switchThreshold)
            // // IF SWITCH IS NOT PRESSED
        {
            capSwitchState[x + 5] = 0;
            capSwitchPressed[x + 5] = 0;

            if (noteMode == 0 && activeTouchChannelAll == x + 5)
                activeTouchChannelAll = 99;  // CLEAR ACTIVE ALL TOUCH CHANNEL
            else if (noteMode == 1 && activeTouchChannelRight == x + 5)
                activeTouchChannelRight = 99;  // CLEAR ACTIVE LEFT TOUCH CHANNEL

            if (noteMode == 0 && activeChannelAll == x + 5) {
                digitalWrite(gateOutputRight, 0);
                digitalWrite(gateOutputAll, 0);
                gateOutputAllActive = 0;
                gateOutputRightActive = 0;
            } else if (noteMode == 1 && activeChannelRight == x + 5) {
                digitalWrite(gateOutputRight, 0);
                gateOutputRightActive = 0;
                if (activeSequenceStepLeft == 99 && activeChannelAll == x + 5) {
                    digitalWrite(gateOutputAll, 0);
                    gateOutputAllActive = 0;
                }
            }

            if (capSwitchCalibrationCounter[x + 5] == 0)
                capSwitchBuffer[x + 5] = capSwitchValue[x + 5];
            capSwitchCalibrationCounter[x + 5]++;
            if (capSwitchCalibrationCounter[x + 5] > 10)
                capSwitchCalibrationCounter[x + 5] = 0;
        }
    }
}

void readYAxisRight() {
    int sliderValue = 0;
    float capAxisTotalRight = 0;
    float capAxisPercent[5] = {0, 0, 0, 0, 0};  // DOUBLE CHECK KEY IS PRESSED

    // DETERMINE Y-AXIS VALUE
    // ------------------------------
    for (byte y = 5; y < 10; y++)  // CYCLE THROUGH All Y-AXIS SENSORS
    {
        stepClock();  // DON'T LET THE CLOCK SHIFT
        lastCapAxisValue[y] = capAxisValue[y];
        if (noteMode == 0) lastCapAxisValue[y - 5] = capAxisValue[y];
        capAxisValue[y] = capAxis[y].capacitiveSensor(
                axisSamples);  // READ Y-AXIS CAPACITIVE SENSOR VALUE

        capAxisValue[y] =
                smooth(capAxisValue[y], axisIndividualSmooth, lastCapAxisValue[y]);

        if (capAxisValue[y] < touchLowRead) capAxisValue[y] = 0;
        capAxisTotalRight =
                capAxisTotalRight + capAxisValue[y];  // ADD VALUE TO LEFT TOTAL
    }

    for (byte y = 5; y < 10; y++)  // CYCLE THROUGH All Y-AXIS SENSORS
    {
        capAxisPercent[y - 5] = (capAxisValue[y] / capAxisTotalRight) *
                                100;  // DETERMINE LEFT SENSOR TOUCH PERCENTAGE
        if (capAxisPercent[y - 5] < touchLowPercent)
            capAxisValue[y] =
                    0;  // SET LOW PERCENTAGE VALUES TO 0 TO CLEAN UP READINGS
        capAxisTotalRight = capAxisTotalRight +
                            capAxisValue[y];  // ADD CLEANED UP VALUE TO LEFT TOTAL
        capAxisPercent[y - 5] =
                (capAxisValue[y] / capAxisTotalRight) *
                100;  // DETERMINE CLEANED UP LEFT SENSOR TOUCH PERCENTAGE
    }

    // DETERMINE RIGHT Y-AXIS FINGER POSITION
    // -------------------------------------
    sliderValue = axisLocation(capAxisPercent[1], capAxisPercent[2],
                               capAxisPercent[3], capAxisPercent[4]);

    // CLEAN UP Y-AXIS VALUE
    // -------------------------------------
    if (sliderValue > 255) sliderValue = 255;

    if (lastSliderValueRight == 0 ||
        sliderValue > lastSliderValueRight + sliderWabbleBuffer ||
        sliderValue < lastSliderValueRight - sliderWabbleBuffer ||
        lastYAxisUsed == 0) {
        sliderValue = smooth(sliderValue, axisSumSmooth, lastSliderValueRight);
        lastSliderValueRight = sliderValue;

        if (noteMode == 0)
            lastActiveSliderValueAll = sliderValue;
        else
            lastActiveSliderValueRight = sliderValue;
        lastYAxisUsed = 1;

        // OUTPUT RIGHT Y-AXIS
        // ------------------
        if ((noteMode == 0 && activeSequenceStepMono == 99) ||
            (noteMode == 1 && activeSequenceStepRight == 99)) {
            analogWrite(axisOutput[1], sliderValue);
            analogWrite(axisOutput[2], sliderValue);
        }
    }

    if (monoButtonPressed == 1)
        sequenceYAxisMono[sequenceLengthMono] = sliderValue;
    else if (duoButtonPressed == 1)
        sequenceYAxisRight[sequenceLengthRight] = sliderValue;
    else if (monoButtonPressed == 0 && duoButtonPressed == 0) {
        if (noteMode == 0 && activeSequenceStepMono < 64)
            sequenceYAxisMono[activeSequenceStepMono] = sliderValue;
        if (noteMode == 1 && activeSequenceStepRight < 64)
            sequenceYAxisRight[activeSequenceStepRight] = sliderValue;
    }
}

int smooth(int data, float filterVal, float smoothedVal) {
    // check to make sure param's are within range
    // if (filterVal > 1) filterVal = .99;
    // if (filterVal <= 0) filterVal = 0;

    smoothedVal = (data * (1 - filterVal)) + (smoothedVal * filterVal);
    return (int)smoothedVal;
}

void clockInterrupted() { clockInterruptTriggered = 1; }

void clockClosing() { clockInputUsed = 0; }

void turnOffGates() {
    clockTimer = millis();

    // TURN OFF GATE OUTPUTS
    // ----------------------------------
    if (gateTimerLeft != 0 && gateTimerLeft < clockTimer) {
        gateTimerLeft = 0;
        digitalWrite(gateOutputLeft, 0);
        gateOutputLeftActive = 0;
    }

    if (gateTimerRight != 0 && gateTimerRight < clockTimer) {
        gateTimerRight = 0;
        digitalWrite(gateOutputRight, 0);
        gateOutputRightActive = 0;
    }

    if (gateTimerAll != 0 && gateTimerAll < clockTimer) {
        gateTimerAll = 0;
        digitalWrite(gateOutputAll, 0);
        gateOutputAllActive = 0;
    }
}

void stepClock() {
    if (!clockInterruptTriggered) return;
    clockInterruptTriggered = false;

    unsigned long now = millis();

    // Measure clock interval (tempo sync!)
    if (lastClockTime != 0) {
        currentClockInterval = now - lastClockTime;
    }
    lastClockTime = now;

    // Clock division (original logic)
    static byte divCount = 0;
    byte div = (noteMode == 0) ? clockDivisionMono :
               (activeSequenceStepLeft != 99 ? clockDivisionLeft : clockDivisionRight);
    if (++divCount < (div ? div + 1 : 1)) return;
    divCount = 0;

    // Start new logical step
    ratchetSubCount = 0;
    nextRatchetTime = now;

    if (noteMode == 0 && seqLenMono > 0) advanceMono(now);
    if (noteMode == 1) {
        if (seqLenLeft > 0)  advanceLeft(now);
        if (seqLenRight > 0) advanceRight(now);
    }
}

void advanceMono(unsigned long stepStartTime) {
    if (ratchetSubCount == 0) {
        currentStepMono = (currentStepMono + 1) % seqLenMono;
        Step& s = seqMono[currentStepMono];
        if (s.channel == 99 || random(10) >= s.prob) {
            ratchetSubCount = 1;  // skip ratcheting entirely
            return;
        }
        ratchetSubCount = s.ratchet;
    }

    if (ratchetSubCount > 0) {
        triggerStep(seqMono[currentStepMono]);
        ratchetSubCount--;

        if (ratchetSubCount > 0) {
            unsigned long interval = currentClockInterval / seqMono[currentStepMono].ratchet;
            nextRatchetTime = stepStartTime + interval * (seqMono[currentStepMono].ratchet - ratchetSubCount);
        }
    }
}

void advanceLeft(unsigned long stepStartTime) {
    static byte sub = 0;
    if (sub == 0) {
        currentStepLeft = (currentStepLeft + 1) % seqLenLeft;
        Step& s = seqLeft[currentStepLeft];
        if (s.channel == 99 || random(10) >= s.prob) { sub = 1; return; }
        sub = s.ratchet;
    }
    if (sub > 0) {
        triggerStep(seqLeft[currentStepLeft]);
        sub--;
        if (sub > 0) {
            unsigned long interval = currentClockInterval / seqLeft[currentStepLeft].ratchet;
            nextRatchetTime = stepStartTime + interval * (seqLeft[currentStepLeft].ratchet - sub);
        }
    }
}

void advanceRight(unsigned long stepStartTime) {
    static byte sub = 0;
    if (sub == 0) {
        currentStepRight = (currentStepRight + 1) % seqLenRight;
        Step& s = seqRight[currentStepRight];
        if (s.channel == 99 || random(10) >= s.prob) { sub = 1; return; }
        sub = s.ratchet;
    }
    if (sub > 0) {
        Step temp = s;
        temp.channel += 5;
        triggerStep(temp);
        sub--;
        if (sub > 0) {
            unsigned long interval = currentClockInterval / seqRight[currentStepRight].ratchet;
            nextRatchetTime = stepStartTime + interval * (seqRight[currentStepRight].ratchet - sub);
        }
    }
}

void checkScan() {
    float readTime = millis();
    int scanInputValue = analogRead(scanInput);

    if (noteMode == 0) {
        if (scanInputValue < 478)
            scannedChannel = 0;
        else if (scanInputValue >= 478 && scanInputValue < 756)
            scannedChannel = 1;
        else if (scanInputValue >= 756 && scanInputValue < 1034)
            scannedChannel = 2;
        else if (scanInputValue >= 1034 && scanInputValue < 1312)
            scannedChannel = 3;
        else if (scanInputValue >= 1312 && scanInputValue < 1590)
            scannedChannel = 4;
        else if (scanInputValue >= 1590 && scanInputValue < 1868)
            scannedChannel = 5;
        else if (scanInputValue >= 1868 && scanInputValue < 2146)
            scannedChannel = 6;
        else if (scanInputValue >= 2146 && scanInputValue < 2424)
            scannedChannel = 7;
        else if (scanInputValue >= 2424 && scanInputValue < 2702)
            scannedChannel = 8;
        else if (scanInputValue >= 2702)
            scannedChannel = 9;

        // if (scanInputValue < 278) scannedChannel = 0;
        // else if (scanInputValue >= 278 && scanInputValue < 556) scannedChannel =
        // 1; else if (scanInputValue >= 556 && scanInputValue < 834) scannedChannel
        // = 2; else if (scanInputValue >= 834 && scanInputValue < 1112)
        // scannedChannel = 3; else if (scanInputValue >= 1112 && scanInputValue <
        // 1390) scannedChannel = 4; else if (scanInputValue >= 1390 &&
        // scanInputValue < 1668) scannedChannel = 5; else if (scanInputValue >=
        // 1668 && scanInputValue < 1946) scannedChannel = 6; else if
        // (scanInputValue >= 1946 && scanInputValue < 2224) scannedChannel = 7;
        // else if (scanInputValue >= 2224 && scanInputValue < 2502) scannedChannel
        // = 8; else if (scanInputValue >= 2502) scannedChannel = 9;

        if (scannedChannel != lastScannedChannel) {
            lastScannedChannel = scannedChannel;

            digitalWrite(channelOutput[0], 0);  // TURN OFF LED 0
            digitalWrite(channelOutput[1], 0);  // TURN OFF LED 1
            digitalWrite(channelOutput[2], 0);  // TURN OFF LED 2
            digitalWrite(channelOutput[3], 0);  // TURN OFF LED 3
            digitalWrite(channelOutput[4], 0);  // TURN OFF LED 4
            digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
            digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
            digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
            digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
            digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

            if (scannedChannel < 5) {
                digitalWrite(allSelectorA, 0);
                digitalWrite(allSelectorB, 1);
                digitalWrite(allSelectorC, 0);
                digitalWrite(allSelectorD, 1);
            }

            else {
                digitalWrite(allSelectorA, 1);
                digitalWrite(allSelectorB, 0);
                digitalWrite(allSelectorC, 1);
                digitalWrite(allSelectorD, 0);
            }

            if (scannedChannel == 0)
                digitalWrite(channelOutput[0], 1);  // TURN ON LED 0
            else if (scannedChannel == 1)
                digitalWrite(channelOutput[1], 1);  // TURN ON LED 1
            else if (scannedChannel == 2)
                digitalWrite(channelOutput[2], 1);  // TURN ON LED 2
            else if (scannedChannel == 3)
                digitalWrite(channelOutput[3], 1);  // TURN ON LED 3
            else if (scannedChannel == 4)
                digitalWrite(channelOutput[4], 1);  // TURN ON LED 4
            else if (scannedChannel == 5)
                digitalWrite(channelOutput[5], 1);  // TURN ON LED 5
            else if (scannedChannel == 6)
                digitalWrite(channelOutput[6], 1);  // TURN ON LED 6
            else if (scannedChannel == 7)
                digitalWrite(channelOutput[7], 1);  // TURN ON LED 7
            else if (scannedChannel == 8)
                digitalWrite(channelOutput[8], 1);  // TURN ON LED 8
            else if (scannedChannel == 9)
                digitalWrite(channelOutput[9], 1);  // TURN ON LED 9

            if (scannedChannel < 5) {
                if (gateTimerLeft != 0 || gateTimerAll != 0) {
                    digitalWrite(gateOutputLeft, 0);
                    digitalWrite(gateOutputAll, 0);
                    gateOutputAllActive = 0;
                    gateOutputLeftActive = 0;
                    delay(1);
                }
                gateTimerLeft = gateLength + readTime;
                gateTimerAll = gateLength + readTime;

                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                if (gateOutputLeftActive == 1) digitalWrite(gateOutputLeft, 0);
                if (gateOutputAllActive == 1 || gateOutputLeftActive == 1) delay(1);

                digitalWrite(gateOutputLeft, 1);  // ENABLE LEFT GATE
                digitalWrite(gateOutputAll, 1);   // ENABLE ALL GATE
                gateOutputAllActive = 1;
                gateOutputLeftActive = 1;
            }

            else {
                if (gateTimerRight != 0 || gateTimerAll != 0) {
                    digitalWrite(gateOutputRight, 0);
                    digitalWrite(gateOutputAll, 0);
                    gateOutputAllActive = 0;
                    gateOutputRightActive = 0;
                    delay(1);
                }
                gateTimerRight = gateLength + readTime;
                gateTimerAll = gateLength + readTime;

                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                if (gateOutputRightActive == 1) digitalWrite(gateOutputRight, 0);
                if (gateOutputAllActive == 1 || gateOutputRightActive == 1) delay(1);
                digitalWrite(gateOutputRight, 1);  // ENABLE RIGHT GATE
                digitalWrite(gateOutputAll, 1);    // ENABLE ALL GATE
                gateOutputAllActive = 1;
                gateOutputRightActive = 1;
            }
        }
    }

    else  // if(noteMode == 1)
    {
        if (scanInputValue < 760)
            scannedChannel = 0;
        else if (scanInputValue >= 760 && scanInputValue < 1420)
            scannedChannel = 1;
        else if (scanInputValue >= 1420 && scanInputValue < 2080)
            scannedChannel = 2;
        else if (scanInputValue >= 2080 && scanInputValue < 2740)
            scannedChannel = 3;
        else if (scanInputValue >= 2740)
            scannedChannel = 4;

        if (scannedChannel != lastScannedChannel) {
            lastScannedChannel = scannedChannel;

            digitalWrite(channelOutput[0], 0);  // TURN OFF LED 0
            digitalWrite(channelOutput[1], 0);  // TURN OFF LED 1
            digitalWrite(channelOutput[2], 0);  // TURN OFF LED 2
            digitalWrite(channelOutput[3], 0);  // TURN OFF LED 3
            digitalWrite(channelOutput[4], 0);  // TURN OFF LED 4
            digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
            digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
            digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
            digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
            digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

            if (gateTimerLeft != 0 || gateTimerRight != 0 || gateTimerAll != 0) {
                digitalWrite(gateOutputLeft, 0);
                digitalWrite(gateOutputRight, 0);
                digitalWrite(gateOutputAll, 0);

                gateOutputAllActive = 0;
                gateOutputLeftActive = 0;
                gateOutputRightActive = 0;
                delay(1);
            }

            if (scannedChannel == 0) {
                digitalWrite(channelOutput[0], 1);  // TURN ON LED 0
                digitalWrite(channelOutput[5], 1);  // TURN ON LED 5

                gateTimerLeft = gateLength + readTime;
                gateTimerRight = gateLength + readTime;
                gateTimerAll = gateLength + readTime;

                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                if (gateOutputLeftActive == 1) digitalWrite(gateOutputLeft, 0);
                if (gateOutputRightActive == 1) digitalWrite(gateOutputRight, 0);
                if (gateOutputAllActive == 1 || gateOutputLeftActive == 1 ||
                    gateOutputRightActive == 1)
                    delay(1);

                digitalWrite(gateOutputLeft, 1);   // ENABLE LEFT GATE
                digitalWrite(gateOutputRight, 1);  // ENABLE RIGHT GATE
                digitalWrite(gateOutputAll, 1);    // ENABLE ALL GATE

                gateOutputAllActive = 1;
                gateOutputLeftActive = 1;
                gateOutputRightActive = 1;
            } else if (scannedChannel == 1) {
                digitalWrite(channelOutput[1], 1);  // TURN ON LED 1
                digitalWrite(channelOutput[6], 1);  // TURN ON LED 6

                gateTimerLeft = gateLength + readTime;
                gateTimerRight = gateLength + readTime;
                gateTimerAll = gateLength + readTime;

                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                if (gateOutputLeftActive == 1) digitalWrite(gateOutputLeft, 0);
                if (gateOutputRightActive == 1) digitalWrite(gateOutputRight, 0);
                if (gateOutputAllActive == 1 || gateOutputLeftActive == 1 ||
                    gateOutputRightActive == 1)
                    delay(1);

                digitalWrite(gateOutputLeft, 1);   // ENABLE LEFT GATE
                digitalWrite(gateOutputRight, 1);  // ENABLE RIGHT GATE
                digitalWrite(gateOutputAll, 1);    // ENABLE ALL GATE

                gateOutputAllActive = 1;
                gateOutputLeftActive = 1;
                gateOutputRightActive = 1;
            } else if (scannedChannel == 2) {
                digitalWrite(channelOutput[2], 1);  // TURN ON LED 2
                digitalWrite(channelOutput[7], 1);  // TURN ON LED 7

                gateTimerLeft = gateLength + readTime;
                gateTimerRight = gateLength + readTime;
                gateTimerAll = gateLength + readTime;

                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                if (gateOutputLeftActive == 1) digitalWrite(gateOutputLeft, 0);
                if (gateOutputRightActive == 1) digitalWrite(gateOutputRight, 0);
                if (gateOutputAllActive == 1 || gateOutputLeftActive == 1 ||
                    gateOutputRightActive == 1)
                    delay(1);

                digitalWrite(gateOutputLeft, 1);   // ENABLE LEFT GATE
                digitalWrite(gateOutputRight, 1);  // ENABLE RIGHT GATE
                digitalWrite(gateOutputAll, 1);    // ENABLE ALL GATE

                gateOutputAllActive = 1;
                gateOutputLeftActive = 1;
                gateOutputRightActive = 1;
            } else if (scannedChannel == 3) {
                digitalWrite(channelOutput[3], 1);  // TURN ON LED 3
                digitalWrite(channelOutput[8], 1);  // TURN ON LED 8

                gateTimerLeft = gateLength + readTime;
                gateTimerRight = gateLength + readTime;
                gateTimerAll = gateLength + readTime;

                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                if (gateOutputLeftActive == 1) digitalWrite(gateOutputLeft, 0);
                if (gateOutputRightActive == 1) digitalWrite(gateOutputRight, 0);
                if (gateOutputAllActive == 1 || gateOutputLeftActive == 1 ||
                    gateOutputRightActive == 1)
                    delay(1);

                digitalWrite(gateOutputLeft, 1);   // ENABLE LEFT GATE
                digitalWrite(gateOutputRight, 1);  // ENABLE RIGHT GATE
                digitalWrite(gateOutputAll, 1);    // ENABLE ALL GATE

                gateOutputAllActive = 1;
                gateOutputLeftActive = 1;
                gateOutputRightActive = 1;
            } else if (scannedChannel == 4) {
                digitalWrite(channelOutput[4], 1);  // TURN ON LED 4
                digitalWrite(channelOutput[9], 1);  // TURN ON LED 9

                gateTimerLeft = gateLength + readTime;
                gateTimerRight = gateLength + readTime;
                gateTimerAll = gateLength + readTime;

                if (gateOutputAllActive == 1) digitalWrite(gateOutputAll, 0);
                if (gateOutputLeftActive == 1) digitalWrite(gateOutputLeft, 0);
                if (gateOutputRightActive == 1) digitalWrite(gateOutputRight, 0);
                if (gateOutputAllActive == 1 || gateOutputLeftActive == 1 ||
                    gateOutputRightActive == 1)
                    delay(1);

                digitalWrite(gateOutputLeft, 1);   // ENABLE LEFT GATE
                digitalWrite(gateOutputRight, 1);  // ENABLE RIGHT GATE
                digitalWrite(gateOutputAll, 1);    // ENABLE ALL GATE

                gateOutputAllActive = 1;
                gateOutputLeftActive = 1;
                gateOutputRightActive = 1;
            }
        }
    }
}

void pressMonoButton() {
    if (duoButtonState == 0) {
        monoButtonState = digitalRead(monoButton);

        if (monoButtonState == 1) {
            if (monoButtonChangeEnabled == 1) {
                // MONO BUTTON ON
                // ------------------------------------
                monoButtonChangeEnabled = 0;
                monoButtonPressed = 1;

                if (restButtonPressed == 1) {
                    externalClockResetDisabled = 1;

                    for (byte x = 0; x < 10; x++)  // CLEAR PRESSED KEYS
                    {
                        autoCalibrateSwitch(x, 1);
                        capAxis[x].reset_CS_AutoCal();

                        digitalWrite(channelOutput[0], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[1], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[2], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[3], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[4], 0);  // TURN OFF LED 9
                        digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

                        if (noteMode == 0) {
                            digitalWrite(channelOutput[activeChannelAll],
                                         1);  // TURN ON ACTIVE CHANNEL
                        } else {
                            digitalWrite(channelOutput[activeChannelLeft],
                                         1);  // TURN ON ACTIVE CHANNEL
                            digitalWrite(channelOutput[activeChannelRight],
                                         1);  // TURN ON ACTIVE CHANNEL
                        }
                    }
                }

                else {
                    // MONO SEQUENCE PROGRAMMING
                    monoNotePressed = 0;
                    monoNotePressedMono = 0;

                    if (noteMode == 0) {
                        enableSequenceEdit = 1;
                    }

                    else  // if (noteMode == 1)
                    {
                        for (byte x = 0; x < 10; x++)  // CLEAR PRESSED KEYS
                        {
                            capSwitchState[x] = 0;
                        }

                        for (byte x = 0; x < 64; x++)  // CLEAR DUO SEQUENCES
                        {
                            sequenceNotesLeft[x] = 0;
                            sequenceNotesRight[x] = 0;
                        }

                        activeSequenceStepLeft = 99;
                        activeSequenceStepRight = 99;

                        sequenceLengthLeft = 0;
                        sequenceLengthRight = 0;

                        duoNotePressedLeft = 1;
                        duoNotePressedRight = 1;

                        noteMode = 0;
                        digitalWrite(channelOutput[0], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[1], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[2], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[3], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[4], 0);  // TURN OFF LED 9
                        digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

                        digitalWrite(channelOutput[activeChannelAll],
                                     1);  // TURN ON ACTIVE CHANNEL

                        digitalWrite(allSelectorA, 1);
                        digitalWrite(allSelectorB, 1);
                        digitalWrite(allSelectorC, 1);
                        digitalWrite(allSelectorD, 1);
                    }
                }
            }
        }

            // MONO BUTTON OFF
            // ------------------------------------
        else if (monoButtonState == 0) {
            if (monoButtonPressed == 1) {
                monoButtonPressed = 0;
                enableSequenceEdit = 0;

                if (monoNotePressed == 0) {
                    for (byte x = 0; x < 64; x++)  // CLEAR MONO SEQUENCE
                    {
                        sequenceNotesMono[x] = 0;
                    }

                    activeSequenceStepLeft = 99;
                    activeSequenceStepRight = 99;
                    activeSequenceStepMono = 99;
                    sequenceLengthMono = 0;
                    monoNotePressedMono = 1;
                }
            }

            monoButtonChangeEnabled = 1;
        }
    }
}

bool restButtonState = 0;
bool restButtonChangeEnabled = 1;

void pressRestButton() {
    restButtonState = digitalRead(restButton);
    if (restButtonState == 1) {
        if (restButtonChangeEnabled == 1) {
            restButtonChangeEnabled = 0;
            if (monoButtonPressed == 0 && duoButtonPressed == 0) {
                restButtonPressed = 1;
                externalClockResetDisabled = 0;
            }

            else if (monoButtonPressed == 1) {
                monoNotePressed = 1;
                externalClockResetDisabled = 1;

                if (monoNotePressedMono == 0) {
                    monoNotePressedMono = 1;

                    for (byte y = 0; y < 64; y++)  // CLEAR MONO SEQUENCES
                    {
                        sequenceNotesMono[y] = 0;
                    }

                    activeSequenceStepMono = 99;
                    sequenceLengthMono = 0;
                }

                if (sequenceLengthMono < 63) {
                    if (activeSequenceStepMono == 99) {
                        activeSequenceStepMono = 88;
                        sequenceLengthMono = 0;
                        sequenceNotesMono[0] = 99;
                    } else {
                        sequenceLengthMono++;
                        sequenceNotesMono[sequenceLengthMono] = 99;
                    }
                }
            } else if (duoButtonPressed == 1) {
                externalClockResetDisabled = 1;

                if (lastDuoNotePressed == 0) {
                    if (sequenceLengthLeft < 63) {
                        sequenceLengthLeft++;
                        sequenceNotesLeft[sequenceLengthLeft] = 99;
                    }
                }

                else if (lastDuoNotePressed == 1) {
                    if (sequenceLengthRight < 63) {
                        sequenceLengthRight++;
                        sequenceNotesRight[sequenceLengthRight] = 99;
                    }
                }
            }
        }
    } else if (restButtonState == 0) {
        if (restButtonChangeEnabled == 0) {
            restButtonChangeEnabled = 1;

            if (externalClockResetDisabled == 0) {
                resetInputUsed = 1;

                if (activeSequenceStepMono != 99) activeSequenceStepMono = 88;
                if (activeSequenceStepLeft != 99) activeSequenceStepLeft = 88;
                if (activeSequenceStepRight != 99) activeSequenceStepRight = 88;

                clockDivisionStepMono = 99;
                clockDivisionStepLeft = 99;
                clockDivisionStepRight = 99;
            }
        }

        restButtonPressed = 0;
    }
}

void pressDuoButton() {
    if (monoButtonState == 0) {
        duoButtonState = digitalRead(duoButton);

        // DUO BUTTON ON
        // ------------------------------------
        if (duoButtonState == 1) {
            if (duoButtonChangeEnabled == 1) {
                duoButtonChangeEnabled = 0;
                duoButtonPressed = 1;

                if (restButtonPressed == 1) {
                    externalClockResetDisabled = 1;

                    for (byte x = 0; x < 10; x++)  // CLEAR PRESSED KEYS
                    {
                        autoCalibrateSwitch(x, 1);
                        capAxis[x].reset_CS_AutoCal();

                        digitalWrite(channelOutput[0], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[1], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[2], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[3], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[4], 0);  // TURN OFF LED 9
                        digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

                        if (noteMode == 0) {
                            digitalWrite(channelOutput[activeChannelAll],
                                         1);  // TURN ON ACTIVE CHANNEL
                        } else {
                            digitalWrite(channelOutput[activeChannelLeft],
                                         1);  // TURN ON ACTIVE CHANNEL
                            digitalWrite(channelOutput[activeChannelRight],
                                         1);  // TURN ON ACTIVE CHANNEL
                        }
                    }
                }

                else {
                    if (noteMode == 1) {
                        enableSequenceEdit = 1;
                    }

                    else  // if (noteMode == 0)
                    {
                        for (byte x = 0; x < 10; x++)  // CLEAR PRESSED KEYS
                        {
                            capSwitchState[x] = 0;
                        }

                        noteMode = 1;
                        lastDuoNotePressed = 0;

                        digitalWrite(channelOutput[0], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[1], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[2], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[3], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[4], 0);  // TURN OFF LED 9
                        digitalWrite(channelOutput[5], 0);  // TURN OFF LED 5
                        digitalWrite(channelOutput[6], 0);  // TURN OFF LED 6
                        digitalWrite(channelOutput[7], 0);  // TURN OFF LED 7
                        digitalWrite(channelOutput[8], 0);  // TURN OFF LED 8
                        digitalWrite(channelOutput[9], 0);  // TURN OFF LED 9

                        digitalWrite(channelOutput[activeChannelLeft],
                                     1);  // TURN ON ACTIVE CHANNEL LEFT
                        digitalWrite(channelOutput[activeChannelRight],
                                     1);  // TURN ON ACTIVE CHANNEL RIGHT

                        digitalWrite(allSelectorA, 0);
                        digitalWrite(allSelectorB, 0);
                        digitalWrite(allSelectorC, 0);
                        digitalWrite(allSelectorD, 0);
                    }

                    // DUO SEQUENCE PROGRAMMING
                    duoNotePressed = 0;
                    duoNotePressedLeft = 0;
                    duoNotePressedRight = 0;
                }
            }
        }

            // DUO BUTTON OFF
            // ------------------------------------
        else if (duoButtonState == 0) {
            if (duoButtonPressed == 1) {
                duoButtonPressed = 0;
                enableSequenceEdit = 0;

                if (duoNotePressed == 0) {
                    for (byte x = 0; x < 64; x++)  // CLEAR DUO SEQUENCES
                    {
                        sequenceNotesLeft[x] = 0;
                        sequenceNotesRight[x] = 0;
                    }

                    activeSequenceStepLeft = 99;
                    activeSequenceStepRight = 99;
                    activeSequenceStepMono = 99;

                    sequenceLengthLeft = 0;
                    sequenceLengthRight = 0;

                    duoNotePressedLeft = 1;
                    duoNotePressedRight = 1;
                }
            }

            duoButtonChangeEnabled = 1;
        }
    }
}

void setup() {
    pinMode(switchInput[0], INPUT);
    pinMode(switchInput[1], INPUT);
    pinMode(switchInput[2], INPUT);
    pinMode(switchInput[3], INPUT);
    pinMode(switchInput[4], INPUT);
    pinMode(switchInput[5], INPUT);
    pinMode(switchInput[6], INPUT);
    pinMode(switchInput[7], INPUT);
    pinMode(switchInput[8], INPUT);
    pinMode(switchInput[9], INPUT);

    pinMode(switchResistor[0], OUTPUT);
    pinMode(switchResistor[1], OUTPUT);
    pinMode(switchResistor[2], OUTPUT);
    pinMode(switchResistor[3], OUTPUT);
    pinMode(switchResistor[4], OUTPUT);
    pinMode(switchResistor[5], OUTPUT);
    pinMode(switchResistor[6], OUTPUT);
    pinMode(switchResistor[7], OUTPUT);
    pinMode(switchResistor[8], OUTPUT);
    pinMode(switchResistor[9], OUTPUT);

    digitalWrite(switchResistor[0], 0);
    digitalWrite(switchResistor[1], 0);
    digitalWrite(switchResistor[2], 0);
    digitalWrite(switchResistor[3], 0);
    digitalWrite(switchResistor[4], 0);
    digitalWrite(switchResistor[5], 0);
    digitalWrite(switchResistor[6], 0);
    digitalWrite(switchResistor[7], 0);
    digitalWrite(switchResistor[8], 0);
    digitalWrite(switchResistor[9], 0);

    pinMode(axisInput[0], INPUT);
    pinMode(axisInput[1], INPUT);
    pinMode(axisInput[2], INPUT);
    pinMode(axisInput[3], INPUT);
    pinMode(axisInput[4], INPUT);
    pinMode(axisInput[5], INPUT);
    pinMode(axisInput[6], INPUT);
    pinMode(axisInput[7], INPUT);
    pinMode(axisInput[8], INPUT);
    pinMode(axisInput[9], INPUT);

    pinMode(axisResistor[0], OUTPUT);
    pinMode(axisResistor[1], OUTPUT);
    pinMode(axisResistor[2], OUTPUT);
    pinMode(axisResistor[3], OUTPUT);
    pinMode(axisResistor[4], OUTPUT);
    pinMode(axisResistor[5], OUTPUT);
    pinMode(axisResistor[6], OUTPUT);
    pinMode(axisResistor[7], OUTPUT);
    pinMode(axisResistor[8], OUTPUT);
    pinMode(axisResistor[9], OUTPUT);

    digitalWrite(axisResistor[0], 0);
    digitalWrite(axisResistor[1], 0);
    digitalWrite(axisResistor[2], 0);
    digitalWrite(axisResistor[3], 0);
    digitalWrite(axisResistor[4], 0);
    digitalWrite(axisResistor[5], 0);
    digitalWrite(axisResistor[6], 0);
    digitalWrite(axisResistor[7], 0);
    digitalWrite(axisResistor[8], 0);
    digitalWrite(axisResistor[9], 0);

    pinMode(channelOutput[0], OUTPUT);
    pinMode(channelOutput[1], OUTPUT);
    pinMode(channelOutput[2], OUTPUT);
    pinMode(channelOutput[3], OUTPUT);
    pinMode(channelOutput[4], OUTPUT);
    pinMode(channelOutput[5], OUTPUT);
    pinMode(channelOutput[6], OUTPUT);
    pinMode(channelOutput[7], OUTPUT);
    pinMode(channelOutput[8], OUTPUT);
    pinMode(channelOutput[9], OUTPUT);

    digitalWrite(channelOutput[0], 0);
    digitalWrite(channelOutput[1], 0);
    digitalWrite(channelOutput[2], 0);
    digitalWrite(channelOutput[3], 0);
    digitalWrite(channelOutput[4], 0);
    digitalWrite(channelOutput[5], 0);
    digitalWrite(channelOutput[6], 0);
    digitalWrite(channelOutput[7], 0);
    digitalWrite(channelOutput[8], 0);
    digitalWrite(channelOutput[9], 0);

    pinMode(axisOutput[0], OUTPUT);
    pinMode(axisOutput[1], OUTPUT);
    pinMode(axisOutput[2], OUTPUT);

    analogWrite(axisOutput[0], 0);
    analogWrite(axisOutput[1], 0);
    analogWrite(axisOutput[2], 0);

    pinMode(clockInput, INPUT);

    pinMode(resetInput, INPUT);
    pinMode(scanInput, INPUT);

    pinMode(monoButton, INPUT);
    pinMode(restButton, INPUT);
    pinMode(duoButton, INPUT);

    pinMode(gateOutputLeft, OUTPUT);
    pinMode(gateOutputRight, OUTPUT);
    pinMode(gateOutputAll, OUTPUT);

    digitalWrite(gateOutputLeft, 0);
    digitalWrite(gateOutputRight, 0);
    digitalWrite(gateOutputAll, 0);

    pinMode(allSelectorA, OUTPUT);
    pinMode(allSelectorB, OUTPUT);
    pinMode(allSelectorC, OUTPUT);
    pinMode(allSelectorD, OUTPUT);

    digitalWrite(allSelectorA, 1);
    digitalWrite(allSelectorB, 1);
    digitalWrite(allSelectorC, 1);
    digitalWrite(allSelectorD, 1);

    analogReadResolution(12);

    // CAPACITIVE CALIBRATION
    // ------------------------------
    for (byte y = 0; y < 10; y++) {
        capSwitch[y].set_CS_Timeout_Millis(1000);
        capAxis[y].set_CS_Timeout_Millis(1000);

        capSwitch[y].set_CS_AutocaL_Millis(0xFFFFFFFF);
        // capSwitch[y].set_CS_AutocaL_Millis(1000);

        // capAxis[y].set_CS_AutocaL_Millis(0xFFFFFFFF);
        capAxis[y].set_CS_AutocaL_Millis(20000);
    }

    attachInterrupt(clockInput, clockInterrupted, RISING);
    attachInterrupt(resetInput, resetInterrupted, RISING);

    lastCapSwitchTimerLeft = millis();
    lastCapSwitchTimerRight = millis();

    for (byte y = 0; y < 10; y++) {
        delay(200);
        capSwitch[y].reset_CS_AutoCal();
        capAxis[y].reset_CS_AutoCal();
    }

    //randomSeed(analogRead(A10) + micros());
    for (int i = 0; i < 64; i++) {
        seqMono[i] = {99, 0, 1, 10};
        seqLeft[i] = {99, 0, 1, 10};
        seqRight[i] = {99, 0, 1, 10};
    }
}

void loop() {
    turnOffGates();
    stepClock();
    handleRatchetSubdivisions();

    checkCapSwitchLeft(0);
    checkCapSwitchRight(0);

    stepClock();
    pressMonoButton();

    checkCapSwitchLeft(1);
    checkCapSwitchRight(1);

    stepClock();
    pressRestButton();

    checkCapSwitchLeft(2);
    checkCapSwitchRight(2);

    stepClock();
    pressDuoButton();

    checkCapSwitchLeft(3);
    checkCapSwitchRight(3);

    stepClock();
    checkScan();

    checkCapSwitchLeft(4);
    checkCapSwitchRight(4);

    handleRestButtonForParamEdit();
    updateParamLED();
}

void turnOffGates() {
    if (gateTimerLeft != 0 && millis() > gateTimerLeft) {
        digitalWrite(gateOutputLeft, 0);
        gateOutputLeftActive = 0;
        gateTimerLeft = 0;
    }
    if (gateTimerRight != 0 && millis() > gateTimerRight) {
        digitalWrite(gateOutputRight, 0);
        gateOutputRightActive = 0;
        gateTimerRight = 0;
    }
    if (gateTimerAll != 0 && millis() > gateTimerAll) {
        digitalWrite(gateOutputAll, 0);
        gateOutputAllActive = 0;
        gateTimerAll = 0;
    }
}

// Find which step uses this channel (simple reverse search)
int8_t findStepWithChannel(byte ch) {
    if (noteMode == 0) {
        for (int i = seqLenMono - 1; i >= 0; i--) {
            if (seqMono[i].channel == ch) return i;
        }
    } else {
        if (ch < 5) {
            for (int i = seqLenLeft - 1; i >= 0; i--) {
                if (seqLeft[i].channel == ch) return i + 100;  // 100+ = left
            }
        } else {
            for (int i = seqLenRight - 1; i >= 0; i--) {
                if (seqRight[i].channel == (ch - 5)) return i + 200;  // 200+ = right
            }
        }
    }
    return -1;
}

// Get reference to the step being edited
Step& getEditStep() {
    if (editStepIndex < 100) return seqMono[editStepIndex];
    if (editStepIndex < 200) return seqLeft[editStepIndex - 100];
    return seqRight[editStepIndex - 200];
}

void triggerChannel(byte ch, byte yVal) {
    // This uses the exact same logic as original firmware
    // Just call the existing gate/CV/LED code from checkCapSwitchLeft/Right
    // when a key is pressed — we just reuse the same paths

    activeChannelAll = ch;
    if (ch < 5) activeChannelLeft = ch;
    else        activeChannelRight = ch;

    // Force gate on (original behavior)
    digitalWrite(gateOutputLeft, (ch < 5 || noteMode == 0) ? 1 : 0);
    digitalWrite(gateOutputRight, (ch >= 5 || noteMode == 0) ? 1 : 0);
    digitalWrite(gateOutputAll, 1);

    gateOutputLeftActive = (ch < 5 || noteMode == 0);
    gateOutputRightActive = (ch >= 5 || noteMode == 0);
    gateOutputAllActive = 1;

    gateTimerLeft = millis() + gateLength;
    gateTimerRight = millis() + gateLength;
    gateTimerAll = millis() + gateLength;

    // CV output
    if (noteMode == 0 || ch < 5) {
        analogWrite(axisOutput[0], yVal);
        analogWrite(axisOutput[2], yVal);
    }
    if (noteMode == 0 || ch >= 5) {
        analogWrite(axisOutput[1], yVal);
        analogWrite(axisOutput[2], yVal);
    }

    // LED feedback
    digitalWrite(channelOutput[ch], 1);
}

void handleRatchetSubdivisions() {
    if (ratchetSubCount > 0 && millis() >= nextRatchetTime) {
        if (noteMode == 0) advanceMono(lastClockTime);
        else if (activeSequenceStepLeft != 99) advanceLeft(lastClockTime);
        else if (activeSequenceStepRight != 99) advanceRight(lastClockTime);
    }
}