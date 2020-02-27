import ROOT as R

from dataclasses import dataclass
import os
from random import randint

R.gSystem.Load(os.path.join(os.getenv('IBDSEL_HOME'),
                            'selector/_build/common.so'))

@dataclass
class Params:
    size: int
    insertions: int
    insert_depth: int

# This makes assumptions on the __str__() that cppyy generates for us
def listify(rb: R.RingBuf):
    string = str(rb)[2:][:-2]   # remove braces
    return [int(_) for _ in string.split(',')]

def test(pars: Params, debug=False, metatest=False):
    assert(pars.insert_depth < pars.size)

    fake = []
    real = R.RingBuf(int)(pars.size)

    for i in range(pars.insertions):
        fake.insert(0, i)
        real.put(i)

    fake.insert(pars.insert_depth, -1)
    real.insert(pars.insert_depth, -1)

    fake = fake[:pars.size]
    real = listify(real)

    if metatest and not randint(0, 50):
        maxdepth = min(pars.insertions, pars.size-1)
        fake[randint(0, maxdepth-1)] += 1

    msg = f'{pars}:\nExpected {fake}\n     Got {real}'

    if fake != real:
        print(msg)
        raise Exception("You done goofed")

    if debug:
        print(msg)

def stress(size=10, max_inserts=None, metatest=False):
    if not max_inserts:
        max_inserts = 3 * size

    n = 0
    for insertions in range(max_inserts):
        for insert_depth in range(0, min(insertions, size)):
            test(Params(size, insertions, insert_depth), metatest=metatest)
            n += 1

    print(f'{n} passed')
