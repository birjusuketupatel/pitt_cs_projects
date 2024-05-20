import pandas as pd
import numpy as np
import nltk
import gensim
import string
import random
import re
import pickle
from sklearn.model_selection import KFold, train_test_split
from sklearn.feature_extraction.text import CountVectorizer, TfidfVectorizer
from sklearn.linear_model import LogisticRegression
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import classification_report
from sklearn.metrics import confusion_matrix
from scipy.sparse import csr_matrix, hstack
from gensim.models import KeyedVectors

class CustomVectorizer(TfidfVectorizer):
    def negate(self, doc):
        """Negates all words after a negation until a punctuation mark is found.
        Ex: "I didn't like that approach, it's no good." -> "I didn't NOT_like NOT_that NOT_apprach, it's no NOT_good."
        """
        punctuation = set(string.punctuation)
        negatives = {"no", "not", "neither", "nor", "never"}
        negate = False
        new_s = ""
        for s in doc:
            if s in punctuation:
                negate = False
                new_s = s
            elif s in negatives or re.search("n\'t", s) is not None:
                negate = True
                new_s = s
            elif negate:
                new_s = "NOT_" + s
            else:
                new_s = s
            yield new_s

    def build_tokenizer(self):
        tokenize = super().build_tokenizer()
        stemmer = nltk.stem.SnowballStemmer(language="english")
        return lambda doc: [stemmer.stem(s) for s in self.negate(tokenize(doc))]

def extract_errors(actual, predicted):
    """Finds false positives and false negatives given actual vs predicted data.
    """
    if len(actual) != len(predicted):
        raise Exception("input arrays are of differing lengths")

    false_negatives = []
    false_positives = []
    for i in range(len(actual)):
        if predicted[i] == 0 and actual[i] == 1:
            false_negatives.append(i)
        elif predicted[i] == 1 and actual[i] == 0:
            false_positives.append(i)

    return false_negatives, false_positives

def train_lr_unigram(train):
    """Returns a logistic regression classifier using simple bag of words
    and the vectorizer used to create it.
    """
    # tokenizes and vectorizes data
    vectorizer = CountVectorizer(min_df=1, tokenizer=nltk.word_tokenize)
    vectorizer.fit(train["text"])

    # extract features and targets
    text = vectorizer.transform(train["text"])
    polite = train["polite"]

    # train logistic regression model
    lr = LogisticRegression(multi_class="ovr", max_iter=200)
    lr.fit(text, polite)

    # save model
    pickle.dump(lr, open("models/lr_unigram.sav", "wb"))
    pickle.dump(vectorizer, open("models/lr_unigram_vectorizer.sav", "wb"))

    return lr, vectorizer

def train_lr_custom(train):
    """Logistic regression with custom features.
    Features:
        - Applies stemming to input text.
        - Applies negation to all words that follow a negative word.
        - Uses TF-IDF vectorization.
        - Adds document length as a feature.
    Returns classifier and vectorizer.
    """
    # preprocesses, tokenizes, normalizes, and vectorizes data
    vectorizer = CustomVectorizer(min_df=1, tokenizer=nltk.word_tokenize)
    vectorizer.fit(train["text"])

    # generate tf-idf matrix
    feature_matrix = vectorizer.transform(train["text"])

    # get lengths of each document and add to feature matrix
    word_counts = lambda doc: len(doc.split())
    len_vec =  train["text"].apply(word_counts)
    hstack([feature_matrix, csr_matrix.transpose(csr_matrix(len_vec))])

    # train logistic regression model
    lr = LogisticRegression(multi_class="ovr")
    lr.fit(feature_matrix, train["polite"])

    # save model
    pickle.dump(lr, open("models/lr_custom.sav", "wb"))
    pickle.dump(vectorizer, open("models/lr_custom_vectorizer.sav", "wb"))

    return lr, vectorizer

def test_lr(lr, vectorizer, test, n=0):
    """Tests given logistic regression classifier.
    Prints performance statistics and confusion matrix.
    Prints list of n false negatives and n false positives.
    """
    # tokenizes and vectorizes data
    text = vectorizer.transform(test["text"])
    polite = test["polite"]

    # evaluate model
    polite_predict = lr.predict(text)
    print("\nSummary Statistics-")
    print(pd.DataFrame(classification_report(polite, polite_predict, output_dict=True)))
    print("\nConfusion Matrix-")
    print(pd.DataFrame(confusion_matrix(polite, polite_predict)))

    # prints n randomly selected false positives and false negatives
    false_negatives, false_positives = extract_errors(polite.tolist(), polite_predict.tolist())
    print("\nSample False Negatives-")
    for i in false_negatives[:n]:
        print(" - " + test.iloc[i]["text"])
    print("\nSample False Positives-")
    for i in false_positives[:n]:
        print(" - " + test.iloc[i]["text"])

    # get top n most important features associated with politeness and impoliteness
    feature_names = vectorizer.get_feature_names_out()
    coefs_with_fns = sorted(zip(lr.coef_[0], feature_names))

    print("Top Features in Impolite Comments-")
    for coef, feature in coefs_with_fns[:n]:
        print(" - feature: \"" + feature + "\", coefficient: " + str(coef))
    print("Top Features in Polite Comments-")
    for coef, feature in list(reversed(coefs_with_fns))[:n]:
        print(" - feature: \"" + feature + "\", coefficient: " + str(coef))


