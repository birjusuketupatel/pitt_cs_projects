from structs import Comment, Episode, Media
from models import *
import sqlite3
import random
import csv

DATABASE_CON = sqlite3.connect("../db/podcasts.db")
DATABASE_CURSOR = DATABASE_CON.cursor()

def media_table_contents():
    res = DATABASE_CURSOR.execute("SELECT * FROM Media")
    return res.fetchall()

def episode_table_contents(media_id):
    res = DATABASE_CURSOR.execute(f"SELECT * FROM Episode WHERE media_id='{media_id}'")
    return res.fetchall()

def comment_table_contents(media_id):
    res = DATABASE_CURSOR.execute(f"SELECT * FROM Comment WHERE media_id='{media_id}'")
    return res.fetchall()

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
            if p is None:
                continue

            # checks if error has occurred in calculating perplexity
            if p != -1:
                total_ppl += p
                count += 1
    return total_ppl / count

# media database -> data structures
media = [Media(id, name) for (id, name) in media_table_contents()]

for m in media:
    if episode_table_contents(m.media_id) is None or len(episode_table_contents(m.media_id)) == 0:
        DATABASE_CURSOR.execute(f"DELETE FROM Comment WHERE media_id='{m.media_id}'")
        media.remove(m)
    elif comment_table_contents(m.media_id) is None or len(comment_table_contents(m.media_id)) == 0:
        DATABASE_CURSOR.execute(f"DELETE FROM Episode WHERE media_id='{m.media_id}'")
        media.remove(m)
print(len(media))

for n in range(1, 4):
    with open(f"../project_data/shapiro/{n}_gram_perplexity.csv", "w+") as f:
        for m in media:
            writer = csv.writer(f)
            model = load_lm(f"../models/UCnQC_G5Xsjhp9fEJKuIcrSw_training.en_{n}_gram_smoothed.csv")
            ppl = round(avg_perplexity(f"../hw3_data/{m.media_id}_test.en", model), 3)
            print(f"Perplexity of The Ben Shapiro Show model with {m.media_title} test data: {ppl}")
            writer.writerow((m.media_title, m.media_title, ppl))