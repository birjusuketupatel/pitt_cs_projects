import json
import os
import pickle
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.svm import SVC
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import classification_report
from sklearn.metrics import confusion_matrix

def load_data():
    # reads raw typing data from /data
    typing_data = []
    fnames = os.listdir("data")
    for fname in fnames:
        with open("data/" + fname) as f:
            typing_data.append(json.load(f))
    return typing_data

"""
the vocabulary is the subset of the keys on the keyboard the model considers
defined as all uppercase letters, lowercase letters, shift, space, and backspace
returns a list containing the vocabulary
"""
def get_vocab():
    vocab = ["\'" + chr(c) + "\'" for c in range(ord('a'), ord('z')+1)]
    vocab = vocab + [s.upper() for s in vocab]
    vocab = vocab + ["<Key.shift: <160>>", "<Key.space: ' '>", "<Key.backspace: <8>>"]
    return vocab

def get_hold_times(log, vocab):
    # given a log of typing events and a list of keys in the vocabulary
    # returns a dict containing the times that each key was held down
    key_times = {}
    pressed_keys = {}
    vocab_set = set(vocab)

    for event in log:
        event_type = event[0]
        key_name = event[1]
        event_time = event[2]

        if key_name not in vocab_set:
            continue

        if event_type == 0:
            # new key pressed down
            pressed_keys[key_name] = event_time
        elif event_type == 1 and key_name in pressed_keys:
            # previously pressed key released
            hold_time = event_time - pressed_keys[key_name]
            del pressed_keys[key_name]
            if key_name not in key_times:
                key_times[key_name] = [hold_time]
            else:
                key_times[key_name].append(hold_time)

    return key_times

def get_wait_times(log, vocab):
    # given a log of typing events and a list of keys in the vocabulary
    # returns a dict containing the times between the release of one key and the pressing down of another
    key_times = {}
    released_keys = {}
    vocab_set = set(vocab)

    for event in log:
        event_type = event[0]
        key_name = event[1]
        event_time = event[2]

        if key_name not in vocab_set:
            continue

        if event_type == 1:
            # key released
            released_keys[key_name] = event_time
        elif event_type == 0:
            # next key pressed
            for item in released_keys.keys():
                wait_time = event_time - released_keys[item]
                tup = (item, key_name)
                if tup not in key_times:
                    key_times[tup] = [wait_time]
                else:
                    key_times[tup].append(wait_time)
            released_keys = {}

    return key_times

def get_backspace_count(log):
    # given a log of typing events
    # returns the number of times that backspace was pressed
    backspace_count = 0

    for event in log:
        event_type = event[0]
        key_name = event[1]

        if event_type == 0 and key_name == "<Key.backspace: <8>>":
            backspace_count += 1

    return backspace_count