def convert_doc_to_vector(text, word_vectorizer, max_len):
    """Returns vectorized representation of a comment using word embeddings.
    """
    row_size = word_vectorizer.vector_size
    matrix = np.empty((0, row_size))
    for i in range(max_len):
        # pads with zeros so that all vectors are a standard size
        next_vector = np.zeros([1, row_size])
        if i < len(text):
            try:
                # find vector representation of each word
                next_vector = word_vectorizer.get_vector(text[i])
            except:
                # if word is unrecognized, put zeros in its place.
                next_vector = np.zeros([1, row_size])
        matrix = np.vstack([matrix, next_vector])
    return matrix.flatten()


def train_mlp(train):
    """Trains a feedforward neural network classifier.
    """
    # import static GloVe embeddings
    glove_vectors = gensim.models.KeyedVectors.load("glove-twitter-25", mmap="r")

    # tokenizing, stemming, and filtering raw data
    stemmer = nltk.stem.SnowballStemmer(language="english")
    stops = set(nltk.corpus.stopwords.words('english'))
    clean_text = train["text"].apply(nltk.word_tokenize)

    # construct feature matrix
    max_len = np.amax(train["text"].apply(len))
    mean_len = np.mean(train["text"].apply(len))
    std_dev_len = np.std(train["text"].apply(len))
    vec_size = int(min(max_len, mean_len + 2 * std_dev_len))
    text_vectors = clean_text.apply(lambda doc: convert_doc_to_vector(doc, glove_vectors, vec_size))
    feature_matrix = np.array(text_vectors.tolist())

    # train neural network
    clf = MLPClassifier(hidden_layer_sizes=(200, 200))
    clf.fit(feature_matrix, train["polite"].to_numpy())

    # save model
    pickle.dump(clf, open("models/neural_network.sav", "wb"))

    return clf

def test_mlp(clf, test, n=0):
    """Tests feedforward neural network classifier.
    Prints summary statistics of accuracy and precision.
    Generates confusion matrix.
    """
    # import static GloVe embeddings
    glove_vectors = gensim.models.KeyedVectors.load("glove-twitter-25", mmap="r")

    # tokenizing, stemming, and filtering raw data
    stemmer = nltk.stem.SnowballStemmer(language="english")
    stops = set(nltk.corpus.stopwords.words('english'))
    clean_text = test["text"].apply(lambda doc: [stemmer.stem(s) for s in nltk.word_tokenize(doc) if s not in stops])

    # construct feature matrix
    vec_size = int(len(clf.coefs_[0]) / glove_vectors.vector_size)
    text_vectors = clean_text.apply(lambda doc: convert_doc_to_vector(doc, glove_vectors, vec_size))
    feature_matrix = np.array(text_vectors.tolist())

    polite_predict = clf.predict(feature_matrix)
    print("\nNeural Network:")
    print("Summary Statistics-")
    print(pd.DataFrame(classification_report(test["polite"], polite_predict, output_dict=True)))
    print("\nConfusion Matrix-")
    print(pd.DataFrame(confusion_matrix(test["polite"], polite_predict)))

    # prints n randomly selected false positives and false negatives
    false_negatives, false_positives = extract_errors(polite_predict.tolist(), test["polite"].tolist())
    print("\nSample False Negatives-")
    for i in false_negatives[:n]:
        print(" - " + test.iloc[i]["text"])
    print("\nSample False Positives-")
    for i in false_positives[:n]:
        print(" - " + test.iloc[i]["text"])


def k_fold_validation(data, k, model):
    kfold = KFold(n_splits=k)

    for train, test in kfold.split(data):
        if model == "lin_reg_unigram":
            print("training model...")
            lr, vec = train_lr_unigram(data.iloc[train])
            print("testing model...")
            test_lr(lr, vec, data.iloc[test])
        elif model == "lin_reg_custom":
            print("training model...")
            lr, vec = train_lr_custom(data.iloc[train])
            print("testing model...")
            test_lr(lr, vec, data.iloc[test])
        elif model == "neural_net":
            print("training model...")
            clf = train_mlp(data.iloc[train])
            print("testing model...")
            test_mlp(clf, data.iloc[test])
