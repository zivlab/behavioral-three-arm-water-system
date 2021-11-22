import time

import PyCmdMessenger

NUMBER_OF_VALVES = 3
MAXIMUM_TRACE_LENGTH = 400
ARDUINO_BAUDRATE = 9600

OPERATION_MODES = [
    "Direct valve control",
    "Reward dispensing",
]

ARDUINO_COMMANDS = [
    ["probe", ""],
    ["reset_trace", ""],
    ["set_operation_mode", "i"],
    ["set_beam_break_threshold", "f"],
    ["set_dispensing_duration", "i"],
    ["set_dispensing_probability", "f"],
    ["set_valves_state", "i" * NUMBER_OF_VALVES],
    ["get_stats", ""],
]

ARDUINO_RESPONSES = [
    ["probe_result", "s"],
    ["stats", "Li" + "Lb" * MAXIMUM_TRACE_LENGTH],
    ["error", "s"],
]


class ProbeMismatch(Exception):
    pass


class WaterSystem:
    def __init__(self, device):
        arduino = PyCmdMessenger.ArduinoBoard(device, baud_rate=ARDUINO_BAUDRATE)
        self._cmd_messenger = PyCmdMessenger.CmdMessenger(
            arduino, ARDUINO_COMMANDS + ARDUINO_RESPONSES
        )

        # False is closed
        self.solenoid_state = [False] * NUMBER_OF_VALVES

    def _send_command(self, command, *args):
        self._cmd_messenger.send(command, *args)

    def _receive_result(self, timeout=1):
        result = self._cmd_messenger.receive()
        start_time = time.time()
        while result is None and time.time() - start_time < timeout:
            result = self._cmd_messenger.receive()

        if result is None:
            return None

        return result

    def init(self):
        self._send_command("probe")
        probe = self._receive_result()
        if probe is None or not (
            probe[0] == "probe_result" and probe[1][0] == "water_system_normal"
        ):
            raise ProbeMismatch(probe)

        self.set_valves_state(self.solenoid_state)

    def reset_trace(self):
        self._send_command("reset_trace")

    def set_operation_mode(self, mode):
        self._send_command("set_operation_mode", mode)

    def set_beam_break_threshold(self, threshold: float):
        self._send_command("set_beam_break_threshold", threshold)

    def set_dispensing_duration(self, duration):
        self._send_command("set_dispensing_duration", duration)

    def set_dispensing_probability(self, probability: float):
        self._send_command("set_dispensing_probability", probability)

    def set_valves_state(self, state):
        """Set the states according to a list of the form [False, True, ...] where 'False' is closed."""
        self._send_command(
            "set_valves_state", *[int(state[i]) for i in range(NUMBER_OF_VALVES)]
        )

    def get_stats(self, timeout=1):
        self._send_command("get_stats")

        # Persistent loop over the stats. This loop might be necessary
        # if the Arduino is still processing previous commands.
        result = self._receive_result(timeout)
        while result is not None and result[0] != "stats":
            result = self._receive_result(timeout)

        if result is None:
            return None

        stats = result[1]

        reset_time, number_of_rewards_dispensed = stats[:2]
        trace = stats[2 : 2 + number_of_rewards_dispensed * 2]
        trace_iter = iter(trace)

        return reset_time, list(zip(trace_iter, trace_iter))


if __name__ == "__main__":
    w = WaterSystem("COM5")

    print(w.get_stats())
