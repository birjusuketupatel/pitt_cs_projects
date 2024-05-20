from models import *
from gensim.downloader import load
import sys
import argparse
import os

def setup():
    nltk.download("punkt")
    nltk.download('stopwords')
    model = gensim.downloader.load('glove-twitter-25')
    model.save("glove-twitter-25")

def train(model, path):
    valid_models = ['lin_reg_unigram', 'lin_reg_custom', 'neural_net']

    if model not in valid_models:
        print("invalid model")
        return -1
    if not os.path.isfile(path):
        print("invalid file path")
        return -1

    data = pd.read_csv(path)
    train, test = train_test_split(data, test_size=0.1)
    if model == "lin_reg_unigram":
        print("training model...")
        lr, vec = train_lr_unigram(train)
        print("testing model...")
        test_lr(lr, vec, test, 5)
    elif model == "lin_reg_custom":
        print("training model...")
        lr, vec = train_lr_custom(train)
        print("testing model...")
        test_lr(lr, vec, test, 5)
    elif model == "neural_net":
        print("training model...")
        clf = train_mlp(train)
        print("testing model...")
        test_mlp(clf, test, 5)

    return 0

def test(model, path):
    valid_models = ['lin_reg_unigram', 'lin_reg_custom', 'neural_net']

    if model not in valid_models:
        print("invalid model")
        return -1
    if not os.path.isfile(path):
        print("invalid file path")
        return -1

    data = pd.read_csv(path)
    if model == "lin_reg_unigram":
        print("loading model...")
        lr = pickle.load(open("models/lr_unigram.sav", "rb"))
        vec = pickle.load(open("models/lr_unigram_vectorizer.sav", "rb"))
        print("testing model...")
        test_lr(lr, vec, data, 5)
    elif model == "lin_reg_custom":
        print("loading model...")
        lr = pickle.load(open("models/lr_custom.sav", "rb"))
        vec = pickle.load(open("models/lr_custom_vectorizer.sav", "rb"))
        print("testing model...")
        test_lr(lr, vec, data, 5)
    elif model == "neural_net":
        print("loading model...")
        clf = pickle.load(open("models/neural_network.sav", "rb"))
        print("testing model...")
        test_mlp(clf, data, 5)

    return 0

def cross_validate(model, path):
    valid_models = ['lin_reg_unigram', 'lin_reg_custom', 'neural_net']

    if model not in valid_models:
        print("invalid model")
        return -1
    if not os.path.isfile(path):
        print("invalid file path")
        return -1

    data = pd.read_csv(path)
    k_fold_validation(data, 5, model)
    return 0

def main():
    parser = argparse.ArgumentParser(description="driver for politeness classifier")
    parser.add_argument("--action", type=str, help="setup, train, test, cross-validate")
    parser.add_argument("--model", type=str, help="specifies which model to run ('lin_reg_unigram', 'lin_reg_custom', 'neural_net')")
    parser.add_argument("--file", type=str, help="path to data file")
    input = vars(parser.parse_args())

    action = input["action"]
    ret = 0
    if action == "setup":
        ret = setup()
    elif action == "train":
        ret = train(input["model"], input["file"])
    elif action == "test":
        ret = test(input["model"], input["file"])
    elif action == "cross-validate":
        ret = cross_validate(input["model", input["file"]])
    else:
        print("invalid action")

    if ret == -1:
        parser.print_help()

if __name__ == '__main__':
    main()
