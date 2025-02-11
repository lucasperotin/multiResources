from ortools.linear_solver import pywraplp
from ortools.init import pywrapinit



def main():
    MAXI=1000000
    file=open('tempForPython.txt','r')
    lines=file.readlines()
    file.close()
    t = lines[0].split(" ")
    
    cmp=0
    name=t[cmp]
    cmp+=1
    debuga=(int) (t[cmp])
    if(debuga>0):
        print("\n\n***********PHASE A': SOLVING THE LINEAR PROGRAM**************\n\n")
    cmp+=1
    n=(int) (t[cmp])
    cmp+=1
    m=(int) (t[cmp])
    cmp+=1
    d=(int) (t[cmp])
    cmp+=1
    rho=(float) (t[cmp])
    cmp+=1
    Names=["" for i in range(n)]
    for i in range(n):
        Names[i]=t[cmp]
        cmp+=1
    Precs=[[0 for i in range(2)] for j in range(m)]
    T=[[[] for i in range(2)] for j in range(n)]
    Alloc=[[[] for i in range(d)] for j in range(n)]
    for i in range(m):
        Precs[i][0]=(int) (t[cmp])
        cmp+=1
        Precs[i][1]=(int) (t[cmp])
        cmp+=1
    
    
    for i in range(n):
        cardi=(int) (t[cmp])
        cmp+=1
        T[i][0]=[0 for j in range(cardi)]
        T[i][1]=[0 for j in range(cardi)]
        for b in range(d):
            Alloc[i][b]=[0 for j in range(cardi)]
        for j in range(cardi):
            T[i][0][j]=(float) (t[cmp])
            cmp+=1
            T[i][1][j]=(float) (t[cmp])
            cmp+=1
            for b in range(d):
                Alloc[i][b][j]=(int) (t[cmp])
                cmp+=1
        
    # Create the linear solver with the GLOP backend.
    solver = pywraplp.Solver.CreateSolver('GLOP')

#   // Create variables.
    Real=[]
    Dates=[]
    Virt=[[] for i in range(n)]
    L=solver.NumVar(0, solver.infinity(), "L")
    W=solver.NumVar(0, solver.infinity(), "W")
    C=solver.NumVar(0, solver.infinity(), "C")
    
    for i in range(n):
        Real.append(solver.NumVar(0.0,T[i][0][0],"Real"+(str) (i)))
        Dates.append(solver.NumVar(0.0,solver.infinity(),"Dates"+(str) (i)))
        for j in range(len(T[i][1])-1):
            Virt[i].append(solver.NumVar(0,T[i][0][j],"Virt"+(str)(i)+(str)(j)))
        Virt[i].append(solver.NumVar(T[i][0][len(T[i][1])-1],T[i][0][len(T[i][1])-1],"Virt"+(str)(i)+(str)(len(T[i][1])-1)))

    if(debuga>0):
        print('Number of variables =', solver.NumVariables())

    # Create Constraints
    
    VirtReal=[[] for i in range(n)]
    
    for i in range(n):
        for j in range(len(Virt[i])):
            VirtReal[i].append(solver.Constraint(0.0,solver.infinity(),"VirtReal"+(str)(i)+(str)(j)))
            VirtReal[i][j].SetCoefficient(Real[i],1)
            VirtReal[i][j].SetCoefficient(Virt[i][j],-1)
            
    RealDates=[]
    for i in range(n):
        RealDates.append(solver.Constraint(0.0,solver.infinity(),"RealDates"+(str)(i)))
        RealDates[i].SetCoefficient(Dates[i],1)
        RealDates[i].SetCoefficient(Real[i],-1)
        
    for i in range(m):
        RealDates.append(solver.Constraint(0.0,solver.infinity(),"RealDates"+(str)(i+n)))
        RealDates[i+n].SetCoefficient(Dates[Precs[i][1]],1)
        RealDates[i+n].SetCoefficient(Dates[Precs[i][0]],-1)
        RealDates[i+n].SetCoefficient(Real[Precs[i][1]],-1)
    
    DatesL=[]
    for i in range(n):
        DatesL.append(solver.Constraint(0.0,solver.infinity(),"DatesL"+(str)(i+n)))
        DatesL[i].SetCoefficient(L,1)
        DatesL[i].SetCoefficient(Dates[i],-1)
    
    maxArea=0.0
    for i in range(n):
        maxArea+=T[i][0][len(T[i][0])-1]
    
    Wcompt=solver.Constraint(maxArea,solver.infinity(),"Wcompt")
    Wcompt.SetCoefficient(W,1)
    for i in range(n):
        for j in range(len(Virt[i])-1):
            Wcompt.SetCoefficient(Virt[i][j],T[i][1][j]/T[i][0][j])
           
    LC=solver.Constraint(0.0, solver.infinity(),"LC")
    LC.SetCoefficient(C,1)
    LC.SetCoefficient(L,-1)
    
    
    WC=solver.Constraint(0.0, solver.infinity(),"WC")
    WC.SetCoefficient(C,1)
    WC.SetCoefficient(W,-1)
    if(debuga>0):
        print('Number of constraints =', solver.NumConstraints())

    #SOLVE
    objective=solver.Objective()
    objective.SetCoefficient(C,1)
    objective.SetMinimization()
    solver.Solve()


#   // Print results
    if (debuga>3):
        print('Solution:')
        print('Objective value =', objective.Value())
        if (debuga>1):
            print('L =', L.solution_value())
            print('W =', W.solution_value())
        
            for i in range(n):
                for j in range(len(Virt[i])):
                    print("Virt"+(str)(i)+(str)(j)+" = ",Virt[i][j].solution_value())
                    
            for i in range(n):
                print("Real"+(str)(i)+" = ",Real[i].solution_value())
                print("Dates"+(str)(i)+" = ",Real[i].solution_value())
  
#     // Round

    ChoiceVirt=[[] for i in range(n)]
    Choice=[0 for i in range(n)]
    
    for i in range(n):
        for j in range(len(Virt[i])):
            temp=Virt[i][j].solution_value()
            if (temp/T[i][1][j]<rho):
                ChoiceVirt[i].append(0)
            else:
                ChoiceVirt[i].append(1)
        ChoiceVirt[i].append(1)
 

    #Rebuild allocation
    file2=open('./'+name,'w')
    file2.write((str)(objective.Value())+'\n')
    if(debuga>0):
        print("Final allocation (not reduced):")
    for i in range(n):
        tpch=0
        j=0
        while(ChoiceVirt[i][j]==0):
            tpch+=1
            j+=1
        Choice[i]=tpch
        if(debuga>0):
            print("-",Names[i],Alloc[i][0][Choice[i]],Alloc[i][1][Choice[i]])
        file2.write(Names[i]+' ')
        for b in range(d):
            file2.write((str)(Alloc[i][b][Choice[i]])+' ')
        file2.write('\n')
        

  
if __name__ == '__main__':
    pywrapinit.CppBridge.InitLogging('basic_example.py')
    cpp_flags = pywrapinit.CppFlags()
    cpp_flags.logtostderr = True
    cpp_flags.log_prefix = False
    pywrapinit.CppBridge.SetFlags(cpp_flags)

    main()




    
  
  

