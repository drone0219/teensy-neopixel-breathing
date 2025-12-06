/*
||═══════════════════════════════════════════════════════════════════════════════
|| @file       Teensy_NeoPixel.ino
|| @project    ESP32/Teensy NeoPixel Breathing LED Controller
|| @description
||   Interactive LED installation controller featuring:
||   - Finite State Machine (FSM) for smooth LED transitions
||   - 22 light sensor triggers for interactive responses
||   - Beautiful breathing effects between Standby/Active/Triggered states
||   - Video player integration via serial communication
||   - HSV color space control for smooth color transitions
||
|| @author     Your Name
|| @date       December 7, 2025
|| @version    1.0
||═══════════════════════════════════════════════════════════════════════════════
*/

// ═════════════════════════ LIBRARY INCLUDES ═════════════════════════
#include "FiniteStateMachine.h"  // Custom FSM library for state transitions
#include <TimeLib.h>              // Time management utilities
#include "FBD.h"                  // Function Block Diagram (PLC-style timers)
#include <FastLED.h>              // High-performance LED control library

// ═════════════════════════ LED CONFIGURATION ═════════════════════════
#define NUM_STRIPS 22                        // Total number of NeoPixel strips
#define PlayerSerial Serial1                 // Hardware serial for video player control

#define NUM_LEDS_PER_STRIP 16                // LEDs per strip (standard density)
#define NUM_LEDS (NUM_LEDS_PER_STRIP * NUM_STRIPS + 8)  // Total LEDs (+8 for driver LEDs)
CRGB leds[NUM_LEDS];                         // Global LED array buffer


// ═════════════════════════ COLOR SETTINGS (HSV) ═════════════════════════
/*
 * HSV Color Space Configuration:
 * 
 * Using HSV (Hue, Saturation, Value) for smooth color transitions
 * FastLED HSV Range: H(0-255), S(0-255), V(0-255)
 * 
 * Current Settings:
 *   HUE = 42       → ~60° (Yellow-Green in web terms)
 *   SAT = 45       → ~17.65% saturation (subtle, warm color)
 *   
 * Brightness States:
 *   STANDBY  = 0   → LEDs off (0% brightness)
 *   ACTIVE   = 255 → LEDs full brightness (100%)
 * 
 * This creates a warm, gentle yellow-green glow that's easy on the eyes
 * Perfect for ambient lighting in interactive installations
 */
#define HUE 42                    // Hue: Yellow-green tint (0-255 range)
#define SAT 45                    // Saturation: Subtle color intensity
#define STBYBRIGHTNESS 0          // Standby: LEDs off
#define ACTIVEBRIGHTNESS 255      // Active: Full brightness

// ═════════════════════════ TIMING CONFIGURATION ═════════════════════════
// All timing values in milliseconds (ms)
// These control the speed of LED breathing animations and state transitions

#define STBYTOACTIVETIME 2000      // ⏱️ Standby → Active: 2 seconds fade-in (breathing up)
#define ACTIVETOSTBYTIME 2000      // ⏱️ Active → Standby: 2 seconds fade-out (breathing down)
#define ACTIVETOTRIGTIME 1000      // ⏱️ Active → Trigger: 1 second dramatic fade
#define TRIGTOACTIVETIME 1000      // ⏱️ Trigger → Active: 1 second recovery fade
#define TRIGGERHOLDINGTIME 15000   // ⏱️ Trigger hold duration: 15 seconds (video playback)
#define TRIGGERIGNORETIME 30000    // ⏱️ Trigger cooldown: 30 seconds (prevent spam)

// ═════════════════════════ GPIO PIN DEFINITIONS ═════════════════════════

/* ──────────────── Power Control ──────────────── */
const uint8_t POWERBUTTON = 24;     // 19mm illuminated power button (active LOW)

/* ──────────────── Light Sensors ──────────────── */
// 22 Interactive trigger sensors detecting visitor presence
// Each sensor corresponds to a specific LED strip and video content
#define NUM_TRIGGERS 22
const uint8_t Toggles[] = { 25, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23 };
/* ──────────────── NeoPixel LED Strip Outputs ──────────────── */
// 8 data pins controlling 22 LED strips via parallel output
// Each pin can drive multiple strips in series
const uint8_t Strip1 = 2;           // Data pin for strip group 1
const uint8_t Strip2 = 3;           // Data pin for strip group 2
const uint8_t Strip3 = 4;           // Data pin for strip group 3
const uint8_t Strip4 = 6;           // Data pin for strip group 4
const uint8_t Strip5 = 7;           // Data pin for strip group 5
const uint8_t Strip6 = 8;           // Data pin for strip group 6
const uint8_t Strip7 = 9;           // Data pin for strip group 7
const uint8_t Strip8 = 10;          // Data pin for strip group 8

// ═════════════════════════ LED STRIP MAPPING ═════════════════════════
// Physical strip positions mapped to LED buffer indices
// Strips organized into two groups (A & B) for spatial arrangement

