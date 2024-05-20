import os
import subprocess
import csv
import re
import random
import numpy as np
import scipy
import math

def read_in_shakespeare():
    """Reads in the Shakespeare dataset and processes it into a list of tuples.
       Also reads in the vocab and play name lists from files.

    Each tuple consists of
    tuple[0]: The name of the play
    tuple[1] A line from the play as a list of tokenized words.

    Returns:
      tuples: A list of tuples in the above format.
      document_names: A list of the plays present in the corpus.
      vocab: A list of all tokens in the vocabulary.
    """

    tuples = []

    with open("shakespeare_plays.csv") as f:
        csv_reader = csv.reader(f, delimiter=";")
        for row in csv_reader:
            play_name = row[1]
            line = row[5]
            line_tokens = re.sub(r"[^a-zA-Z0-9\s]", " ", line).split()
            line_tokens = [token.lower() for token in line_tokens]

            tuples.append((play_name, line_tokens))

    with open("vocab.txt") as f:
        vocab = [line.strip() for line in f]

    with open("play_names.txt") as f:
        document_names = [line.strip() for line in f]

    return tuples, document_names, vocab

def read_in_snli(max_words):
    """Reads in the snli dataset and processes it into a list of lists of tokenized words.
       Generates a vocabulary consisting of the top max_words words and identity labels.

       Each tuple consists of
       tuple[0]: The sentence id
       tuple[1]: A tokenized line

       Returns:
       tuples: A list of tuples in the above format.
       vocab: A list of the top max_words words and identity labels.
    """
    # tokenize snli file
    tuples = []
    with open("snli.csv") as f:
        csv_reader = csv.reader(f, delimiter=",")
        for row in csv_reader:
            sentence_id = row[0]
            line = row[2]
            tokenized_line = re.sub(r"[^a-zA-Z0-9\s]", " ", line).split()
            tokenized_line = [token.lower() for token in tokenized_line]
            tuples.append((sentence_id, tokenized_line))

    with open("identity_labels.txt") as f:
        identity = [line.strip() for line in f]

    # get word counts
    word_counts = {}
    for tup in tuples:
        line = tup[1]
        for word in line:
            if word in word_counts:
                word_counts[word] += 1
            else:
                word_counts[word] = 1

    # get top 5000 words and identity labels
    vocab = sorted(word_counts, key=word_counts.get, reverse=True)
    vocab = vocab[:max_words]
    vocab = vocab + identity
    vocab = list(set(vocab))

    return tuples, vocab

def get_row_vector(matrix, row_id):
    """A convenience function to get a particular row vector from a numpy matrix

    Inputs:
      matrix: a 2-dimensional numpy array
      row_id: an integer row_index for the desired row vector

    Returns:
      1-dimensional numpy array of the row vector
    """
    return matrix[row_id, :]


def get_column_vector(matrix, col_id):
    """A convenience function to get a particular column vector from a numpy matrix

    Inputs:
      matrix: a 2-dimensional numpy array
      col_id: an integer col_index for the desired row vector

    Returns:
      1-dimensional numpy array of the column vector
    """
    return matrix[:, col_id]


def create_term_document_matrix(line_tuples, document_names, vocab):
    """Returns a numpy array containing the term document matrix for the input lines.

    Inputs:
      line_tuples: A list of tuples, containing the name of the document and
      a tokenized line from that document.
      document_names: A list of the document names
      vocab: A list of the tokens in the vocabulary

    Let m = len(vocab) and n = len(document_names).

    Returns:
      td_matrix: A mxn numpy array where the number of rows is the number of words
          and each column corresponds to a document. A_ij contains the
          frequency with which word i occurs in document j.
    """

    # initialize dict that stores word counts for each document
    table = {}
    for doc_name in document_names:
        next_doc = {}
        for word in vocab:
            # initialize word counts to 0
            next_doc[word] = 0
        table[doc_name] = next_doc

    # count the number of times each word appears for each document
    for line in line_tuples:
        doc_name = line[0]
        text = line[1]
        for word in text:
            if doc_name in table and word in table[doc_name]:
                table[doc_name][word] += 1

    # convert dict to numpy array
    vocab_size = len(vocab)
    corpus_size = len(document_names)
    td_matrix = np.empty((vocab_size, corpus_size), dtype='int')
    for i in range(0, vocab_size):
        word = vocab[i]
        for j in range(0, corpus_size):
            doc_name = document_names[j]
            td_matrix[i][j] = table[doc_name][word]

    return td_matrix


