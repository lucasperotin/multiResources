import sys
import random
import os

def generate_mold(foldername,n,m,nb_iter):    
    os.system("mkdir files/precedence_constraints/"+foldername)
    for j in range(nb_iter):
    	f = open(os.getcwd()+"/files/precedence_constraints/"+foldername+"/sample"+(str)(j)+".txt",'w')
    	Done=[[0 for i in range(n)] for j in range(n)]
    	for i in range(n):
    		Done[i][i]=1
    	i=0
    	while i<m:
    		s0=random.randint(0,n-1)
    		s1=random.randint(0,n-1)
    		if (s0<s1):
    			s0,s1=s1,s0
    		if (Done[s0][s1]==0):
    			Done[s0][s1]=1
    			name = str(s0)+" "+str(s1)
    			f.write(name+"\n")
    			i+=1
    	f.close()

generate_mold(sys.argv[1],(int) (sys.argv[2]), (int) (sys.argv[3]),(int) (sys.argv[4]))

