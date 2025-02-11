import sys
import os
import glob


#### HERE ARE SOME ADDITIONAL PARAMETERS ####
priority="length"
alpha="1"
beta="0"
debuga="0"
rho="0"  #use 0 for "normal" settings, otherwise you should use it between 0.1 and 0.9
mu="0" #use 0 for "normal" settings, otherwise you should use it between 0.1 and 0.4
#############################################


argsys=1
foldername=sys.argv[argsys]
argsys+=1
d=(int) (sys.argv[argsys])
argsys+=1
p=[]
for i in range(d):
    p.append(sys.argv[argsys])
    argsys+=1
time_fun=sys.argv[argsys]

outname=foldername+'_'
for i in range(d):
    outname+=p[i]+'_'
outname+=time_fun+'.txt'
outfile="files/results/"+outname
genfiles=glob.glob("files/tasks_parameters/"+foldername+"/*")
outshort=[]





for i in range(len(genfiles)):
    genfiles[i]=genfiles[i][23:]
    
for genfile in genfiles:
    filetask="files/tasks_parameters/"+genfile
    fileprec="files/precedence_constraints/"+genfile
    filealloc="files/allocation/"+genfile
    os.system("touch tempForPython.txt")
    os.system("touch args.txt")
    args=filetask+' '+fileprec+' '+filealloc+' '+time_fun+' '+rho+' '+mu+' '+(str)(d)+' '
    for b in range(d):
        args+=p[b]+' '
    args+=priority+' '+debuga+' '+alpha+' '+beta+' '+outfile
    myfile=open("args.txt",'w')
    myfile.write(args)
    myfile.close()
    os.system("./LP")
    os.system("python3 getAlloc.py")
    os.system("./SIM")
    os.system("rm tempForPython.txt")
    os.system("rm args.txt")