/* ──────────────── Group A: Strips 1-11 ──────────────── */
const uint16_t STRIP_A1 = 33;       // Starting index for strip A1
const uint16_t STRIP_A2 = 17;       // Starting index for strip A2
const uint16_t STRIP_A3 = 82;       // Starting index for strip A3
const uint16_t STRIP_A4 = 66;       // Starting index for strip A4
const uint16_t STRIP_A5 = 131;      // Starting index for strip A5
const uint16_t STRIP_A6 = 1;        // Starting index for strip A6
const uint16_t STRIP_A7 = 50;       // Starting index for strip A7
const uint16_t STRIP_A8 = 115;      // Starting index for strip A8
const uint16_t STRIP_A9 = 99;       // Starting index for strip A9
const uint16_t STRIP_A10 = 164;     // Starting index for strip A10
const uint16_t STRIP_A11 = 148;     // Starting index for strip A11

/* ──────────────── Group B: Strips 1-11 ──────────────── */
const uint16_t STRIP_B1 = 181;      // Starting index for strip B1
const uint16_t STRIP_B2 = 197;      // Starting index for strip B2
const uint16_t STRIP_B3 = 214;      // Starting index for strip B3
const uint16_t STRIP_B4 = 312;      // Starting index for strip B4
const uint16_t STRIP_B5 = 263;      // Starting index for strip B5
const uint16_t STRIP_B6 = 230;      // Starting index for strip B6
const uint16_t STRIP_B7 = 328;      // Starting index for strip B7
const uint16_t STRIP_B8 = 344;      // Starting index for strip B8
const uint16_t STRIP_B9 = 279;      // Starting index for strip B9
const uint16_t STRIP_B10 = 295;     // Starting index for strip B10
const uint16_t STRIP_B11 = 246;     // Starting index for strip B11

/* ──────────────── Driver LED Positions ──────────────── */
// Special LEDs that are always OFF (used as spacers/drivers in the chain)
const uint16_t DriverLED[8] = { 0, 49, 98, 147, 180, 213, 262, 311 };

/* ──────────────── Strip Index Constants ──────────────── */
const uint16_t STRIP_NONE = 0x00;   // Placeholder for inactive triggers

// Lookup table: Quick access to all strip starting positions
const uint16_t STRIP_PTR[23] = {
	STRIP_A1 , STRIP_A2, STRIP_A3, STRIP_A4, STRIP_A5, STRIP_A6, STRIP_A7, STRIP_A8, STRIP_A9, STRIP_A10, STRIP_A11,
	STRIP_B1 , STRIP_B2, STRIP_B3, STRIP_B4, STRIP_B5, STRIP_B6, STRIP_B7, STRIP_B8, STRIP_B9, STRIP_B10, STRIP_B11,
	STRIP_NONE  // Sentinel value for end of array
};


// ═════════════════════════ ANIMATION SETTINGS ═════════════════════════
const uint16_t nLEDInterval = 25;   // LED update interval: 25ms = 40 FPS (smooth animation)

// ═════════════════════════ TRIGGER MAPPING TABLE ═════════════════════════
/*
 * PreDetermine Array: Maps each sensor trigger to 3 LED strips
 * 
 * When a sensor (index 0-21) is triggered:
 *   - The 3 specified strips stay BRIGHT
 *   - All other strips fade to STANDBY
 *   - Creates a dramatic spotlight effect
 * 
 * Format: { Strip1, Strip2, Strip3 }
 * STRIP_NONE = No strip assigned (trigger ignored)
 * 
 * Example: Trigger[0] → Lights up STRIP_A6, STRIP_A9, STRIP_A5
 */
const uint16_t PreDetermine[NUM_TRIGGERS][3] = {
	// Triggers 0-3
	{ STRIP_A6, STRIP_A9, STRIP_A5 },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },{ STRIP_A5, STRIP_A6, STRIP_A9 },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },
	// Triggers 4-7
	{ STRIP_A1, STRIP_A3, STRIP_A6 },{ STRIP_A1, STRIP_A5, STRIP_A10 },{ STRIP_A2, STRIP_A4, STRIP_A8 },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },
	// Triggers 8-10
	{ STRIP_A6, STRIP_A10, STRIP_A1 },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },
	// Triggers 11-14
	{ STRIP_B4, STRIP_B6, STRIP_B9 },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },{ STRIP_B9, STRIP_B6, STRIP_B1 },
	// Triggers 15-18
	{ STRIP_NONE, STRIP_NONE, STRIP_NONE },{ STRIP_B4, STRIP_B9, STRIP_B10 },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },{ STRIP_NONE, STRIP_NONE, STRIP_NONE },
	// Triggers 19-21
	{ STRIP_B4, STRIP_B6, STRIP_B10 },{ STRIP_B9, STRIP_B1, STRIP_B6 },{ STRIP_NONE, STRIP_NONE, STRIP_NONE }
};