"""
extracts features from a list dictionary containing the training data
stores the following features into a dataframe
each row represents a single typed phrase
 - the average time each key in the vocabulary was pressed down
 - the average time between a key being released and the next key being pressed
 - the variance of press down times for the 10 most common letters in English
 - the number of times backspace is pressed
 - the name of the subject that generated this data
"""
def extract_features(test_data, vocab, fixed_phrase=False, target_name=None):
    pairs = [(i, j) for i in vocab for j in vocab]
    common_letters = ["e", "t", "a", "o", "i", "n", "s", "r", "h", "l"]
    num_features = len(vocab) + len(pairs) + len(common_letters) + 2
    cols = vocab + [str(pair) for pair in pairs] + ["var_" + letter for letter in common_letters] + ["backspaces", "name"]

    vocab_index = {}
    pair_index = {}
    for i in range(len(vocab)):
        vocab_index[vocab[i]] = i
    for i in range(len(pairs)):
        pair_index[pairs[i]] = i + len(vocab)

    matrix = []
    for run in test_data:
        name = run["name"]

        for item in run.keys():
            if not item.isnumeric():
                continue

            phrase = run[item]["phrase"]
            if fixed_phrase and phrase != "Review privacy policies before sharing.":
                continue

            log = run[item]["log"]
            holds = get_hold_times(log, vocab)

            waits = get_wait_times(log, vocab)
            backspaces = get_backspace_count(log)

            next_row = [0] * num_features
            for key in holds.keys():
                avg_hold_time = sum(holds[key]) / len(holds[key])
                next_row[vocab_index[key]] = avg_hold_time
            for pair in waits.keys():
                avg_wait_time = sum(waits[pair]) / len(waits[pair])
                next_row[pair_index[pair]] = avg_wait_time
            for index in range(len(common_letters)):
                key = "\'" + common_letters[index] + "\'"
                if key in holds and len(holds[key]) > 0:
                    mean = sum(holds[key]) / len(holds[key])
                    var = sum((i - mean) ** 2 for i in holds[key]) / len(holds[key])
                    next_row[len(vocab) + len(pairs) + index] = var
            next_row[len(next_row) - 2] = backspaces

            if target_name == None:
                next_row[len(next_row) - 1] = name
            else:
                if name == target_name:
                    next_row[len(next_row) - 1] = 1
                else:
                    next_row[len(next_row) - 1] = 0

            matrix.append(next_row)

    return pd.DataFrame(matrix, columns=cols)

def train_rf(train):
    x_train = train.drop("name", axis=1)
    y_train = train["name"]
    rf = RandomForestClassifier(max_depth=6, max_features=None, max_leaf_nodes=6, n_estimators=150)
    rf.fit(x_train, y_train)
    return rf

def train_svm(train):
    x_train = train.drop("name", axis=1)
    y_train = train["name"]
    svm = SVC(C=0.1, gamma=1, kernel='linear')
    svm.fit(x_train, y_train)
    return svm

def train_lr(train):
    x_train = train.drop("name", axis=1)
    y_train = train["name"]
    lr = LogisticRegression(C=100, penalty="l1", solver="liblinear", max_iter=300)
    lr.fit(x_train, y_train)
    return lr

def save_model(model, fname):
    pickle.dump(model, open(fname, "wb"))
    return

def test_model(model, test):
    x_test = test.drop("name", axis=1)
    y_test = test["name"]
    y_predict = model.predict(x_test)
    print("Summary Statistics-")
    print(pd.DataFrame(classification_report(y_test, y_predict, output_dict=True)))
    print("\nConfusion Matrix-")
    print(pd.DataFrame(confusion_matrix(y_test, y_predict)))
    return

def load_model(fname):
    return pickle.load(open(fname, "rb"))

def main():
    name = "Birju"

    # loading data
    raw_typing_data = load_data()

    # extracting features
    vocab = get_vocab()
    matrix = extract_features(raw_typing_data, vocab, fixed_phrase=True, target_name=name)

    # training models
    train, test = train_test_split(matrix, test_size=0.2)
    rf = train_rf(train)
    svm = train_svm(train)
    lr = train_lr(train)

    # testing models
    print("Results for " + name + ".")
    print("Model trained on data from one phrase only.")
    print("Random Forest:")
    test_model(rf, test)
    print("\nSVM:")
    test_model(svm, test)
    print("\nLogistic Regression:")
    test_model(lr, test)

    # extracting features
    matrix = extract_features(raw_typing_data, vocab, fixed_phrase=False, target_name=name)

    # training models
    train, test = train_test_split(matrix, test_size=0.2)
    rf = train_rf(train)
    svm = train_svm(train)
    lr = train_lr(train)

    # testing models
    print("\nResults for " + name + ".")
    print("Model trained on data from all phrases.")
    print("Random Forest:")
    test_model(rf, test)
    print("\nSVM:")
    test_model(svm, test)
    print("\nLogistic Regression:")
    test_model(lr, test)

if __name__ == '__main__':
    main()
