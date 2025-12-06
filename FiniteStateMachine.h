/*
||═════════════════════════════════════════════════════════════════════════
|| @file       FiniteStateMachine.h
|| @project    Arduino Finite State Machine Library
||
|| @description
||   Lightweight, elegant state machine implementation for Arduino.
||   Provides structured state management with enter/update/exit callbacks.
||
|| @features
||   ✓ Clean state-based architecture
||   ✓ Enter/Update/Exit callbacks per state
||   ✓ Smooth state transitions with timing
||   ✓ State query methods (isInState, getCurrentState)
||   ✓ Time tracking (how long in current state)
||   ✓ Method chaining support
||
|| @author     Alexander Brevig
|| @version    1.7
|| @date       2010-03-08
|| @license    MIT License
||═════════════════════════════════════════════════════════════════════════
*/

#ifndef FINITESTATEMACHINE_H
#define FINITESTATEMACHINE_H

#include <WProgram.h>

// ═════════════════════════ CONVENIENCE MACROS ═════════════════════════
// Use these when a state doesn't need enter/update/exit callbacks
#define NO_ENTER (0)          // No enter function
#define NO_UPDATE (0)         // No update function
#define NO_EXIT (0)           // No exit function

#define FSM FiniteStateMachine  // Shorthand alias

// ═════════════════════════ STATE CLASS ═════════════════════════

/**
 * @brief State - Represents a single state with optional callbacks
 * 
 * A state can have three lifecycle callbacks:
 *   1. enter()  - Called ONCE when entering this state
 *   2. update() - Called EVERY loop while in this state
 *   3. exit()   - Called ONCE when leaving this state
 * 
 * Example:
 *   // State with only update callback
 *   State idleState = State(idleUpdate);
 *   
 *   // State with all three callbacks
 *   State activeState = State(activeEnter, activeUpdate, activeExit);
 *   
 *   void activeEnter() { Serial.println("Entering active!"); }
 *   void activeUpdate() { // Do work... }
 *   void activeExit() { Serial.println("Leaving active!"); }
 */
class State {
	public:
		// Constructor: State with only update callback
		State( void (*updateFunction)() );
		
		// Constructor: State with enter, update, and exit callbacks
		State( void (*enterFunction)(), void (*updateFunction)(), void (*exitFunction)() );
		
		// Execute enter callback (called once on state entry)
		void enter();
		
		// Execute update callback (called every loop)
		void update();
		
		// Execute exit callback (called once on state exit)
		void exit();
		
	private:
		void (*userEnter)();     // Pointer to user's enter function
		void (*userUpdate)();    // Pointer to user's update function
		void (*userExit)();      // Pointer to user's exit function
};

// ═════════════════════════ FINITE STATE MACHINE CLASS ═════════════════════════

/**
 * @brief FiniteStateMachine - Manages state transitions and execution
 * 
 * The state machine:
 *   • Maintains current and next state pointers
 *   • Handles enter/exit callbacks automatically
 *   • Tracks time spent in current state
 *   • Supports method chaining for clean code
 * 
 * Typical Usage:
 * 
 *   // Define states
 *   State idle = State(idleUpdate);
 *   State active = State(activeEnter, activeUpdate, activeExit);
 *   
 *   // Create FSM starting in 'idle' state
 *   FSM machine = FSM(idle);
 *   
 *   void loop() {
 *     // Check conditions and transition
 *     if (buttonPressed) {
 *       machine.transitionTo(active);
 *     }
 *     
 *     // Execute current state's update()
 *     machine.update();
 *   }
 * 
 * Transition Flow:
 *   1. transitionTo(newState) called
 *   2. Current state's exit() runs (if defined)
 *   3. New state becomes current
 *   4. New state's enter() runs (if defined)
 *   5. New state's update() runs every loop
 */
class FiniteStateMachine {
	public:
		// Constructor: Initialize FSM with starting state
		FiniteStateMachine(State& current);
		
		// Execute current state's update() and handle transitions
		// @return Reference to self (for method chaining)
		FiniteStateMachine& update();
		
		// Queue a transition to a new state (happens before next update)
		// @param state The state to transition to
		// @return Reference to self (for method chaining)
		FiniteStateMachine& transitionTo( State& state );
		
		// Immediately transition to a new state (runs exit/enter now)
		// @param state The state to transition to
		// @return Reference to self (for method chaining)
		FiniteStateMachine& immediateTransitionTo( State& state );
		
		// Get reference to the current state object
		// @return Current State reference
		State& getCurrentState();
		
		// Check if FSM is currently in a specific state
		// @param state The state to check
		// @return true if current state matches, false otherwise
		boolean isInState( State &state ) const;
		
		// Get milliseconds elapsed since entering current state
		// @return Time in current state (ms)
		unsigned long timeInCurrentState();
		
	private:
		bool needToTriggerEnter;      // Flag: Should we call enter() on next update?
		State* currentState;          // Pointer to currently executing state
		State* nextState;             // Pointer to queued next state
		unsigned long stateChangeTime; // millis() timestamp of last state change
};

#endif

/*
|| @changelog
|| | 1.7 2010-03-08- Alexander Brevig : Fixed a bug, constructor ran update, thanks to Ren� Press�
|| | 1.6 2010-03-08- Alexander Brevig : Added timeInCurrentState() , requested by sendhb
|| | 1.5 2009-11-29- Alexander Brevig : Fixed a bug, introduced by the below fix, thanks to Jon Hylands again...
|| | 1.4 2009-11-29- Alexander Brevig : Fixed a bug, enter gets triggered on the first state. Big thanks to Jon Hylands who pointed this out.
|| | 1.3 2009-11-01 - Alexander Brevig : Added getCurrentState : &State
|| | 1.3 2009-11-01 - Alexander Brevig : Added isInState : boolean, requested by Henry Herman 
|| | 1.2 2009-05-18 - Alexander Brevig : enter and exit bug fix
|| | 1.1 2009-05-18 - Alexander Brevig : Added support for cascaded calls
|| | 1.0 2009-04-13 - Alexander Brevig : Initial Release
|| #
*/