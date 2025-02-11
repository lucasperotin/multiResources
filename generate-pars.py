import sys
import random
import os

def generate_mold(foldername,n,d,niter): 
    os.system("mkdir files/tasks_parameters/"+foldername)
    for j in range (niter):
        f = open(os.getcwd()+"/files/tasks_parameters/"+foldername+"/sample"+(str)(j)+".txt",'w')
        for i in range(n):
            s0=random.uniform(0,0.2)
            s=[0 for j in range(d)]
            a=[0 for j in range(d)]
            tot=0
            for j in range(d):
                s[j]=random.uniform(0,1)
                a[j]=random.uniform(0.3,1)
                tot+=s[j]
            norm=1-s0
            work=random.uniform(0,1)    #######CAREFUL : THIS IS NOT REALLY THE WORK (I.E. TIME WITH ONE PROCESSORS OF EACH TIME) FOR MAX##############
            s0=s0*work
            stro=(str) (s0)+" "
            for j in range(d):
                s[j]=s[j]*norm/tot*work
                stro+=(str) (s[j])+" "
            for j in range(d):
                stro+=(str) (a[j])+" "
            name = "Task"+str(i)+" "
            f.write(name+stro+"\n")
        f.close()

generate_mold(sys.argv[1],(int) (sys.argv[2]), (int) (sys.argv[3]),(int) (sys.argv[4]))

