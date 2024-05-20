from pynput import keyboard
from msvcrt import getch
from time import time
from json import dumps

"""
stores data for each run in data/<name>.json
each phrase is stored with a time series of typing events

event record: [type, key, timestamp]
type: int, press down = 0, release = 1
key: string, unique identifier for each key
timestamp: float, unix timestamp in milliseconds
"""

run_data = {}
pressed_keys = {}
log = []

def on_press(key):
    press_time = time() * 1000
    if key not in pressed_keys:
        pressed_keys[key] = press_time
        log.append([0, repr(key), press_time])

def on_release(key):
    if key == keyboard.Key.enter:
        # Stop listener
        return False

    release_time = time() * 1000
    if key in pressed_keys:
        del pressed_keys[key]
        log.append([1, repr(key), release_time])

def main():
    name = input("What is your name?\n")
    name = name.replace(" ", "")
    print("\nHello " + name + "! Welcome to the typing test.")
    print("You must type a series of short phrases.")
    print("After finishing one phrase, hit enter to move on to the next.")
    print("You must match the phrase exactly, or else you will not advance.")
    print("You will type 50 total phrases. There may be repeats.")
    print("Please type at a normal pace. Try to maintain the speed you type at when you are working.")
    choice = "n"
    while choice != "y":
        print("Press y to begin.")
        choice = getch().decode("utf-8")

    global log, run_data
    run_data["name"] = name

    lines = []
    with open("phrases.txt", "r") as file:
        lines = [line.strip("\n") for line in file.readlines()]

    completed = 0
    while(completed < 50):
        next_entry = {}
        next_entry["phrase"] = lines[completed]
        print("\n" + str(completed + 1) + ". " + lines[completed])

        with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
            answer = input("Answer: ")
            listener.join()

            if answer == lines[completed]:
                next_entry["log"] = log
                run_data[str(completed)] = next_entry
                completed += 1
            else:
                print("Your answer contained mistakes. Please retype it.")

            log = []

    output_data = dumps(run_data)
    output_fname = "data/" + name + ".json"
    with open(output_fname, "w") as outf:
        outf.write(output_data)

if __name__ == '__main__':
    main()
