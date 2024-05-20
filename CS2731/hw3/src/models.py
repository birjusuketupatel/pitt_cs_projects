import os
import re
import sys
import nltk
import functools
import itertools
import math
import random

debug = True

def extract_features(fname, n):
    """Generates the following data from the training set.
    vocab: a closed vocabulary of characters from the language and special characters
    context: all possible n-1 length strings composed of characters from the vocab
    ngrams: the training text broken down as a sequence of n-tuples

    The following preprocessing rules are used to clean the data.
        - only letters, international characters ('í', 'ß', etc.), and the punctuation marks '¿?¡!.,' are preserved
        - all characters are converted to lowercase
        - '^' is added to the beginning of each line, "$" is added to the end
        - the 4 least frequently used characters are replaced with the unknown character '#'
    """
    # reads in raw data and filters out illegal characters
    # adds special characters to start and end of each line
    lines = []
    with open(fname, encoding='utf-8') as f:
        raw_data = f.readlines()
        for line in raw_data:
            line = re.sub('[^A-Za-zŽžÀ-ÿ¿\?¡!., ]+', '', line)
            line = '(' + line.lower() + ')'
            lines.append(line)

    # gets character counts
    char_counts = {}
    for line in lines:
        for c in line:
            if c not in char_counts:
                char_counts[c] = 1
            else:
                char_counts[c] += 1

    # finds 4 least frequently used characters
    unks = [k[0] for k in sorted(char_counts.items(), key=lambda x: x[1])]
    unks = unks[:4]
    unk_count = 0
    for unk in unks:
        unk_count += char_counts.pop(unk)
    char_counts['#'] = unk_count

    # converts text to a sequence of ngrams
    ngrams = []
    pattern = functools.reduce(lambda acc, c: acc + c, unks, '[') + ']'
    for line in lines:
        line = re.sub(pattern, '#', line)
        ngrams += list(nltk.ngrams([*line], n))

    # gets vocab
    vocab = list(char_counts.keys())

    # gets all possible sequences of n-1 characters in the vocab
    context = list(map(lambda x: ''.join(x), itertools.product(vocab, repeat=n-1)))

    return (vocab, context, ngrams)

def n_gram_lm(fname, n, smoothed=False):
    """Generates n-gram language model.
    Writes probabilities to a file 'models/{fname}_{n}_gram_{smoothed/unsmoothed}.csv'.
    Returns name of file with weights.
    """
    (vocab, context_list, ngrams) = extract_features(fname, n)

    ngram_counts = {}
    for ngram in ngrams:
        context = ''.join(ngram)[:n-1]
        word = ngram[n-1]
        if context not in ngram_counts:
            ngram_counts[context] = {word: 1}
        else:
            context_dict = ngram_counts[context]
            if word not in context_dict:
                context_dict[word] = 1
            else:
                context_dict[word] += 1
            ngram_counts[context] = context_dict

    for context in ngram_counts.keys():
        context_dict = ngram_counts[context]
        total_count = 0
        for word in context_dict.keys():
            total_count += context_dict[word]
        context_dict['total'] = total_count
        ngram_counts[context] = context_dict

    # construct probability matrix
    context_size = len(context_list)
    vocab_size = len(vocab)
    prob_matrix = []

    for i in range(0, context_size):
        context = context_list[i]
        next_row = []
        for j in range(vocab_size):
            word = vocab[j]
            prob = 0.0

            if smoothed:
                # applies laplace smoothing
                if context in ngram_counts and word in ngram_counts[context]:
                    prob = (ngram_counts[context][word] + 1) / (ngram_counts[context]['total'] + vocab_size)
                elif context in ngram_counts and word not in ngram_counts[context]:
                    prob = 1 / (ngram_counts[context]['total'] + vocab_size)
                else:
                    prob = 1 / vocab_size
            else:
                if context in ngram_counts and word in ngram_counts[context]:
                    prob = ngram_counts[context][word] / ngram_counts[context]['total']
                else:
                    prob = 0.0

            next_row.append(prob)
        prob_matrix.append(next_row)

    # write contents to file
    type = 'smoothed' if smoothed else 'unsmoothed'
    out_file = '../models/' + os.path.basename(fname) + "_" + str(n) + "_gram_" + type + ".csv"
    save_model(out_file, (n, smoothed, vocab, context_list, prob_matrix))

    return out_file

