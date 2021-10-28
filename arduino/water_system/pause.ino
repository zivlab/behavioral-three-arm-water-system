void pause_millis(int dispensing_duration) {
  unsigned long start_time = millis();
  unsigned long current_time = start_time;
 
  // How much time has passed, accounting for rollover with subtraction!
  // https://www.baldengineer.com/arduino-how-do-you-reset-millis.html
  while ((unsigned long)(current_time - start_time) < dispensing_duration) {
    current_time = millis();
  }
}