def create_term_context_matrix(line_tuples, vocab, context_window_size=1):
    """Returns a numpy array containing the term context matrix for the input lines.

    Inputs:
      line_tuples: A list of tuples, containing the name of the document and
      a tokenized line from that document.
      vocab: A list of the tokens in the vocabulary

    # NOTE: THIS DOCSTRING WAS UPDATED ON JAN 24, 12:39 PM.
    # NOTE: Words cannot co-occur with themselves, diagonal is zero

    Let n = len(vocab).

    Returns:
      tc_matrix: A nxn numpy array where A_ij contains the frequency with which
          word j was found within context_window_size to the left or right of
          word i in any sentence in the tuples.
    """
    # stores word co-occurences in table
    # stores indicies of each word
    table = {}
    vocabulary = {}
    vocab_size = len(vocab)
    for i in range(0, vocab_size):
        vocabulary[vocab[i]] = i

    # count number of times word appears within context_window_size words of the target word
    for line in line_tuples:
        text = line[1]
        for i in range(0, len(text)):
            target = text[i]
            if target not in vocabulary:
                # ignores words not specified in vocab
                continue
            if target not in table:
                table[target] = {}

            window = get_window(text, i, context_window_size)
            for context in window:
                if context not in vocabulary or context == target:
                    # ignores words not specified in vocab
                    # words also cannot co-occur with themselves
                    continue
                # count a co-occurrence
                if context not in table[target]:
                    table[target][context] = 1
                else:
                    table[target][context] += 1

    # convert dict to numpy array
    tc_matrix = np.zeros((vocab_size, vocab_size), dtype='int')
    for target in table:
        for context in table[target]:
            i = vocabulary[target]
            j = vocabulary[context]
            tc_matrix[i][j] = table[target][context]

    return tc_matrix

def get_window(line, index, window_size):
    """
    Given an array and an index, returns a subarray of all values
    that are +/- window_size away from that index
    """
    start = index - window_size
    end = index + window_size + 1
    if start < 0: start = 0
    if end > len(line): end = len(line)

    return line[start:end]

def create_tf_idf_matrix(term_document_matrix):
    """Given the term document matrix, output a tf-idf weighted version.

    See section 6.5 in the textbook.

    Hint: Use numpy matrix and vector operations to speed up implementation.

    Input:
      term_document_matrix: Numpy array where each column represents a document
      and each row, the frequency of a word in that document.

    Returns:
      A numpy array with the same dimension as term_document_matrix, where
      A_ij is weighted by the inverse document frequency of document h.
    """
    vocab_size = len(term_document_matrix)
    N = len(term_document_matrix[0])

    # calculate idf for each row
    idf = [0] * vocab_size
    for i in range(0, vocab_size):
        df = 0
        for j in range(0, N):
            if term_document_matrix[i][j] != 0:
                df += 1
        if df == 0:
            idf[i] = 0
        else:
            idf[i] = math.log((N / df), 10)

    # calculate tf-idf
    tf_idf = np.zeros((vocab_size, N))
    for i in range(0, vocab_size):
        for j in range(0, N):
            tf_idf[i][j] = math.log(term_document_matrix[i][j] + 1, 10) * idf[i]
    return tf_idf


def create_ppmi_matrix(term_context_matrix):
    """Given the term context matrix, output a PPMI weighted version.

    See section 6.6 in the textbook.

    Hint: Use numpy matrix and vector operations to speed up implementation.
    NOTE: Takes 4 minutes to complete for shakespeare corpus

    Input:
      term_context_matrix: Numpy array where each column represents a context word
      and each row, the frequency of a word that occurs with that context word.

    Returns:
      A numpy array with the same dimension as term_context_matrix, where
      A_ij is weighted by PPMI.
    """
    vocab_size = len(term_context_matrix)
    target_counts = np.squeeze(np.asarray(term_context_matrix.sum(axis=1))).tolist()
    context_counts = np.squeeze(np.asarray(term_context_matrix.sum(axis=0))).tolist()
    word_count = term_context_matrix.sum()

    target_prob = [x / word_count for x in target_counts]
    context_prob = [x / word_count for x in context_counts]

    ppmi = np.zeros((vocab_size, vocab_size))
    for i in range(0, vocab_size):
        for j in range(0, vocab_size):
            if term_context_matrix[i][j] == 0 or target_counts[i] == 0 or context_counts[j] == 0:
                ppmi[i][j] = 0
                continue

            p_i_j = term_context_matrix[i][j] / word_count
            p_i_star = target_prob[i]
            p_star_j = context_prob[j]
            pmi = p_i_j / (p_i_star * p_star_j)
            ppmi[i][j] = max(math.log(pmi, 2), 0)
    return ppmi

def compute_cosine_similarity(vector1, vector2):
    """Computes the cosine similarity of the two input vectors.

    Inputs:
      vector1: A nx1 numpy array
      vector2: A nx1 numpy array

    Returns:
      A scalar similarity value.
    """
    # Check for 0 vectors
    if not np.any(vector1) or not np.any(vector2):
        sim = 0

    else:
        sim = 1 - scipy.spatial.distance.cosine(vector1, vector2)

    return sim


