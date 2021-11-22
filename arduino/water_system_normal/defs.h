#ifndef __DEFS_H__
#define __DEFS_H__

#define NUMBER_OF_REWARD_POSITIONS (3)
#define MAXIMUM_TRACE_LENGTH (400)
#define BAUD_RATE (9600)

#define BEAM_BREAK_SAMPLE_WINDOW (16)
#define BEAM_BREAK_SAMPLE_PAUSE (2)

const int beam_break_pins[NUMBER_OF_REWARD_POSITIONS] = {0, 1, 2};
const int led_pins[NUMBER_OF_REWARD_POSITIONS] = {2, 3, 4};
const int valves_pins[NUMBER_OF_REWARD_POSITIONS] = {5, 6, 7};

enum operation_modes {
  DIRECT_VALVE_CONTROL = 0,
  REWARD_DISPENSING,
  ALTERNATING_REWARD_DISPENSING,
};

// CmdMessenger commands and telemetries
enum {
  command_probe,
  command_reset_trace,
  command_set_operation_mode,
  command_set_beam_break_threshold,
  command_set_dispensing_duration,
  command_set_dispensing_probability,
  command_set_valves_state,
  command_get_stats,

  response_probe,
  response_stats,
  response_error,
};

typedef struct {
  unsigned long timestamp;
  unsigned char position;
} dispense_trace_t;

// Holds the state of the entire water system
struct {
  int operation_mode;
  int current_reward_position;

  float baseline_beam_break_value[NUMBER_OF_REWARD_POSITIONS];

  // Controls single dispensing per edge visit in non-alternating mode
  bool first_edge_visit;
  
  struct {
    // Ratio of phototransistor read values to consider
    // dark, and cause dispensing.
    float beam_break_threshold;
    // Duration, in ms, to keep the solenoids open for
    // dispensing.
    int dispensing_duration;
    // Sets the probability for dispensing (0.0-1.0) across
    // all different operation modes.
    float dispensing_probability;
  } configuration;

  struct {
    unsigned long reset_time;
    int number_of_rewards_dispensed;
    dispense_trace_t dispense_trace[MAXIMUM_TRACE_LENGTH];
  } stats;
} water_system;

#endif /* __DEFS_H__ */