// ═════════════════════════ FUNCTION BLOCK INSTANCES ═════════════════════════
// PLC-style timer and trigger objects for robust debouncing

/* ──────────────── Trigger Sensors (22x) ──────────────── */
static TON TriggerTON[NUM_TRIGGERS];    // Timer-On-Delay: Debounce sensor inputs
static TP TriggerTP[NUM_TRIGGERS];      // Timer-Pulse: Cooldown period (future use)
static Rtrg TriggerRtrg[NUM_TRIGGERS];  // Rising-edge Trigger: Detect activation

/* ──────────────── Power Button ──────────────── */
static TON PowerTON;                    // Timer-On-Delay: Debounce power button
static Rtrg PowerTrg;                   // Rising-edge Trigger: Detect button press

/* ──────────────── Global State Variables ──────────────── */
static uint64_t nTimeMs = 0;            // Last LED update timestamp (for timing)
static uint8_t nTrgNumber = 0;          // Currently triggered sensor index (0-21)

// ═════════════════════════ STATE FUNCTION DECLARATIONS ═════════════════════════
// Forward declarations for Finite State Machine callbacks

/* ──────────────── Transition States ──────────────── */
void Standby2ActiveUpdate();            // 💡 Breathing UP: Standby → Active
void Active2StandbyUpdate();            // 💡 Breathing DOWN: Active → Standby
void Active2TriggerUpdate();            // 🎯 Spotlight fade: Active → Triggered
void Trigger2ActiveUpdate();            // 🎯 Recovery fade: Triggered → Active

/* ──────────────── Steady States ──────────────── */
void StandbyUpdate();                   // 😴 Standby: All LEDs off, waiting
void ActiveEnter();                     // ✨ Active Entry: Turn on all LEDs
void ActiveUpdate();                    // ✨ Active: All LEDs bright, ready
void TrigerEnter();                     // 🎬 Trigger Entry: Start video + spotlight
void TrigerUpdate();                    // 🎬 Trigger: Hold spotlight during video
void TrigerEnd();                       // 🎬 Trigger Exit: Stop video

// ═════════════════════════ FINITE STATE MACHINE SETUP ═════════════════════════
/*
 * State Machine Flow:
 * 
 *   ┌─────────┐  Power Button  ┌────────────────┐  2s fade  ┌────────┐
 *   │ STANDBY │ ──────────────→ │ Standby2Active │ ────────→ │ ACTIVE │
 *   └─────────┘                 └────────────────┘           └────────┘
 *        ↑                                                        │ │
 *        │                      ┌────────────────┐  2s fade      │ │
 *        └──────────────────────│ Active2Standby │ ←─────────────┘ │
 *                               └────────────────┘   (timeout)      │
 *                                                                   │
 *                               ┌────────────────┐  1s fade         │ Sensor
 *                     ┌─────────│ Trigger2Active │ ←────────┐      │ Trigger
 *                     │         └────────────────┘          │      │
 *                     ↓                                      │      ↓
 *                 ┌────────┐                          ┌──────────────┐
 *                 │ ACTIVE │                          │Active2Trigger│
 *                 └────────┘                          └──────────────┘
 *                                                            │ 1s fade
 *                                                            ↓
 *                                                      ┌───────────┐
 *                                                      │ TRIGGERED │
 *                                                      └───────────┘
 *                                                       (15s video)
 */

/* ──────────────── State Definitions ──────────────── */
// Each state can have: Enter(), Update(), Exit() callbacks
State Standby2Active = State(Standby2ActiveUpdate);           // Transition: Breathing up
State Active2Standby = State(Active2StandbyUpdate);           // Transition: Breathing down
State Active = State(ActiveEnter, ActiveUpdate, NULL);        // Steady: Bright & ready
State Active2Trigger = State(Active2TriggerUpdate);           // Transition: Spotlight fade
State Triggered = State(TrigerEnter, TrigerUpdate, TrigerEnd); // Steady: Video playing
State Trigger2Active = State(Trigger2ActiveUpdate);           // Transition: Recovery fade
State Standby = State(StandbyUpdate);                         // Steady: Sleeping (LEDs off)

/* ──────────────── FSM Instance ──────────────── */
FSM stateMachine = FSM(Standby);  // 🎬 Start in Standby state

/* ──────────────── Input Debounce Settings ──────────────── */
const uint16_t DEBOUNCE = 200;    // 200ms debounce for button/sensors (prevent noise)

// ═════════════════════════════════════════════════════════════════════════════
// ⚙️  INITIALIZATION FUNCTIONS
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Initialize all timer and trigger variables to default state
 * 
 * Resets all PLC-style function blocks (TON, TP, Rtrg) to ensure
 * clean startup without phantom triggers or timing glitches.
 * 
 * Also sets all LEDs to WHITE as a visual boot indicator.
 */
