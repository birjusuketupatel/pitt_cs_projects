Homework 1 - Birju Patel

Run Instructions:
The code is written in python and run via the command line.
Step 1: Place the following files in the same directory
- hw1_skeleton_bsp22.py
- shakespeare_plays.csv
- vocab.txt
- play_names.txt
- snli.csv
- identity_labels.txt
Step 2: Navigate to that directory in the command line
Step 3: Run the following command, "python3 hw1_skeleton_bsp22.py"
Step 4: Wait approximately 5 minutes for the results.

Dependencies:
I did not add any packages beyond what was already specified by the skeleton code.
The skeleton code required the installation of numpy and scipy.
I tested my implementation using Python 3.10.4.
I ran the code on my personal laptop, which has 8GB of RAM.

Additional Resources/Collaboration:
I consulted Stack Overflow and the online documentation for numpy while doing the project.
I did not collaborate with or talk about this project with any of my peers, nor did I use any AI tools.

Notes/Unresolved Issues:
I assumed that a word cannot co-occur with itself, so the diagonal of the term-context matrix is all zeros.
Calculating the PPMI matrix for the Shakespeare corpus takes approximately 4 minutes. This takes the bulk of the program's runtime.
Please comment out this section when grading if it becomes an issue.
When calculating the cosine similarity of words from the SNLI corpus using the term-context matrix for the SNLI corpus,
a warning pops up saying that an error has occurred while calculating the distance between two vectors.
However, the warning does not affect the final result.