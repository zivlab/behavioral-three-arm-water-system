#include "defs.h"

#include "CmdMessenger.h"


extern CmdMessenger c;


void set_default_configuration(void) {
  water_system.configuration.beam_break_threshold = 0.5;
  water_system.configuration.dispensing_duration = 100;
  water_system.configuration.dispensing_probability = 1;
}

void reset_system(void) {
  water_system.operation_mode = DIRECT_VALVE_CONTROL;
  water_system.current_reward_position = 0;
  water_system.first_edge_visit = true;
  water_system.phase = LINEAR_TRACK;

  // Analog pin 2 is disconnected, thus providing random
  // values for the seed.
  randomSeed(analogRead(2));
  
  reset_trace();

  set_default_configuration();
}

void setup() {
  Serial.begin(BAUD_RATE);
  
  attach_callbacks();
  
  for (int i = 0; i < NUMBER_OF_REWARD_POSITIONS; ++i) {
    pinMode(led_pins[i], OUTPUT);
    // Turn on LEDs
    digitalWrite(led_pins[i], HIGH);

    // NOTE: The following line is mandatory. While the default
    // mode of the pins is indeed output, it also includes
    // setting the internal pull up resistor to on, which
    // limits the amount of current which is possible to
    // output on the output pins.
    pinMode(valves_pins[i], OUTPUT);
    // Close all valves
    digitalWrite(valves_pins[i], LOW);
  }

  // Turn off the LED in the reward arm.
  // (It will be turned on when a reward
  // is available).
  digitalWrite(led_pins[2], LOW);

  reset_system();
}

void sample_beam_break_baseline(void) {
  int baseline_average[NUMBER_OF_REWARD_POSITIONS][BEAM_BREAK_SAMPLE_WINDOW] = {0};

  for (int i = 0; i < BEAM_BREAK_SAMPLE_WINDOW; i++) {
    for (int j = 0; j < NUMBER_OF_REWARD_POSITIONS; j++) {
      baseline_average[j][i] = analogRead(beam_break_pins[j]);

      pause_millis(BEAM_BREAK_SAMPLE_PAUSE);
    }
  }

  for (int k = 0; k < NUMBER_OF_REWARD_POSITIONS; k++) {
    int sum = 0;
    for (int i = 0; i < BEAM_BREAK_SAMPLE_WINDOW; i++) {
      sum += baseline_average[k][i];
    }
    water_system.baseline_beam_break_value[k] = sum / BEAM_BREAK_SAMPLE_WINDOW;
  }
}

void update_trace(int reward_position) {
  // Trace is circular
  int trace_position = water_system.stats.number_of_rewards_dispensed % MAXIMUM_TRACE_LENGTH;
  
  water_system.stats.dispense_trace[trace_position].timestamp = millis();
  water_system.stats.dispense_trace[trace_position].position = (unsigned char)reward_position;

  water_system.stats.number_of_rewards_dispensed++;
}
   
void dispense_reward(int reward_position) {
  // Generate a random number in the range 0.00 - 0.99
  // to be used for comparison with our intended
  // dispensing probability.
  if ((random(0, 100) / 100.0) < water_system.configuration.dispensing_probability) {
    digitalWrite(valves_pins[reward_position], HIGH);
    pause_millis(water_system.configuration.dispensing_duration);
    digitalWrite(valves_pins[reward_position], LOW);
  
    update_trace(reward_position);
  }

  // Make sure the system pauses for 500 ms to eliminate occurrences of
  // incorrect successive dispenses
  pause_millis(500);
}

void advance_reward_position() {
  water_system.current_reward_position++;

  if (water_system.current_reward_position == LINEAR_TRACK_REWARDS) {
    water_system.current_reward_position = 0;
  }
}

int get_reward_position(int beam_break_state[NUMBER_OF_REWARD_POSITIONS]) {
  int reward_position = -1;
  
  if (beam_break_state[water_system.current_reward_position] == LOW) {
    reward_position = water_system.current_reward_position;
  }

  return reward_position;
}

void read_beam_break_state(int beam_break_state[NUMBER_OF_REWARD_POSITIONS]) {
  for (int i = 0; i < NUMBER_OF_REWARD_POSITIONS; i++) {
    int beam_break_value = analogRead(beam_break_pins[i]);

    beam_break_state[i] = beam_break_value > (water_system.baseline_beam_break_value[i] * water_system.configuration.beam_break_threshold) ? HIGH : LOW;
  }
}

void operate_reward_dispensing() {
  // Dispense reward according to the position of
  // the mouse and the next location of the reward.
  
  int beam_break_state[NUMBER_OF_REWARD_POSITIONS] = {0};
  int reward_position = 0;

  read_beam_break_state(beam_break_state);
  reward_position = get_reward_position(beam_break_state);

  if (reward_position == -1) {
    return;
  }

  if (water_system.phase == LINEAR_TRACK) {
    // Check positions 0, 1
    if (reward_position < LINEAR_TRACK_REWARDS) {
      dispense_reward(reward_position);

      // TODO: This condition could be more random, or
      // depend on time.
      if (water_system.stats.number_of_rewards_dispensed % REWARDS_FOR_ARM == 0) {
        water_system.phase = REWARD_ARM;
        digitalWrite(led_pins[2], HIGH);
      } else {      
        advance_reward_position();
      }
    }
  } else {
    if (reward_position == LINEAR_TRACK_REWARDS) {
      // TODO: Add beep from speaker
      dispense_reward(reward_position);
      water_system.phase = LINEAR_TRACK;
      digitalWrite(led_pins[2], LOW);
      // TODO: advance_reward_position? Or keep the 
      // position from last time
    }
  }
}

void process_commands() {
  // The call to the following function is instantaneous in the case
  // where no user serial input exists.
  c.feedinSerialData();
}

void operate() {
  switch (water_system.operation_mode) {
  case DIRECT_VALVE_CONTROL:
    // Opening and closing of the valves occurrs during
    // the operating callback.
    break;
  case REWARD_DISPENSING:
    operate_reward_dispensing();
    break;
  }
}

void loop() {
  process_commands();

  operate();
}
