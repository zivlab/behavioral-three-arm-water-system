#include "defs.h"

#include "CmdMessenger.h"


CmdMessenger c = CmdMessenger(Serial, ',', ';', '/');


void probe(void) {
  // Send an answer to signify that the system is
  // indeed connected and is active
  
  c.sendCmd(response_probe, F("water_system_normal"));
}

void reset_trace(void) {
  water_system.stats.reset_time = millis();
  water_system.stats.number_of_rewards_dispensed = 0;
}

void set_operation_mode(void) {
  water_system.operation_mode = c.readBinArg<int>();

  water_system.current_reward_position = 0;
  water_system.previous_edge = -1;

  sample_beam_break_baseline();
}

void set_beam_break_threshold(void) {
  water_system.configuration.beam_break_threshold = c.readBinArg<float>();
}

void set_dispensing_duration(void) {
  water_system.configuration.dispensing_duration = c.readBinArg<int>();
}

void set_dispensing_probability(void) {
  water_system.configuration.dispensing_probability = c.readBinArg<float>();
}

void set_valves_state(void) {
  for (int i = 0; i < NUMBER_OF_REWARD_POSITIONS; i++) {
    digitalWrite(valves_pins[i], c.readBinArg<int>());
  }
}

void get_stats(void) {
  c.sendCmdStart(response_stats);
  c.sendCmdBinArg(water_system.stats.reset_time);
  c.sendCmdBinArg(water_system.stats.number_of_rewards_dispensed);
  for (int i = 0; i < MAXIMUM_TRACE_LENGTH; i++) {
    c.sendCmdBinArg(water_system.stats.dispense_trace[i].timestamp);
    c.sendCmdBinArg(water_system.stats.dispense_trace[i].position);
  }
  c.sendCmdEnd();
}

void on_unknown_command(void){
  c.sendCmd(response_error, "Unknown command");
}

void attach_callbacks(void) {
  c.attach(command_probe, probe);
  c.attach(command_reset_trace, reset_trace);
  c.attach(command_set_operation_mode, set_operation_mode);
  c.attach(command_set_beam_break_threshold, set_beam_break_threshold);
  c.attach(command_set_dispensing_duration, set_dispensing_duration);
  c.attach(command_set_dispensing_probability, set_dispensing_probability);
  c.attach(command_set_valves_state, set_valves_state);
  c.attach(command_get_stats, get_stats);
  c.attach(on_unknown_command);
}
