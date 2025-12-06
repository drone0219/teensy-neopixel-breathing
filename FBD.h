/*
||═══════════════════════════════════════════════════════════════════════════
|| @file       FBD.h (Function Block Diagram)
|| @project    PLC-Style Timer and Trigger Library for Arduino
||
|| @description
||   This library implements IEC 61131-3 standard function blocks:
||   • TON  (Timer On-Delay)     - Delays turning ON
||   • TP   (Timer Pulse)        - Generates timed pulse
||   • Rtrg (Rising Trigger)     - Detects 0→1 transitions
||   • Ftrg (Falling Trigger)    - Detects 1→0 transitions
||
||   Perfect for debouncing buttons, sensors, and timing operations
||   in embedded systems with deterministic, predictable behavior.
||
|| @features
||   ✓ Industrial-grade timing functions
||   ✓ Edge detection (rising/falling)
||   ✓ Built-in debouncing capabilities
||   ✓ Memory-efficient struct-based design
||   ✓ Compatible with Arduino millis() timing
||
|| @author     Your Name
|| @date       December 7, 2025
|| @version    1.0
||═══════════════════════════════════════════════════════════════════════════
*/

#ifndef MyFunc_h
#define MyFunc_h

// ═══════════════════════════════════════════════════════════════════════
// ⏰ TIMER ON-DELAY (TON) FUNCTION BLOCK
// ═══════════════════════════════════════════════════════════════════════

/**
 * @brief TON - Timer On-Delay
 * @standard IEC 61131-3
 * 
 * Behavior:
 *   Output (Q) turns ON after input (IN) has been HIGH for PT milliseconds.
 *   Output turns OFF immediately when input goes LOW.
 * 
 * Timing Diagram:
 *   IN:  ──┐┌─────────────────────────────────┐┌──
 *         └┘                                └┘
 *   Q:   ────────┐┌─────────────────────────┐┌─
 *               └┘                          └┘
 *               ←─PT─→                       ←PT→
 * 
 * Use Cases:
 *   • Button debouncing (require 200ms stable press)
 *   • Sensor filtering (ignore quick glitches)
 *   • Delayed activation (safety interlocks)
 * 
 * @member IN   Input signal (0 or 1)
 * @member PRE  Previous input state (for edge detection)
 * @member Q    Output signal (0 or 1)
 * @member PT   Preset time in milliseconds (delay duration)
 * @member ET   Elapsed time tracking (internal)
 */
struct tonblock
{
    unsigned IN: 1;          // Input signal (0 = OFF, 1 = ON)
    unsigned PRE: 1;         // Previous input state (for change detection)
    unsigned Q: 1;           // Output signal (0 = OFF, 1 = ON after delay)
    unsigned long PT;        // Preset Time: How long IN must be HIGH (ms)
    unsigned long ET;        // Elapsed Time: Timestamp tracking (internal)
};
typedef struct tonblock TON;

// ═══════════════════════════════════════════════════════════════════════
// ⏱️ TIMER PULSE (TP) FUNCTION BLOCK
// ═══════════════════════════════════════════════════════════════════════

/**
 * @brief TP - Timer Pulse
 * @standard IEC 61131-3
 * 
 * Behavior:
 *   Output (Q) generates a fixed-duration pulse when input (IN) rises.
 *   Pulse lasts for PT milliseconds, regardless of input state.
 *   Retriggerable: New rising edge restarts the pulse.
 * 
 * Timing Diagram:
 *   IN:  ──┐┌────────────┐┌──────────────────────────┐┌─
 *         └┘              └┘                          └┘
 *   Q:   ────┐┌───────────────────────────────────┐┌───
 *           └┘                                  └┘
 *           ←───────PT (30s)──────────────→
 * 
 * Use Cases:
 *   • Cooldown timers (prevent spam/retriggering)
 *   • Watchdog timers (timeout detection)
 *   • Timed enable/disable signals
 *   • Event rate limiting
 * 
 * @member IN   Input signal (0 or 1)
 * @member PRE  Previous input state (for edge detection)
 * @member Q    Output pulse (0 or 1)
 * @member PT   Preset time: Pulse duration in milliseconds
 * @member ET   End time: When pulse expires (internal)
 */
struct tpblock
{
    unsigned IN: 1;          // Input signal (0 = OFF, 1 = ON)
    unsigned PRE: 1;         // Previous input state (for edge detection)
    unsigned Q: 1;           // Output pulse (0 = OFF, 1 = ON during pulse)
    unsigned long PT;        // Preset Time: Pulse duration (ms)
    unsigned long ET;        // End Time: When pulse should end (millis())
};
typedef struct tpblock TP;

// ═══════════════════════════════════════════════════════════════════════
// ⬆️ RISING EDGE TRIGGER (R_TRIG)
// ═══════════════════════════════════════════════════════════════════════

