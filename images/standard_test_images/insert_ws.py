# load base64 file and insert random number of spaces between characters

import sys
from random import randint, random, seed

def load(path):
    with open(path) as f:
        return f.read()


def insertspaces(data):

    result = ''

    for c in data:
        if random() > 0.8:
            k = randint(1, 33)
            result += ' ' * k

        result += c

    return result


def main():

    seed(0) # make the results predictable

    data = load(sys.argv[1])
    print insertspaces(data)

if __name__ == '__main__':
    main()