def rank_words(target_word_index, matrix):
    """Ranks the similarity of all of the words to the target word using compute_cosine_similarity.

    Inputs:
      target_word_index: The index of the word we want to compare all others against.
      matrix: Numpy matrix where the ith row represents a vector embedding of the ith word.

    Returns:
      A length-n list of integer word indices, ordered by decreasing similarity to the
      target word indexed by word_index
      A length-n list of similarity scores, ordered by decreasing similarity to the
      target word indexed by word_index
    """
    target_vector = get_row_vector(matrix, target_word_index)
    similarity = {}
    for i in range(0, len(matrix)):
        if i == target_word_index:
            continue
        compare_vector = get_row_vector(matrix, i)
        similarity[i] = compute_cosine_similarity(target_vector, compare_vector)

    sorted_similarity = sorted(similarity.items(), key=lambda x: x[1], reverse=True)
    indicies = list(x[0] for x in sorted_similarity)
    values = list(x[1] for x in sorted_similarity)

    return indicies, values


if __name__ == "__main__":
    print("Reading in corpus...")
    tuples, document_names, vocab = read_in_shakespeare()

    snli_tuples, snli_vocab = read_in_snli(5000)

    print("Computing term document matrix...")
    td_matrix = create_term_document_matrix(tuples, document_names, vocab)

    print("Computing tf-idf matrix...")
    tf_idf_matrix = create_tf_idf_matrix(td_matrix)

    print("Computing term context matrix...")
    tc_matrix = create_term_context_matrix(tuples, vocab, context_window_size=4)

    print("Computing PPMI matrix...")
    print("WARNING: This takes approximately 4 minutes")
    ppmi_matrix = create_ppmi_matrix(tc_matrix)

    print("Computing term context matrix for SNLI...")
    snli_tc_matrix = create_term_context_matrix(snli_tuples, snli_vocab, context_window_size=4)

    print("Computing PPMI matrix for SNLI...")
    snli_ppmi_matrix = create_ppmi_matrix(snli_tc_matrix)

    # random_idx = random.randint(0, len(document_names) - 1)

    word = "jester"
    vocab_to_index = dict(zip(vocab, range(0, len(vocab))))

    snli_word = "lesbian"
    snli_vocab_to_index = dict(zip(snli_vocab, range(0, len(snli_vocab))))

    print(
        '\nThe 10 most similar words to "%s" using cosine-similarity on term-document frequency matrix are:'
        % (word)
    )
    ranks, scores = rank_words(vocab_to_index[word], td_matrix)
    for idx in range(0,10):
        word_id = ranks[idx]
        print("%d, %s, %s" %(idx+1, vocab[word_id], scores[idx]))

    print(
        '\nThe 10 most similar words to "%s" using cosine-similarity on term-context frequency matrix are:'
        % (word)
    )
    ranks, scores = rank_words(vocab_to_index[word], tc_matrix)
    for idx in range(0,10):
        word_id = ranks[idx]
        print("%d, %s, %s" %(idx+1, vocab[word_id], scores[idx]))

    print(
        '\nThe 10 most similar words to "%s" using cosine-similarity on tf-idf matrix are:'
        % (word)
    )
    ranks, scores = rank_words(vocab_to_index[word], tf_idf_matrix)
    for idx in range(0,10):
        word_id = ranks[idx]
        print("%d, %s, %s" %(idx+1, vocab[word_id], scores[idx]))

    print(
        '\nThe 10 most similar words to "%s" using cosine-similarity on PPMI matrix are:'
        % (word)
    )
    ranks, scores = rank_words(vocab_to_index[word], ppmi_matrix)
    for idx in range(0,10):
        word_id = ranks[idx]
        print("%d, %s, %s" %(idx+1, vocab[word_id], scores[idx]))

    print(
        '\nThe 10 most similar words to "%s" using cosine-similarity on term-context frequency matrix for SNLI dataset are:'
        % (snli_word)
    )
    ranks, scores = rank_words(snli_vocab_to_index[snli_word], snli_tc_matrix)
    for idx in range(0,10):
        word_id = ranks[idx]
        print("%d, %s, %s" %(idx+1, snli_vocab[word_id], scores[idx]))

    print(
        '\nThe 10 most similar words to "%s" using cosine-similarity on PPMI matrix for SNLI dataset are:'
        % (snli_word)
    )
    ranks, scores = rank_words(snli_vocab_to_index[snli_word], snli_ppmi_matrix)
    for idx in range(0,10):
        word_id = ranks[idx]
        print("%d, %s, %s" %(idx+1, snli_vocab[word_id], scores[idx]))