/**
 * @brief Rtrg - Rising Edge Trigger
 * @standard IEC 61131-3 (R_TRIG)
 * 
 * Behavior:
 *   Output (Q) pulses HIGH for ONE scan cycle when input (IN) transitions 0→1.
 *   Perfect for detecting button presses or event moments.
 * 
 * Timing Diagram:
 *   IN:  ────────┐┌────────────────────────────┐┌────
 *               └┘                              └┘
 *   Q:   ──────────┐┌──────────────────────────────┐┌─
 *                 └┘                                └┘
 *                 ↑                                  ↑
 *              (Moment of press)                (Another press)
 * 
 * Use Cases:
 *   • Detect button press moment (not hold)
 *   • Count events (increment on each trigger)
 *   • State machine transitions
 *   • Capture sensor activation instant
 * 
 * @member IN   Input signal (0 or 1)
 * @member PRE  Previous input state (for edge detection)
 * @member Q    Output pulse (1 for one cycle on rising edge)
 */
struct RisingTrg
{
    unsigned IN : 1;         // Input signal (current state)
    unsigned PRE : 1;        // Previous input state (last scan)
    unsigned Q : 1;          // Output pulse (1 on rising edge)
    unsigned : 5;            // Padding bits (struct alignment)
};
typedef struct RisingTrg Rtrg;

// ═══════════════════════════════════════════════════════════════════════
// ⬇️ FALLING EDGE TRIGGER (F_TRIG)
// ═══════════════════════════════════════════════════════════════════════

/**
 * @brief Ftrg - Falling Edge Trigger
 * @standard IEC 61131-3 (F_TRIG)
 * 
 * Behavior:
 *   Output (Q) pulses HIGH for ONE scan cycle when input (IN) transitions 1→0.
 *   Perfect for detecting button releases or signal drops.
 * 
 * Timing Diagram:
 *   IN:  ──────────────────────────────┐┌──────────────
 *                                  └┘
 *   Q:   ────────────────────────────────┐┌───────────
 *                                    └┘
 *                                    ↓
 *                                (Release)
 * 
 * Use Cases:
 *   • Detect button release
 *   • Sensor deactivation events
 *   • Signal loss detection
 *   • End-of-event triggers
 * 
 * @member IN   Input signal (0 or 1)
 * @member PRE  Previous input state (for edge detection)
 * @member Q    Output pulse (1 for one cycle on falling edge)
 */
struct FallingTrg
{
    unsigned IN : 1;         // Input signal (current state)
    unsigned PRE : 1;        // Previous input state (last scan)
    unsigned Q : 1;          // Output pulse (1 on falling edge)
    unsigned : 5;            // Padding bits (struct alignment)
};
typedef struct FallingTrg Ftrg;

// ═══════════════════════════════════════════════════════════════════════
// 🔧 FUNCTION IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════

/**
 * @brief Execute TON (Timer On-Delay) function block
 * @param pTP Pointer to TON structure
 * 
 * Algorithm:
 * 1. Detect rising edge (IN changes 0→1)
 *    → Start timer by capturing current millis()
 * 
 * 2. While IN is HIGH:
 *    → Check if (current_time >= start_time + PT)
 *    → If yes: Set Q = 1 (delay expired)
 * 
 * 3. When IN goes LOW:
 *    → Immediately set Q = 0 (instant off)
 *    → Reset timer
 * 
 * @note Must be called every scan cycle for proper operation
 * @usage
 *   TON myTimer;
 *   myTimer.PT = 200;  // 200ms delay
 *   
 *   loop() {
 *     myTimer.IN = digitalRead(BUTTON);
 *     TONFunc(&myTimer);
 *     if (myTimer.Q) {
 *       // Button has been pressed for 200ms!
 *     }
 *   }
 */
void TONFunc(TON *pTP)
{
    // ⬆️ Edge detection: Has input changed?
    if(pTP->IN != pTP->PRE)
    {
        pTP->PRE = pTP->IN;              // Update previous state
        if(pTP->IN == 1)                 // Rising edge detected?
            pTP->ET = millis();          // ⏰ Start timer (capture timestamp)
    }
    
    // ⏱️ Timer logic
    if(pTP->IN)                          // If input is HIGH
    {
        // Check if delay time has elapsed
        if((pTP->ET + pTP->PT) <= millis())
            pTP->Q = 1;                  // ✅ Delay expired → Turn ON output
    }
    else                                 // If input is LOW
    {
        pTP->ET = millis();              // Reset timer
        pTP->Q = 0;                      // ❌ Immediately turn OFF output
    }
}