void InitVars()
{
	// Visual boot indicator: Flash all LEDs white
	fill_solid(leds, NUM_LEDS, CRGB::White);

	// Initialize all 22 trigger sensor timers
	for (uint8_t idx = 0; idx < NUM_TRIGGERS; idx++)
	{
		// TON (Timer-On-Delay): Debounce sensor input
		TriggerTON[idx].ET = 0;              // Elapsed time = 0
		TriggerTON[idx].IN = 0;              // Input = OFF
		TriggerTON[idx].PRE = 0;             // Previous state = OFF
		TriggerTON[idx].Q = 0;               // Output = OFF
		TriggerTON[idx].PT = DEBOUNCE;       // Preset time = 200ms

		// TP (Timer-Pulse): Cooldown period (future feature)
		TriggerTP[idx].PT = TRIGGERIGNORETIME; // Preset time = 30s
		TriggerTP[idx].IN = 0;
		TriggerTP[idx].PRE = 0;
		TriggerTP[idx].Q = 0;
		TriggerTP[idx].ET = 0;

		// Rtrg (Rising Trigger): Detect button press moment
		TriggerRtrg[idx].Q = 0;              // Output = OFF
		TriggerRtrg[idx].IN = 0;             // Input = OFF
		TriggerRtrg[idx].PRE = 0;            // Previous state = OFF
	}

	// Initialize power button trigger
	PowerTrg.IN = 0;
	PowerTrg.PRE = 0;
	PowerTrg.Q = 0;

	// Initialize power button timer
	PowerTON.ET = 0;
	PowerTON.PT = DEBOUNCE;              // 200ms debounce
	PowerTON.IN = 0;
	PowerTON.PRE = 0;
	PowerTON.Q = 0;
}

// ═════════════════════════════════════════════════════════════════════════════
// 🚀 ARDUINO SETUP FUNCTION
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief One-time initialization (runs once on power-up)
 * 
 * Setup sequence:
 * 1. Initialize serial communications (debug + video player)
 * 2. Configure GPIO pins (power button, sensors, LED outputs)
 * 3. Initialize FastLED library with strip assignments
 * 4. Reset all timer/trigger variables to default state
 */
void setup()
{
	// 📡 Console Initialization (USB debugging)
	Serial.begin(115200);
	while (!Serial);  // Wait for USB serial port (Teensy specific)
	Serial.println("Program started!!!");
	
	// 🎬 Video Player Serial Interface (hardware UART)
	PlayerSerial.begin(115200);

	// 🔘 Power Button Configuration (19mm illuminated push button)
	digitalWrite(POWERBUTTON, HIGH);        // Enable internal pull-up resistor
	pinMode(POWERBUTTON, INPUT_PULLUP);    // Active LOW when pressed

	// 💡 Light Sensor Configuration (22 interactive triggers)
	for (uint8_t idx = 0; idx < NUM_TRIGGERS; idx++)
	{
		digitalWrite(Toggles[idx], HIGH);   // Enable internal pull-up resistor
		pinMode(Toggles[idx], INPUT_PULLUP); // Active LOW when triggered
	}

	// 🎨 LED Strip Pin Configuration (8 outputs for 22 strips)
	pinMode(Strip1, OUTPUT);
	pinMode(Strip2, OUTPUT);
	pinMode(Strip3, OUTPUT);
	pinMode(Strip4, OUTPUT);
	pinMode(Strip5, OUTPUT);
	pinMode(Strip6, OUTPUT);
	pinMode(Strip7, OUTPUT);
	pinMode(Strip8, OUTPUT);

	// ✨ FastLED Configuration: Assign LED strips to output pins
	// Each pin drives multiple strips in series (daisy-chained)
	// Format: addLeds<TYPE, PIN>(buffer_start, led_count)
	
	FastLED.addLeds<NEOPIXEL, Strip1>(leds + (NUM_LEDS_PER_STRIP * 0 + 0), NUM_LEDS_PER_STRIP * 3 + 1);
	FastLED.addLeds<NEOPIXEL, Strip2>(leds + (NUM_LEDS_PER_STRIP * 3 + 1), NUM_LEDS_PER_STRIP * 3 + 1);
	FastLED.addLeds<NEOPIXEL, Strip3>(leds + (NUM_LEDS_PER_STRIP * 6 + 2), NUM_LEDS_PER_STRIP * 3 + 1);
	FastLED.addLeds<NEOPIXEL, Strip4>(leds + (NUM_LEDS_PER_STRIP * 9 + 3), NUM_LEDS_PER_STRIP * 2 + 1);
	FastLED.addLeds<NEOPIXEL, Strip5>(leds + (NUM_LEDS_PER_STRIP * 11 + 4), NUM_LEDS_PER_STRIP * 2 + 1);
	FastLED.addLeds<NEOPIXEL, Strip6>(leds + (NUM_LEDS_PER_STRIP * 13 + 5), NUM_LEDS_PER_STRIP * 3 + 1);
	FastLED.addLeds<NEOPIXEL, Strip7>(leds + (NUM_LEDS_PER_STRIP * 16 + 6), NUM_LEDS_PER_STRIP * 3 + 1);
	FastLED.addLeds<NEOPIXEL, Strip8>(leds + (NUM_LEDS_PER_STRIP * 19 + 7), NUM_LEDS_PER_STRIP * 3 + 1);

	// 🔄 Initialize All Variables
	InitVars();
}


