/*
||═════════════════════════════════════════════════════════════════════════
|| @file       FiniteStateMachine.cpp
|| @project    Arduino Finite State Machine Library - Implementation
||
|| @description
||   Implementation of State and FiniteStateMachine classes.
||   Provides the core logic for state management and transitions.
||
|| @author     Alexander Brevig
|| @version    1.7
||═════════════════════════════════════════════════════════════════════════
*/

#include "FiniteStateMachine.h"

// ═════════════════════════ STATE CLASS IMPLEMENTATION ═════════════════════════
/**
 * @brief State constructor - Update only
 * @param updateFunction Function pointer to call every loop
 * 
 * Creates a simple state with only an update callback.
 * No enter or exit callbacks.
 */
State::State(void(*updateFunction)()) {
	userEnter = 0;           // No enter function
	userUpdate = updateFunction;
	userExit = 0;            // No exit function
}

/**
 * @brief State constructor - Full lifecycle
 * @param enterFunction  Function called once on state entry
 * @param updateFunction Function called every loop
 * @param exitFunction   Function called once on state exit
 * 
 * Creates a state with complete lifecycle management.
 */
State::State(void(*enterFunction)(), void(*updateFunction)(), void(*exitFunction)()) {
	userEnter = enterFunction;
	userUpdate = updateFunction;
	userExit = exitFunction;
}

/**
 * @brief Execute enter callback
 * 
 * Called by FSM when entering this state.
 * Only runs if userEnter was provided.
 */
void State::enter() {
	if (userEnter) {
		userEnter();         // Call user's enter function
	}
}

/**
 * @brief Execute update callback
 * 
 * Called by FSM every loop while in this state.
 * Only runs if userUpdate was provided.
 */
void State::update() {
	if (userUpdate) {
		userUpdate();        // Call user's update function
	}
}

/**
 * @brief Execute exit callback
 * 
 * Called by FSM when leaving this state.
 * Only runs if userExit was provided.
 */
void State::exit() {
	if (userExit) {
		userExit();          // Call user's exit function
	}
}

// ═════════════════════════ FSM CLASS IMPLEMENTATION ═════════════════════════

/**
 * @brief FSM Constructor
 * @param current Starting state for the state machine
 * 
 * Initializes the FSM with a starting state.
 * The enter() callback will be triggered on first update().
 */
FiniteStateMachine::FiniteStateMachine(State& current) {
	needToTriggerEnter = true;      // Flag: Call enter() on first update
	currentState = nextState = &current;  // Both point to starting state
	stateChangeTime = 0;             // No time elapsed yet
}

/**
 * @brief Main FSM update method
 * @return Reference to self (enables method chaining)
 * 
 * Call this every loop cycle. It:
 *   1. Handles first-time enter() trigger
 *   2. Processes queued state transitions
 *   3. Calls current state's update()
 * 
 * Example:
 *   machine.update();  // Simple
 *   machine.update().transitionTo(nextState);  // Chained
 */
FiniteStateMachine& FiniteStateMachine::update() {
	// 🎉 First-time initialization: Trigger enter() for starting state
	if (needToTriggerEnter) {
		currentState->enter();
		needToTriggerEnter = false;
	}
	else {
		// 🔄 Check if a state transition is queued
		if (currentState != nextState) {
			immediateTransitionTo(*nextState);  // Execute transition now
		}
		// ▶️ Run current state's update logic
		currentState->update();
	}
	return *this;  // Enable method chaining
}

/**
 * @brief Queue a state transition (deferred execution)
 * @param state The state to transition to
 * @return Reference to self (enables method chaining)
 * 
 * Transition happens before next update() call.
 * Use this for most transitions (allows current update to complete).
 */
FiniteStateMachine& FiniteStateMachine::transitionTo(State& state) {
	nextState = &state;              // Queue the next state
	stateChangeTime = millis();      // Record transition time
	return *this;
}

/**
 * @brief Execute state transition immediately
 * @param state The state to transition to
 * @return Reference to self (enables method chaining)
 * 
 * Transition happens NOW:
 *   1. Current state's exit() runs
 *   2. State pointers updated
 *   3. New state's enter() runs
 * 
 * Use this when you need instant transitions (rare).
 */
FiniteStateMachine& FiniteStateMachine::immediateTransitionTo(State& state) {
	currentState->exit();            // 🚪 Exit current state
	currentState = nextState = &state; // 🔄 Switch to new state
	currentState->enter();           // 👋 Enter new state
	stateChangeTime = millis();      // ⏰ Record transition time
	return *this;
}

/**
 * @brief Get the current state object
 * @return Reference to current State
 * 
 * Useful for debugging or advanced state queries.
 */
State& FiniteStateMachine::getCurrentState() {
	return *currentState;
}

/**
 * @brief Check if FSM is in a specific state
 * @param state The state to check against
 * @return true if current state matches, false otherwise
 * 
 * Example:
 *   if (machine.isInState(activeState)) {
 *     // Do something only in active state
 *   }
 */
boolean FiniteStateMachine::isInState(State &state) const {
	if (&state == currentState) {
		return true;
	}
	else {
		return false;
	}
}

/**
 * @brief Get time elapsed in current state
 * @return Milliseconds since entering current state
 * 
 * Useful for timeouts and duration-based logic.
 * 
 * Example:
 *   if (machine.timeInCurrentState() > 5000) {
 *     // Been in this state for 5+ seconds
 *     machine.transitionTo(timeoutState);
 *   }
 */
unsigned long FiniteStateMachine::timeInCurrentState() {
	return millis() - stateChangeTime;
}

/*
 * ═════════════════════════════════════════════════════════════════════
 * 🎉 END OF FILE - Finite State Machine Implementation
 * 
 * Usage Summary:
 * 
 *   1. Define State objects with callbacks:
 *      State idle = State(idleUpdate);
 *      State active = State(activeEnter, activeUpdate, activeExit);
 * 
 *   2. Create FSM with starting state:
 *      FSM machine = FSM(idle);
 * 
 *   3. In loop(), check conditions and transition:
 *      if (condition) machine.transitionTo(active);
 *      machine.update();
 * 
 *   4. Query state:
 *      if (machine.isInState(active)) { ... }
 *      unsigned long t = machine.timeInCurrentState();
 * 
 * Perfect for:
 *   • Game state management (menu, playing, paused)
 *   • Robot behavior (idle, searching, attacking)
 *   • UI flow (splash, home, settings)
 *   • Protocol states (connecting, connected, disconnected)
 *   • Animation sequences
 * 
 * ═════════════════════════════════════════════════════════════════════
 */
