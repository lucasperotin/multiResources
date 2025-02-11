import sys
import os

name=sys.argv[1]
n=sys.argv[2]
d=sys.argv[3]
m=sys.argv[4]
iters=sys.argv[5]
os.system("python3 generate-pars.py "+name+' '+n+' '+d+' '+iters)
os.system("python3 generate-precs.py "+name+' '+n+' '+m+' '+iters)
os.system("mkdir files/allocation/"+name)