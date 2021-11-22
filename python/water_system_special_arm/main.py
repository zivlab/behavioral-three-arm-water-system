"""Water controller module.

Controls the lab's water system. Connects by serial port to the
device itself which then controls the solenoids to let liquids through.
"""

import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))

import glob
import serial

from water_system import WaterSystem, OPERATION_MODES


def command_reset_trace(water_system: WaterSystem):
    water_system.reset_trace()


def command_set_operation_mode(water_system: WaterSystem):
    for i, str_ in enumerate(OPERATION_MODES):
        print(f"{i + 1} - {str_}")

    try:
        # Compensate for the 1-based indexing of the printing
        operation_mode = (
            int(input("Choose an operation mode by its number: ")) - 1
        )
    except Exception as e:
        print(f"Error in input! ({e})")
        return

    water_system.set_operation_mode(operation_mode)


def command_set_dispensing_duration(water_system: WaterSystem):
    try:
        duration = int(input("Enter dispensing duration in milliseconds: "))
    except Exception as e:
        print(f"Error in input! ({e})")
        return

    water_system.set_dispensing_duration(duration)


def command_set_dispensing_probability(water_system: WaterSystem):
    try:
        probability = float(input("Enter dispensing probability [0.0, 1.0]: "))
    except Exception as e:
        print(f"Error in input! ({e})")
        return

    water_system.set_dispensing_probability(probability)


def command_set_beam_break_threshold(water_system: WaterSystem):
    try:
        threshold = float(input("Enter beam break threshold [0.0, 1.0]: "))
    except Exception as e:
        print(f"Error in input! ({e})")
        return

    water_system.set_beam_break_threshold(threshold)


def command_toggle_solenoids(water_system: WaterSystem):
    water_system.solenoid_state = [not v for v in water_system.solenoid_state]

    water_system.set_valves_state(water_system.solenoid_state)


def command_get_stats(water_system: WaterSystem):
    result = water_system.get_stats()
    if result is None:
        print("Failed getting stats")
        return

    reset_time, dispenses = result

    print(f"Reset time: {reset_time}")
    print(f"Number of rewards dispensed: {len(dispenses)}")

    print(dispenses)


COMMANDS = [
    ("Set operation mode", command_set_operation_mode),
    ("Set dispensing duration", command_set_dispensing_duration),
    ("Set dispensing probability", command_set_dispensing_probability),
    ("Set beam break threshold", command_set_beam_break_threshold),
    ("Reset trace", command_reset_trace),
    ("Toggle solenoids", command_toggle_solenoids),
    ("Get stats", command_get_stats),
    ("Quit", sys.exit),
]


def list_serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith("win"):
        ports = ["COM%s" % (i + 1) for i in range(256)]
    elif sys.platform.startswith("linux") or sys.platform.startswith("cygwin"):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob("/dev/tty[A-Za-z]*")
    elif sys.platform.startswith("darwin"):
        ports = glob.glob("/dev/tty.*")
    else:
        raise EnvironmentError("Unsupported platform")

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass

    return result


def print_menu():
    print("Menu:")
    for i, (description, _) in enumerate(COMMANDS):
        # Print menu using 1-based indexing
        print(f"{i + 1} - {description}")


def main_loop(water_system: WaterSystem):
    water_system.init()

    while True:
        print_menu()

        try:
            # Compensate for menu using 1-based indexing
            command_id = int(input("Enter command id: ")) - 1
        except Exception:
            print("Error in entered command id!")
            continue

        if command_id >= len(COMMANDS) or command_id < 0:
            print("Illegal command number chosen!")
            continue

        # Compensate for menu using 1-based indexing
        _, function = COMMANDS[command_id]

        try:
            function(water_system)
        except Exception as e:
            print(f"Failed executing command ({e})")


def find_water_system_port(ports):
    for port in ports:
        water_system = WaterSystem(port)

        water_system._send_command("probe")
        result = water_system._receive_result()
        if result is None:
            continue

        if result[1][0] == "three_arm_water_system":
            return water_system

    return None


def main():
    serial_ports = list_serial_ports()

    # Probe for the water system upon all available COM ports
    water_system = find_water_system_port(serial_ports)
    if water_system is None:
        print(
            "Water system was not found on any of the available ports!", serial_ports
        )

        return

    main_loop(water_system)


if __name__ == "__main__":
    main()