/**
 * @brief Execute TP (Timer Pulse) function block
 * @param pTP Pointer to TP structure
 * 
 * Algorithm:
 * 1. Detect rising edge (IN changes 0→1)
 *    → Start pulse by setting ET = current_time + PT
 * 
 * 2. While ET > current_time:
 *    → Q = 1 (pulse active)
 * 
 * 3. When current_time >= ET:
 *    → Q = 0 (pulse expired)
 * 
 * @note Pulse runs for PT milliseconds regardless of input state
 * @note Retriggerable: New rising edge restarts pulse
 * @usage
 *   TP cooldown;
 *   cooldown.PT = 30000;  // 30-second cooldown
 *   
 *   if (trigger_event) {
 *     cooldown.IN = 1;
 *   }
 *   TPFunc(&cooldown);
 *   if (!cooldown.Q) {
 *     // Cooldown expired, ready for next trigger
 *   }
 */
void TPFunc(TP *pTP)
{
    // ⬆️ Edge detection: Has input changed?
    if(pTP->IN != pTP->PRE)
    {
        pTP->PRE = pTP->IN;              // Update previous state
        if(pTP->IN == 1)                 // Rising edge detected?
            pTP->ET = pTP->PT + millis(); // ⏰ Start pulse (set end time)
    }
    
    // ⏱️ Pulse logic
    if(pTP->ET > millis())               // Is pulse still active?
        pTP->Q = 1;                      // ✅ Pulse ON
    else
    {
        pTP->Q = 0;                      // ❌ Pulse expired → OFF
        pTP->ET = millis();              // Reset end time
    }
}

/**
 * @brief Execute Rising Edge Trigger (R_TRIG) function block
 * @param pTrg Pointer to Rtrg structure
 * 
 * Algorithm:
 * 1. Always reset Q = 0 first (single-scan pulse)
 * 2. If IN changed from 0 to 1:
 *    → Set Q = 1 for ONE scan cycle
 * 3. Update previous state
 * 
 * @note Q is only HIGH for ONE execution cycle
 * @note Must be called every scan cycle
 * @usage
 *   Rtrg buttonPress;
 *   
 *   loop() {
 *     buttonPress.IN = digitalRead(BUTTON);
 *     RTrgFunc(&buttonPress);
 *     if (buttonPress.Q) {
 *       // Button was just pressed THIS cycle!
 *       count++;
 *     }
 *   }
 */
void RTrgFunc(Rtrg *pTrg)
{
    pTrg->Q = 0;                         // ♻️ Reset output (single-scan pulse)
    
    // ⬆️ Edge detection
    if(pTrg->IN != pTrg->PRE)            // Has input changed?
    {
        pTrg->PRE = pTrg->IN;            // Update previous state
        if(pTrg->PRE == 1)               // Was it a 0→1 transition?
        {
            pTrg->Q = 1;                 // ✨ Trigger! (one cycle only)
        }    
    }
}

/**
 * @brief Execute Falling Edge Trigger (F_TRIG) function block
 * @param pTrg Pointer to Ftrg structure
 * 
 * Algorithm:
 * 1. Always reset Q = 0 first (single-scan pulse)
 * 2. If IN changed from 1 to 0:
 *    → Set Q = 1 for ONE scan cycle
 * 3. Update previous state
 * 
 * @note Q is only HIGH for ONE execution cycle
 * @note Must be called every scan cycle
 * @usage
 *   Ftrg buttonRelease;
 *   
 *   loop() {
 *     buttonRelease.IN = digitalRead(BUTTON);
 *     FTrgFunc(&buttonRelease);
 *     if (buttonRelease.Q) {
 *       // Button was just released THIS cycle!
 *     }
 *   }
 */
void FTrgFunc(Ftrg *pTrg)
{
    pTrg->Q = 0;                         // ♻️ Reset output (single-scan pulse)
    
    // ⬇️ Edge detection
    if(pTrg->IN != pTrg->PRE)            // Has input changed?
    {
        pTrg->PRE = pTrg->IN;            // Update previous state
        if(pTrg->IN == 0)                // Was it a 1→0 transition?
        {
            pTrg->Q = 1;                 // ✨ Trigger! (one cycle only)
        }    
    }
}

#endif //MyFunc_h

/*
 * ═════════════════════════════════════════════════════════════════════
 * 🎉 END OF FILE - PLC Function Block Library
 * 
 * Summary:
 * ✓ TON  - Timer On-Delay (debouncing, delayed activation)
 * ✓ TP   - Timer Pulse (cooldowns, watchdogs)
 * ✓ Rtrg - Rising Edge Trigger (press detection)
 * ✓ Ftrg - Falling Edge Trigger (release detection)
 * 
 * Standards Compliance:
 * ✓ IEC 61131-3 (International PLC Programming Standard)
 * ✓ Predictable, deterministic timing behavior
 * ✓ Industrial-grade reliability
 * 
 * Perfect for:
 * • Embedded systems
 * • Arduino/Teensy projects
 * • Industrial automation
 * • Robotics control
 * • Interactive installations
 * 
 * ═════════════════════════════════════════════════════════════════════
 */