// ═════════════════════════════════════════════════════════════════════════════
// ♻️ ARDUINO MAIN LOOP
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Main program loop (executes continuously)
 * 
 * Loop structure:
 * 1. Process all 22 light sensor inputs (debouncing + edge detection)
 * 2. Process power button input
 * 3. Evaluate state machine timeout conditions
 * 4. Execute state transitions based on conditions
 * 5. Update current state (LED animations, etc.)
 * 
 * This creates a responsive, real-time interactive installation!
 */
void loop()
{
	// ┌────────────────────────────────────────────────────────┐
	// │ 💡 SENSOR INPUT PROCESSING (22 triggers)           │
	// └────────────────────────────────────────────────────────┘
	// Process each trigger sensor with PLC-style function blocks
	for (uint8_t idx = 0; idx < NUM_TRIGGERS; idx++)
	{
		// Step 1: Read raw sensor input (active LOW)
		TriggerTON[idx].IN = digitalRead(Toggles[idx]) == LOW;
		
		// Step 2: TON - Debounce the input (requires 200ms stable signal)
		TONFunc(&TriggerTON[idx]);

		// Step 3: Rtrg - Detect rising edge (moment of activation)
		TriggerRtrg[idx].IN = TriggerTON[idx].Q;
		RTrgFunc(&TriggerRtrg[idx]);
		
		// Step 4: If sensor just triggered AND we're in Active state...
		if (TriggerRtrg[idx].Q && stateMachine.isInState(Active))
		{
			Serial.print("Light sensor ");
			Serial.print(idx + 1);
			Serial.println(" is Triggered!");

			// Only process specific sensors (those with video content)
			switch (idx)
			{
			case 0:   // Sensors with active content
			case 2:
			case 4:
			case 5:
			case 6:
			case 8:
			case 11:
			case 14:
			case 16:
			case 19:
			case 20:
			{
				// Store triggered sensor index and transition to spotlight mode
				nTrgNumber = idx;
				stateMachine.transitionTo(Active2Trigger);
			}
			break;
			}
		}
	}

	// ┌────────────────────────────────────────────────────────┐
	// │ 🔘 POWER BUTTON PROCESSING                          │
	// └────────────────────────────────────────────────────────┘
	// Only works when in Standby state (prevents accidental shutdowns)
	PowerTON.IN = digitalRead(POWERBUTTON) == LOW && stateMachine.isInState(Standby);
	TONFunc(&PowerTON);              // Debounce button press
	PowerTrg.IN = PowerTON.Q;
	RTrgFunc(&PowerTrg);             // Detect press moment
	
	if (PowerTrg.Q)
	{
		Serial.println("Power Button Pressed, I have entered into Active Status.");
		stateMachine.transitionTo(Standby2Active);  // 💤 → ✨ Wake up!
	}

	// ┌────────────────────────────────────────────────────────┐
	// │ ⏰ STATE TIMEOUT CONDITIONS                          │
	// └────────────────────────────────────────────────────────┘
	
	// Active state timeout: After 60 seconds of no activity, fade out
	uint32_t ActiveTimeOut = 60000; // 1 minute
	if (stateMachine.isInState(Active) && stateMachine.timeInCurrentState() >= ActiveTimeOut)
	{
		stateMachine.transitionTo(Active2Standby);
		Serial.println("I have switched into Active2Standby Status");
	}

	// Standby2Active transition complete: Breathing up animation finished
	if (stateMachine.isInState(Standby2Active) && stateMachine.timeInCurrentState() >= STBYTOACTIVETIME)
	{
		stateMachine.transitionTo(Active);
		Serial.println("I have switched into Active Status");
	}

	// Active2Standby transition complete: Breathing down animation finished
	if (stateMachine.isInState(Active2Standby) && stateMachine.timeInCurrentState() >= ACTIVETOSTBYTIME)
	{
		Serial.println("I have switched into Standby Status");
		stateMachine.transitionTo(Standby);
	}

	// Active2Trigger transition complete: Spotlight fade finished
	if (stateMachine.isInState(Active2Trigger) && stateMachine.timeInCurrentState() >= ACTIVETOTRIGTIME)
	{
		stateMachine.transitionTo(Triggered);
		Serial.println("I have switched into Trigger Status");
	}

	// Triggered state timeout: Video playback complete (or sensor cleared)
	if (stateMachine.isInState(Triggered) && stateMachine.timeInCurrentState() >= TRIGGERHOLDINGTIME)
	{
		// Check if sensor is no longer blocked (visitor moved away)
		if (digitalRead(Toggles[nTrgNumber]) == HIGH)
		{
			Serial.println("I have switched into Trigger2Active Status");
			stateMachine.transitionTo(Trigger2Active);
		}
		// Safety timeout: Force transition after 3x normal duration
		else if (stateMachine.timeInCurrentState() > TRIGGERHOLDINGTIME * 3)
		{
			Serial.println("Three cycles ignored, I have switched into Trigger2Active Status");
			stateMachine.transitionTo(Trigger2Active);
		}
	}

	// Trigger2Active transition complete: Recovery fade finished
	if (stateMachine.isInState(Trigger2Active) && stateMachine.timeInCurrentState() >= TRIGTOACTIVETIME)
	{
		Serial.println("I have switched into Active Status");
		stateMachine.transitionTo(Active);
	}

	// ┌────────────────────────────────────────────────────────┐
	// │ 🔄 EXECUTE CURRENT STATE                            │
	// └────────────────────────────────────────────────────────┘
	// Run the update() function for the current state
	// This handles LED animations, timing, etc.
	stateMachine.update();
}

