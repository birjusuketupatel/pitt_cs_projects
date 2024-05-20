import typing_test
from models import *
from pynput import keyboard
from msvcrt import getch
from json import dumps

def main():
    print("To authenticate an existing user, enter 0.")
    print("To add a new user, press 1.")
    choice = -1
    while not (choice == 0 or choice == 1):
        choice = int(input())
        if not (choice == 0 or choice == 1):
            print("Please select 0 or 1.")

    if choice == 0:
        names = [fname[:-5] for fname in os.listdir("data")]
        print("\nKnown Users:")
        for name in names:
            print(" - " + name)

        name = ""
        while name not in names:
            name = input("Who are you?\n")
            if name not in names:
                print("Please select a name from the list.")

        typing_test.run_data["name"] = name

        # collect testing data
        print("You must type the following phrase three times exactly.")
        print("Your typing speed will be measured and used to authenticate you.")
        print("Please type at a normal pace.")
        choice = "n"
        while choice != "y":
            print("Press y to begin.")
            choice = getch().decode("utf-8")

        phrase = "Review privacy policies before sharing."
        completed = 0
        while completed < 3:
            next_entry = {}
            next_entry["phrase"] = phrase
            print(str(completed + 1) + ". " + phrase)

            with keyboard.Listener(on_press=typing_test.on_press, on_release=typing_test.on_release) as listener:
                answer = input("Answer: ")
                listener.join()

                if answer == phrase:
                    next_entry["log"] = typing_test.log
                    typing_test.run_data[str(completed)] = next_entry
                    completed += 1
                else:
                    print("Your answer contained mistakes. Please retype it.")
                typing_test.log = []

        # retrain logistic regression classifier
        vocab = get_vocab()
        raw_typing_data = load_data()
        matrix = extract_features(raw_typing_data, vocab, fixed_phrase=True, target_name=name)
        lr = train_lr(matrix)

        # classify user input
        matrix = extract_features([typing_test.run_data], vocab, fixed_phrase=True, target_name=name)
        x_test = matrix.drop("name", axis=1)
        y_predict = lr.predict(x_test)

        # considered to pass if 2/3 phrases are classified as having been typed by the correct user
        if sum(y_predict) >= 2:
            print("\nAccess granted to " + name + ".")
        else:
            print("\nAccess denied to " + name + ".")
    elif choice == 1:
        name = input("What is your name?\n")
        name = name.replace(" ", "")

        typing_test.run_data["name"] = name

        print("\nHello " + name + "!")
        print("You must type the same phrase 20 times.")
        print("Your typing speed will be used to authenticate you.")
        print("Please type at a normal pace. Try to maintain the speed you type at when you are working.")
        choice = "n"
        while choice != "y":
            print("Press y to begin.")
            choice = getch().decode("utf-8")

        # collect new user data
        phrase = "Review privacy policies before sharing."
        completed = 0
        while completed < 20:
            next_entry = {}
            next_entry["phrase"] = phrase
            print(str(completed + 1) + ". " + phrase)

            with keyboard.Listener(on_press=typing_test.on_press, on_release=typing_test.on_release) as listener:
                answer = input("Answer: ")
                listener.join()

                if answer == phrase:
                    next_entry["log"] = typing_test.log
                    typing_test.run_data[str(completed)] = next_entry
                    completed += 1
                else:
                    print("Your answer contained mistakes. Please retype it.")
                typing_test.log = []

        # save to file
        output_data = dumps(typing_test.run_data)
        output_fname = "data/" + name + ".json"
        with open(output_fname, "w") as outf:
            outf.write(output_data)

if __name__ == '__main__':
    main()