def save_model(fname, model):
    """Given a model represented as a tuple, saves that model to a file.
    n: n-gram size, integer
    smoothed: whether or not the model has been smoothed, boolean
    vocab: list of words, [char]
    context_list: list of contexts, [str]
    prob_matrix: probability of a word given a context, [[double]]
    """
    # unpacks model
    (n, smoothed, vocab, context_list, prob_matrix) = model

    type = 'smoothed' if smoothed else 'unsmoothed'
    with open(fname, 'w+', encoding='utf-8') as f:
        # writes model type (smooth vs unsmoothed) and n-gram number on first line
        f.write(type + ';' + str(n) + '\n')

        # writes vocab on second line
        f.write(';'.join(vocab) + '\n')

        # writes context probabilities
        for i in range(0, len(context_list)):
            line = context_list[i] + ';'
            line += ';'.join(['{:f}'.format(p) for p in prob_matrix[i]])
            f.write(line + '\n')

            # prints warning if sum of probability distribution does not equal 1
            total = sum(prob_matrix[i])
            if abs(total - 1) >= 0.03 and total != 0:
                print("WARNING: the probability distribution of '" + context_list[i] + "' sums to " + str(total) + ".")

def load_lm(fname):
    """Loads a trained n-gram language model from a file and returns the following values.
    n: n-gram size, integer
    smoothed: whether or not the model has been smoothed, boolean
    vocab: list of words, [char]
    context_list: list of contexts, [str]
    prob_matrix: probability of a word given a context, [[double]]
    """
    with open(fname, 'r', encoding='utf-8') as f:
        # first line: {smoothed/unsmoothed};{n-gram size}\n
        l = f.readline().rstrip('\n').split(';')
        smoothed = l[0] == 'smoothed'
        n = int(l[1])

        # second line: {word_0};{word_1};...;{word_n}
        vocab = f.readline().rstrip('\n').split(';')
        vocab_size = len(vocab)

        # subsequent lines: {context};{P(word_0 | context)};...;{P(word_n | context)}
        prob_matrix = []
        context_list = []
        while l := f.readline():
            row = l.rstrip('\n').split(';')
            context_list.append(row[0])
            probs = [float(p) for p in row[1:]]
            prob_matrix.append(probs)

            total = sum(probs)
            if abs(total - 1) >= 0.03 and total != 0:
                print("WARNING: the probability distribution of '" + row[0] + "' sums to " + str(total) + ".")

    return (n, smoothed, vocab, context_list, prob_matrix)

def interpolated_lm(fname, n, weights):
    """Generates an interpolated n-gram language model.
    Writes probabilities to a file 'models/{fname}_interpolated_{n}_gram_{smoothed/unsmoothed}.csv'.
    """
    if abs(sum(weights) - 1) >= 0.03:
        print("ERROR: sum of weights is not 1")
        return ""
    elif len(weights) != n:
        print("ERROR: number of weights inconsistent with n")
        return ""

    # train 1-gram, 2-gram, ... , n-gram language models
    model_files = []
    for i in range(1, n+1):
        model_files.append(n_gram_lm(fname, i))

    # load 1-gram, 2-gram, ... , n-gram language models
    models = []
    for model_file in model_files:
        models.append(load_lm(model_file))

    # hashes incidies of each model for faster lookup
    final_models = []
    for (n, smoothed, vocab, context_list, prob_matrix) in models:
        vocab_dict = {}
        context_dict = {}
        for i in range(0, len(vocab)):
            vocab_dict[vocab[i]] = i
        for i in range(0, len(context_list)):
            context_dict[context_list[i]] = i
        final_models.append((vocab_dict, context_dict, prob_matrix))

    # combines all models together
    final_context = models[-1][3]
    final_vocab = models[-1][2]
    final_matrix = []
    for i in range(0, len(final_context)):
        next_row = []
        for j in range(0, len(final_vocab)):
            context = final_context[i]
            vocab = final_vocab[j]
            final_prob = 0.0

            for k in range(0, len(final_models)):
                mod_context = context[(n-k-1):]
                (vocab_dict, context_dict, prob_matrix) = final_models[k]
                vocab_index = vocab_dict[vocab]
                context_index = context_dict[mod_context]
                final_prob += weights[k] * prob_matrix[context_index][vocab_index]
            next_row.append(final_prob)

        # normalizes probability distribution such that its sum is 1
        total = sum(next_row)
        next_row = [p / total for p in next_row]
        final_matrix.append(next_row)

    # write contents to file
    out_file = 'models/' + os.path.basename(fname) + "_" + str(n) + "_gram_interpolated.csv"
    save_model(out_file, (n, False, final_vocab, final_context, final_matrix))

    return out_file