// ═════════════════════════════════════════════════════════════════════════════
// 🎨 STATE UPDATE FUNCTIONS
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief STANDBY State Update
 * @state 😴 Sleeping - All LEDs off
 * 
 * System is dormant, waiting for power button press.
 * LEDs are set to STBYBRIGHTNESS (0) = completely off.
 * Updates at 40 FPS to maintain responsiveness.
 */
void StandbyUpdate()
{
	// Throttle updates to 40 FPS (every 25ms)
	if ((nTimeMs + nLEDInterval) < millis())
	{
		nTimeMs = millis();
		
		// Turn off all LEDs (HSV brightness = 0)
		fill_solid(leds, NUM_LEDS, CHSV(HUE, SAT, STBYBRIGHTNESS));
		
		// Ensure driver LEDs stay off (visual spacers)
		for (uint8_t idx = 0; idx < 8; idx++)
			leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);
			
		FastLED.show();  // Push changes to physical LEDs
	}
}

/**
 * @brief ACTIVE State Entry (runs once when entering state)
 * @state ✨ Active - All LEDs bright and ready
 * 
 * Instantly turns on all LEDs to full brightness.
 * This is called when transitioning from Standby2Active.
 */
void ActiveEnter()
{
	// Instantly set all LEDs to full brightness
	fill_solid(leds, NUM_LEDS, CHSV(HUE, SAT, ACTIVEBRIGHTNESS));
	
	// Keep driver LEDs off
	for (uint8_t idx = 0; idx < 8; idx++)
		leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);
		
	FastLED.show();  // Display immediately
}

/**
 * @brief ACTIVE State Update (runs continuously while in Active state)
 * @state ✨ Active - Ready for sensor triggers
 * 
 * Maintains all LEDs at full brightness with a warm white color.
 * System is listening for sensor triggers in this state.
 * Uses RGB(255, 255, 209) = warm white for pleasant ambient lighting.
 */
void ActiveUpdate()
{
	// Throttle updates to 40 FPS
	if ((nTimeMs + nLEDInterval) < millis())
	{
		nTimeMs = millis();
		
		// Warm white color (slightly yellow tint)
		fill_solid(leds, NUM_LEDS, CRGB(255, 255, 209));
		
		// Alternative: Use HSV color
		// fill_solid(leds, NUM_LEDS, CHSV(HUE, SAT, ACTIVEBRIGHTNESS));
		
		// Keep driver LEDs off
		for (uint8_t idx = 0; idx < 8; idx++)
			leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);
			
		FastLED.show();
	}
}

/**
 * @brief TRIGGERED State Entry (runs once when sensor activates)
 * @state 🎯 Triggered - Spotlight effect + video playback
 * 
 * Creates dramatic spotlight effect:
 * - 3 predetermined strips stay BRIGHT (spotlight)
 * - All other strips fade to STANDBY (dark background)
 * - Sends video command to external player
 * 
 * This creates visual focus on the triggered area!
 */
void TrigerEnter()
{
	// 🎬 Start video playback for this trigger
	SendVideoCommand(nTrgNumber + 1);
	
	// Apply spotlight effect: Bright vs. Dark strips
	for (uint8_t idx = 0; idx < NUM_STRIPS; idx++)
	{
		uint16_t ntemp = STRIP_PTR[idx];  // Get strip starting position
		
		// Check if this strip is in the spotlight (one of the 3 predetermined strips)
		if ((PreDetermine[nTrgNumber][0] == ntemp || PreDetermine[nTrgNumber][1] == ntemp) || PreDetermine[nTrgNumber][2] == ntemp)
		{
			// ✨ SPOTLIGHT: Keep this strip bright
			fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, ACTIVEBRIGHTNESS));
		}
		else
		{
			// 🌑 BACKGROUND: Fade this strip to dark
			fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, STBYBRIGHTNESS));
		}
	}
	
	// Keep driver LEDs off
	for (uint8_t idx = 0; idx < 8; idx++)
		leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);

	FastLED.show();  // Display spotlight effect immediately
}

