from models import *

def init():
    """Builds every possible language model.
    Stores writes probabilities to file in '/models'.
    Creates smoothed and unsmoothed 1-gram, 2-gram, and 3-gram for each language.
    Builds 2-gram and 3-gram interpolated models for each language.
    """
    # models with laplace smoothing
    n_gram_lm('hw3_data/training.en', 1, smoothed=True)
    n_gram_lm('hw3_data/training.en', 2, smoothed=True)
    n_gram_lm('hw3_data/training.en', 3, smoothed=True)
    n_gram_lm('hw3_data/training.es', 1, smoothed=True)
    n_gram_lm('hw3_data/training.es', 2, smoothed=True)
    n_gram_lm('hw3_data/training.es', 3, smoothed=True)
    n_gram_lm('hw3_data/training.de', 1, smoothed=True)
    n_gram_lm('hw3_data/training.de', 2, smoothed=True)
    n_gram_lm('hw3_data/training.de', 3, smoothed=True)

    # interpolated n-gram language model automatically builds models for n-1, n-2, ..., 1
    interpolated_lm('hw3_data/training.en', 2, [0.5, 0.5])
    interpolated_lm('hw3_data/training.en', 3, [0.333, 0.333, 0.333])
    interpolated_lm('hw3_data/training.es', 2, [0.5, 0.5])
    interpolated_lm('hw3_data/training.es', 3, [0.333, 0.333, 0.333])
    interpolated_lm('hw3_data/training.de', 2, [0.5, 0.5])
    interpolated_lm('hw3_data/training.de', 3, [0.333, 0.333, 0.333])

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

if __name__ == '__main__':
    # builds all models
    print("building language models...")
    init()

    # calculates average perplexity for each line in the test set
    en_inter_3 = load_lm('models/training.en_3_gram_interpolated.csv')
    es_inter_3 = load_lm('models/training.es_3_gram_interpolated.csv')
    de_inter_3 = load_lm('models/training.de_3_gram_interpolated.csv')

    en_ppl = avg_perplexity('hw3_data/test', en_inter_3)
    es_ppl = avg_perplexity('hw3_data/test', es_inter_3)
    de_ppl = avg_perplexity('hw3_data/test', de_inter_3)

    print("\naverage perplexity per line, interpolated trigram model:")
    print("english: " + str(en_ppl))
    print("spanish: " + str(es_ppl))
    print("german: " + str(de_ppl))

    en_smooth_3 = load_lm('models/training.en_3_gram_smoothed.csv')
    es_smooth_3 = load_lm('models/training.es_3_gram_smoothed.csv')
    de_smooth_3 = load_lm('models/training.de_3_gram_smoothed.csv')

    en_ppl = avg_perplexity('hw3_data/test', en_smooth_3)
    es_ppl = avg_perplexity('hw3_data/test', es_smooth_3)
    de_ppl = avg_perplexity('hw3_data/test', de_smooth_3)

    print("\naverage perplexity per line, trigram model with laplace smoothing:")
    print("english: " + str(en_ppl))
    print("spanish: " + str(es_ppl))
    print("german: " + str(de_ppl))

    # generate english sentences using smoothed and unsmoothed bigram and trigram model
    en_unsmooth_3 = load_lm('models/training.en_3_gram_unsmoothed.csv')
    en_unsmooth_2 = load_lm('models/training.en_2_gram_unsmoothed.csv')
    en_smooth_2 = load_lm('models/training.en_2_gram_smoothed.csv')

    print("\nsentence generation, unsmoothed bigram model:")
    f = lambda c: "'" + c + "' - " + generate_text(c, 100, en_unsmooth_2)
    prefixes = ['(', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i']
    for prefix in prefixes:
        print(f(prefix))

    print("\n sentence generation, smoothed bigram model:")
    f = lambda c: "'" + c + "' - " + generate_text(c, 100, en_smooth_2)
    for prefix in prefixes:
        print(f(prefix))

    print("\n sentence generation, unsmoothed trigram model:")
    f = lambda c: "'" + c + "' - " + generate_text(c, 100, en_unsmooth_3)
    prefixes = ['(t', 'at', 'be', 'ch', 'dr', 'ea', 'fr', 'ga', 'ho', 'ie']
    for prefix in prefixes:
        print(f(prefix))

    print("\n sentence generation, smoothed trigram model:")
    f = lambda c: "'" + c + "' - " + generate_text(c, 100, en_smooth_3)
    for prefix in prefixes:
        print(f(prefix))

    # compare perplexity across smoothed and unsmoothed unigram, bigram, and trigram english model
    en_unsmooth_1 = load_lm('models/training.en_1_gram_unsmoothed.csv')
    en_ppl_1 = avg_perplexity('hw3_data/test', en_unsmooth_1)
    en_ppl_2 = avg_perplexity('hw3_data/test', en_unsmooth_2)
    en_ppl_3 = avg_perplexity('hw3_data/test', en_unsmooth_3)

    print("\naverage perplexity per line, english models")
    print("unsmoothed unigram: " + str(en_ppl_1))
    print("unsmoothed bigram: " + str(en_ppl_2))
    print("unsmoothed trigram: " + str(en_ppl_3))

    en_smooth_1 = load_lm('models/training.en_1_gram_smoothed.csv')
    en_ppl_1 = avg_perplexity('hw3_data/test', en_smooth_1)
    en_ppl_2 = avg_perplexity('hw3_data/test', en_smooth_2)
    en_ppl_3 = avg_perplexity('hw3_data/test', en_smooth_3)

    print("smoothed unigram: " + str(en_ppl_1))
    print("smoothed bigram: " + str(en_ppl_2))
    print("smoothed trigram: " + str(en_ppl_3))
