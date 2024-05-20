Birju Patel
CS2731, Dr. Michael Miller-Yoder
Homework 2

How to Run:
The tool is built with python. I used version 3.9.5 in development.
To install dependencies, run the following command.
  - pip install -r requirements.txt
The tool comes preloaded with trained models located in the /models directory.
The test set can be tested out of the box.
You can also re-train a model on new training data. Doing so will overwrite the existing model.

I created a simple command line interface for the model. Type the following for usage information.
  - python driver.py -h
When you first download the file, you must download the required corpora.
  - python driver.py --action setup
Then, you can test a test set using the pre-trained models.
  - python driver.py --action test --model MODEL --file FILE
  - MODEL = 'lin_reg_unigram', 'lin_reg_custom', or 'neural_net'
  - FILE = path to file with test data
You may also re-train the model on training data.
  - python driver.py --action train --model MODEL --file politeness_data.csv
You can also run 5-fold cross validation.
  - python driver.py --action cross-validation --model MODEL --file politeness_data.csv

When the model is re-trained, it will also be tested and summary statistics will be given.
The summary statistics given are...
  - precision
  - recall
  - accuracy
  - f1-score
  - confusion matrix
  - sample of 5 false negatives
  - sample of 5 false positives

Additional Resources:
I consulted the documentation for the various libraries I used, such as numpy and sklearn.
I also referenced Stack Overflow.

Collaboration:
I did not collaborate with anyone on this project.

Generative AI:
I did not use any generative AI tools.

Unresolved Problems:
The neural network model takes a rather long time to train, and it is also not particularly accurate.