/**
 * @brief TRIGGERED State Update
 * @state 🎯 Holding spotlight during video
 * 
 * Maintains spotlight effect while video plays (15 seconds).
 * No animation needed - just hold the pattern.
 */
void TrigerUpdate()
{
	// Static state - spotlight pattern set in TrigerEnter()
	// Video player handles playback timing
}

// ═════════════════════════════════════════════════════════════════════════════
// 🎬 VIDEO PLAYER CONTROL
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Send video playback command to external player via serial
 * @param nVideoIndex Video number to play (0 = stop, 1-22 = content, 23 = pause)
 * 
 * Protocol: Custom 6-byte packet format
 * 
 * Packet Structure:
 * [0] 0xFF       - Start marker
 * [1] 0xAA       - Packet header
 * [2] 0x01       - Data length (1 byte)
 * [3] Video ID   - Which video to play (1-23)
 * [4] Checksum   - (Data_Length + Video_ID) % 100
 * [5] 0xFE       - End marker
 * 
 * Example: Play video 5
 * 0xFF 0xAA 0x01 0x05 0x06 0xFE
 *      ^    ^    ^    ^    ^    ^
 *     STX  HDR  LEN  VID  CHK  ETX
 */
void SendVideoCommand(uint8_t nVideoIndex)
{
	uint8_t nPacket[6];
	
	// 📦 Build packet bytes
	nPacket[0] = 0xFF;               // Start character (STX)
	nPacket[1] = 0xAA;               // Packet start byte (header)
	nPacket[2] = 0x01;               // Data length (1 byte follows)
	nPacket[3] = nVideoIndex + 1;    // Video index (1-based)

	// 🔐 Calculate checksum (simple modulo-100)
	uint8_t nSum = 0;
	nSum += nPacket[2];              // Add data length
	nSum += nPacket[3];              // Add video index
	nPacket[4] = nSum % 100;         // Checksum byte
	
	nPacket[5] = 0xFE;               // End character (ETX)

	// 📤 Transmit packet to video player
	for (uint8_t idx = 0; idx < 6; idx++)
	{
		PlayerSerial.write(nPacket[idx]);
	}

	// 📝 Debug: Print command to console
	Serial.println("I sent video command!, Command is ");
	Serial.print("0x");
	for (uint8_t idx = 0; idx < 6; idx++)
	{
		if (nPacket[idx] < 16)
			Serial.print("0");  // Add leading zero for single hex digits
		Serial.print(nPacket[idx], HEX);
	}
	Serial.println();
}

// ═════════════════════════════════════════════════════════════════════════════
// 🎬 STATE TRANSITION ANIMATIONS
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief STANDBY → ACTIVE Transition (Breathing UP animation)
 * @duration 2 seconds (STBYTOACTIVETIME)
 * 
 * Beautiful fade-in effect:
 * - Smoothly interpolates brightness from 0 → 255
 * - Creates "breathing" sensation (inhale)
 * - Uses Arduino map() for linear interpolation
 */
void Standby2ActiveUpdate()
{
	// Calculate current brightness based on elapsed time
	// map(value, fromLow, fromHigh, toLow, toHigh)
	uint8_t nBrightness = map(stateMachine.timeInCurrentState(), 0, STBYTOACTIVETIME, STBYBRIGHTNESS, ACTIVEBRIGHTNESS);
	
	// Throttle updates to 40 FPS
	if ((nTimeMs + nLEDInterval) < millis())
	{
		nTimeMs = millis();
		
		// Apply brightness to all strips
		for (uint8_t idx = 0; idx < NUM_STRIPS; idx++)
		{
			uint16_t ntemp = STRIP_PTR[idx];
			fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, nBrightness));
		}

		// Keep driver LEDs off
		for (uint8_t idx = 0; idx < 8; idx++)
			leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);
			
		FastLED.show();
	}
}

/**
 * @brief ACTIVE → STANDBY Transition (Breathing DOWN animation)
 * @duration 2 seconds (ACTIVETOSTBYTIME)
 * 
 * Beautiful fade-out effect:
 * - Smoothly interpolates brightness from 255 → 0
 * - Creates "breathing" sensation (exhale)
 * - Prepares system for sleep mode
 */
void Active2StandbyUpdate()
{
	// Calculate fading brightness (reverse direction)
	uint8_t nBrightness = map(stateMachine.timeInCurrentState(), 0, ACTIVETOSTBYTIME, ACTIVEBRIGHTNESS, STBYBRIGHTNESS);
	
	if ((nTimeMs + nLEDInterval) < millis())
	{
		nTimeMs = millis();
		
		for (uint8_t idx = 0; idx < NUM_STRIPS; idx++)
		{
			uint16_t ntemp = STRIP_PTR[idx];
			fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, nBrightness));
		}
		
		for (uint8_t idx = 0; idx < 8; idx++)
			leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);
			
		FastLED.show();
	}
}

