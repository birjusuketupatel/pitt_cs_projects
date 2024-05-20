import os
import random
from random import choices
import pandas as pd
from datetime import datetime

def get_street(row):
    # removes house number from address
    tokens = row['RESIDENTIAL_ADDRESS1'].split()
    tokens = tokens[1:]
    street = ' '.join(tokens)
    street = street + ", " + row['RESIDENTIAL_CITY']
    return street

def get_age_range(row):
    # age ranges: 0-24, 25-44, 45-64, 65+
    bday = datetime.strptime(row['DATE_OF_BIRTH'], '%Y-%m-%d')
    days_in_year = 365.2425
    age = int((datetime.today() - bday).days / days_in_year)

    if 0 <= age and age <= 24:
        return '0-24'
    elif 25 <= age and age <= 44:
        return '24-44'
    elif 45 <= age and age <= 64:
        return '45-64'
    else:
        return '65+'

def sample_party(probs):
    r = random.random()
    p = 0
    for key in probs.keys():
        p += probs[key]
        if p >= r:
            return key
    return key

def generate_report(df, k, l, e, table_name):
    print('\noriginal dataset\n')
    print(df.head(5))

    # apply groupings
    df = df.fillna('None')
    df['STREET'] = df.apply(get_street, axis=1)
    df['AGE_RANGE'] = df.apply(get_age_range, axis=1)

    # k-anonymity, where the anonymity group is the street the voter lives on
    # groups by street, removes streets with fewer than k people
    df_k_anonymous = df[['STREET', 'AGE_RANGE', 'PARTY_AFFILIATION']]
    df_k_anonymous = df_k_anonymous.dropna()
    df_k_anonymous = df_k_anonymous.groupby(['AGE_RANGE', 'STREET']).filter(lambda group: len(group) >= k)
    df_k_anonymous = df_k_anonymous.reset_index().drop('index', axis=1)

    print('\nk-anonymized dataset, k=' + str(k) + '\n')
    print(df_k_anonymous.head(5))

    # save to file
    fname = 'data/' + table_name + '_K_ANON.csv'
    df_k_anonymous.to_csv(fname, sep=',', encoding='utf-8')

    # show streets with only one party
    group_counts = df_k_anonymous.groupby(['STREET', 'AGE_RANGE', 'PARTY_AFFILIATION']).size().reset_index(name='count')
    count_of_counts = group_counts.groupby(['STREET', 'AGE_RANGE']).size().reset_index(name='count')
    one_party_groups = count_of_counts.loc[count_of_counts['count'] == 1]
    one_party_groups = one_party_groups.merge(df_k_anonymous, how='left').drop_duplicates()
    one_party_groups = one_party_groups.drop('count', axis=1)
    one_party_groups = one_party_groups.loc[one_party_groups['PARTY_AFFILIATION'] != 'None']

    print('\ndataset is not l-diverse')
    print('the streets below have only residents from one political party\n')
    print(one_party_groups.head(5))

    # make dataset l-diverse, group by age range, city
    # removes groups where group size < k and if group is not l diverse
    df_l_diverse = df[['RESIDENTIAL_CITY', 'AGE_RANGE', 'PARTY_AFFILIATION']]
    df_l_diverse = df_l_diverse.dropna()
    df_l_diverse = df_l_diverse.groupby(['RESIDENTIAL_CITY', 'AGE_RANGE']).filter(lambda group: len(group) >= k)
    df_l_diverse = df_l_diverse.reset_index().drop('index', axis=1)

    # remove non-diverse groups
    group_counts = df_l_diverse.groupby(['RESIDENTIAL_CITY', 'AGE_RANGE', 'PARTY_AFFILIATION']).size().reset_index(name='count')
    count_of_counts = group_counts.groupby(['RESIDENTIAL_CITY', 'AGE_RANGE']).size().reset_index(name='count')
    not_diverse = count_of_counts.loc[count_of_counts['count'] < l].drop('count', axis=1)

    df_l_diverse = pd.merge(df_l_diverse, not_diverse, on=['RESIDENTIAL_CITY', 'AGE_RANGE'], how="outer", indicator=True)
    df_l_diverse = df_l_diverse.loc[df_l_diverse['_merge'] == 'left_only'].drop('_merge', axis=1)
    df_l_diverse = df_l_diverse.reset_index().drop('index', axis=1)

    print('\ndataset is now l-diverse, l=' + str(l))
    print('accomplished by grouping on city instead of street\n')
    print(df_l_diverse.head(5))

    # save to file
    fname = 'data/' + table_name + '_L_DIVERSE.csv'
    df_l_diverse.to_csv(fname, sep=',', encoding='utf-8')

    # calculate probability of being in a particular party
    party_prob = {}
    n = 0.0
    for index, row in df.iterrows():
        if row['PARTY_AFFILIATION'] not in party_prob:
            party_prob[row['PARTY_AFFILIATION']] = 1.0
        else:
            party_prob[row['PARTY_AFFILIATION']] += 1.0
        n += 1.0
    for key in party_prob.keys():
        party_prob[key] /= n

    # calculate counts for number of party members on each street
    # make differentially private with laplace sampling
    street_counts = {}
    next_street = {}
    for index, row in df.iterrows():
        street = row['STREET']
        party = ''

        if street not in street_counts:
            next_street = {}
        else:
            next_street = street_counts[street]

        if random.random() < e:
            # report party honestly
            party = row['PARTY_AFFILIATION']
        else:
            # report party randomly
            party = sample_party(party_prob)

        if party not in next_street:
            next_street[party] = 1
        else:
            next_street[party] += 1

        street_counts[street] = next_street

    df_differential = pd.DataFrame({'STREET': [], 'PARTY': [], 'COUNT': []})
    for street in street_counts.keys():
        for party in street_counts[street].keys():
            new_row = {'STREET': street, 'PARTY': party, 'COUNT': street_counts[street][party]}
            df_differential.loc[len(df_differential)] = new_row
    df_differential = df_differential.reset_index(drop=True)

    print('\ndataset is now differentially private, e=' + str(e))
    print(df_differential.head(10))

    # save to file
    fname = 'data/' + table_name + '_DIFFERENTIAL.csv'
    df_differential.to_csv(fname, sep=',', encoding='utf-8')


def main():
    pd.set_option('display.max_rows', None)

    vinton_description = """
    Location: Vinton County, Ohio
    Description: A rural county in central Ohio. About an hour and a half south of Columbus.
    Population: 12,565
    Area: 415 square miles
    Denisty: 30 people per square mile
    2022 Election Results: 77.81% for DeWine (R), 20.88% for Whaley (D), 1.31% for other
    2020 Election Results: 76.71% for Trump (R), 22.04% for Biden (D), 1.25% for other"""
    print(vinton_description)
    vinton = pd.read_csv('data/VINTON.txt', dtype=str)
    generate_report(vinton, k=5, l=2, e=0.9, table_name='VINTON')

    athens_description = """
    Location: Athens County, Ohio
    Description: A rural county in Ohio on the Kentucky border. The home of the Ohio University.
    Population: 58,979
    Area: 508 square miles
    Denisty: 116 people per square mile
    2022 Election Results: 46.63% for DeWine (R), 53.08% for Whaley (D), 0.29% for other
    2020 Election Results: 41.58% for Trump (R), 56.55% for Biden (D), 1.87% for other"""
    print(athens_description)
    athens = pd.read_csv('data/ATHENS.txt', dtype=str)
    generate_report(athens, k=5, l=2, e=0.9, table_name='ATHENS')

if __name__ == '__main__':
    main()