def clean_text(raw_text, vocab):
    """Cleans raw text given a fixed vocabulary.
    Replaces all characters not in vocabulary with '#'.
    Adds '(' to start of each line and ')' at the end.
    Makes all letters lowercase.
    """
    final_text = ""
    lines = raw_text.split('\n')
    for line in lines:
        if len(line) == 0:
            # skip empty lines
            continue

        line = re.sub('[^A-Za-zŽžÀ-ÿ¿\?¡!., ]+', '', line)
        line = '(' + line.lower() + ')'
        final_text += line

    pattern = '[^' + ''.join(vocab) + ']'
    final_text = re.sub(pattern, '#', final_text)
    return final_text

def perplexity(text, model, n):
    """ Args:
            text: a string of characters
            model: a matrix or df of the probabilities with rows as prefixes, columns as suffixes.
			You can modify this depending on how you set up your model.
            n: n-gram order of the model

        Acknowledgment:
	  https://towardsdatascience.com/perplexity-intuition-and-derivation-105dd481c8f3
	  https://courses.cs.washington.edu/courses/csep517/18au/
	  ChatGPT with GPT-3.5
    """

    (n, smoothed, vocab, context_list, prob_matrix) = model

    text = clean_text(text, vocab)

    # hashes indices for faster lookup
    vocab_dict = {}
    context_dict = {}
    for i in range(0, len(vocab)):
        vocab_dict[vocab[i]] = i
    for i in range(0, len(context_list)):
        context_dict[context_list[i]] = i

    N = len(text)
    char_probs = []
    for i in range(n-1, N):
        prefix = text[i-n+1:i]
        suffix = text[i]

        row = context_dict[prefix]
        col = vocab_dict[suffix]

        prob = prob_matrix[row][col]

        # word appears in unseen context, P(text) = 0, Perplexity(text) = infinity
        if prob == 0:
            return -1

        char_probs.append(math.log2(prob))

    if N < n:
        return None

    neg_log_lik = -1 * sum(char_probs) # negative log-likelihood of the text
    ppl = 2 ** (neg_log_lik/(N - n + 1)) # 2 to the power of the negative log likelihood of the words divided by #ngrams
    return ppl

def generate_text(init_text, num_chars, model):
    (n, smoothed, vocab, context_list, prob_matrix) = model

    if n - 1 > len(init_text):
        print("ERROR: not enough initial information was given.")
        return init_text

    # hashes indices for faster lookup
    vocab_dict = {}
    context_dict = {}
    for i in range(0, len(vocab)):
        vocab_dict[vocab[i]] = i
    for i in range(0, len(context_list)):
        context_dict[context_list[i]] = i

    final_text = init_text
    while len(final_text) < num_chars - 1:
        # gets distribution of possible words given context
        context = final_text[(len(final_text)-n+1):]
        pdf = prob_matrix[context_dict[context]]

        # generates cummulative distribution function
        cdf = []
        total = 0
        for p in pdf:
            total += p
            cdf.append(total)

        # samples cdf
        rand = random.random()
        index = 0
        while index < len(cdf) and cdf[index] < rand:
            index += 1

        if index == len(cdf):
            # no character can be generated
            return final_text

        final_text += vocab[index]

    return final_text

def avg_perplexity(fname, model):
    """
    Calculates average perplexity for a line in the test file.
    """
    total_ppl = 0.0
    count = 0
    with open(fname, encoding='utf-8') as f:
        lines = f.readlines()
        n = model[0]
        for line in lines:
            p = perplexity(line, model, n)

            # checks if error has occurred in calculating perplexity
            if p != -1:
                total_ppl += p
                count += 1
    return total_ppl / count