/**
 * @brief ACTIVE → TRIGGER Transition (Spotlight fade-in)
 * @duration 1 second (ACTIVETOTRIGTIME)
 * 
 * Dramatic focusing effect:
 * - 3 predetermined strips: Stay BRIGHT (spotlight)
 * - All other strips: Fade from ACTIVE → STANDBY (darken background)
 * - Creates visual focus on triggered area
 * - Builds anticipation for video content
 */
void Active2TriggerUpdate()
{
	// Calculate fading brightness for background strips
	uint8_t nBrightFadeDown = map(stateMachine.timeInCurrentState(), 0, ACTIVETOTRIGTIME, ACTIVEBRIGHTNESS, STBYBRIGHTNESS);
	
	if ((nTimeMs + nLEDInterval) < millis())
	{
		nTimeMs = millis();
		Serial.println("Active to Trigger");
		
		for (uint8_t idx = 0; idx < NUM_STRIPS; idx++)
		{
			uint16_t ntemp = STRIP_PTR[idx];
			
			// Check if this strip is in the spotlight
			if ((PreDetermine[nTrgNumber][0] == ntemp || PreDetermine[nTrgNumber][1] == ntemp) || PreDetermine[nTrgNumber][2] == ntemp)
			{
				// ✨ SPOTLIGHT: Keep bright
				fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, ACTIVEBRIGHTNESS));
			}
			else
			{
				// 🌑 BACKGROUND: Fade to dark
				fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, nBrightFadeDown));
			}
		}
		
		for (uint8_t idx = 0; idx < 8; idx++)
			leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);

		FastLED.show();
	}
}

/**
 * @brief TRIGGER → ACTIVE Transition (Spotlight fade-out)
 * @duration 1 second (TRIGTOACTIVETIME)
 * 
 * Recovery effect:
 * - 3 spotlight strips: Stay BRIGHT (already on)
 * - Background strips: Fade from STANDBY → ACTIVE (brighten up)
 * - Returns to "ready" state with all LEDs bright
 * - Smooth transition back to normal operation
 */
void Trigger2ActiveUpdate()
{
	// Calculate brightening for background strips
	uint8_t nBrightFadeUp = map(stateMachine.timeInCurrentState(), 0, TRIGTOACTIVETIME, STBYBRIGHTNESS, ACTIVEBRIGHTNESS);
	
	if ((nTimeMs + nLEDInterval) < millis())
	{
		nTimeMs = millis();
		Serial.println("Trigger to activate");
		
		for (uint8_t idx = 0; idx < NUM_STRIPS; idx++)
		{
			uint16_t ntemp = STRIP_PTR[idx];
			
			// Check if this strip was in the spotlight
			if ((PreDetermine[nTrgNumber][0] == ntemp || PreDetermine[nTrgNumber][1] == ntemp) || PreDetermine[nTrgNumber][2] == ntemp)
			{
				// ✨ SPOTLIGHT: Already bright, stay bright
				fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, ACTIVEBRIGHTNESS));
			}
			else
			{
				// ☀️ BACKGROUND: Fade back to bright
				fill_solid(leds + ntemp, NUM_LEDS_PER_STRIP, CHSV(HUE, SAT, nBrightFadeUp));
			}
		}
		
		for (uint8_t idx = 0; idx < 8; idx++)
			leds[DriverLED[idx]] = CRGB(0x00, 0x00, 0x00);
			
		FastLED.show();
	}
}

/**
 * @brief TRIGGERED State Exit (runs once when leaving state)
 * @state 🎯 Cleanup after trigger
 * 
 * Stops video playback and prepares for transition back to Active.
 * Called automatically by FSM when exiting Triggered state.
 */
void TrigerEnd()
{
	// Stop video playback (send command 0)
	SendVideoCommand(0);
}

/**
 * @brief Pause video playback (utility function)
 * 
 * Sends pause command (23) to video player.
 * Currently not used in main state machine.
 */
void pauseVideo()
{
	SendVideoCommand(23);  // Command 23 = Pause
}

/**
 * @brief Stop video playback (utility function)
 * 
 * Sends stop command (25) to video player.
 * Currently not used in main state machine.
 */
void stopVideo()
{
	SendVideoCommand(25);  // Command 25 = Stop
}

/*
 * ═══════════════════════════════════════════════════════════════════════════
 * 🎉 END OF FILE - Interactive LED Installation Controller
 * 
 * Project Features:
 * ✓ Beautiful breathing animations (2-second fades)
 * ✓ 22 interactive sensor triggers
 * ✓ Dramatic spotlight effects
 * ✓ Video player integration
 * ✓ Robust debouncing (PLC-style function blocks)
 * ✓ 60-second auto-sleep timeout
 * ✓ 40 FPS smooth LED updates
 * 
 * State Machine Flow:
 * STANDBY → Standby2Active → ACTIVE → Active2Trigger → TRIGGERED → Trigger2Active → ACTIVE
 *     ↑                                  │
 *     └────── Active2Standby ←────────┘
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */
