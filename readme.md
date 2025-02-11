
Requirements:
- Python3
- C++
- orTools for Python (install with "python -m pip install --upgrade --user ortools")


USAGE:

0/ Compilation
If you make changes in build_LP.cc (resp simu.cc), use "g++ -O3 build_LP.cc -o LP" (resp. "g++ -O3 simu.cc -o SIM"). This should not be necessary though

A/ Generate Workflow

Use python3 generate-jobs.py f n d m nb
- The first argument, f, is the name of the folder in which your generated files will be placed. This can be viewed as the name of the experiments you want to make
- n is the number of tasks of your workflow
- d is the number of dimensions of the tasks
- m is the number of precedence constraints
- nb is the number of workflows to generate

This will generate nb files in the folder f placed in files/tasks_parameters. Each file consists of n lines, each line represents one task and is in the form "name s_0 s_1 ... s_d a_1 ... a_d".
The time functions are the inverse of the speedup functions that presented in the IPDPS 18' paper (Scheduling Parallel Tasks under Multiple Resources: List Scheduling vs. Pack Scheduling). For instance, for the Extended Amdahl's law (1), we have t(p)=s_0+sum_{i=1}^n s_i/p_i . (Therefore the definitions diverges slightly from IPDPS 18' but I felt it was easier)

This will also generate nb files in the folder f placed in files/precedence_constraints/, whose name matches the ones in tasks_parameters. Each file consists of the precedence constraints of the workload, represented by m lines of the form "a b", meaning that the task a must be processed before the task b. Here the tasks are not labeled by their names, but by their number. For instance 0 corresponds to the task represented by the first line of the corresponding file in task_parameters.

Finally, it will create an empty folder f in files/allocation.

You will probably not use this code for the experiments, but this is the required structure so that the simulator work.

Example: you can try "python3 generate-jobs.py test 8 3 10 6" and check the generated files. It will create 8 samples in files/tasks_parameters/test/ with n=8 lines with 7 parameters, because d=3. It will also create 8 samples in files/precedence_constraints/test/ with m=10 lines corresponding to 10 random precedence constraints, and it will generate the folder files/allocation/test/


B/ Run experiments
Use python3 master.py f d p_1 ... p_d m
- f is the name of the folder for which you want to run the simulator. It will try all the files placed in that folder
- d is the number of dimensions of the tasks in your workflows be careful, it must match your generation!
- p_i is the number of resources of the i-st dimension
- m is the speedupmodel which can take the four possible values
	* amdSum, which corresponds to the Extended Amdahl Law(i) (See IPDPS 18)
	* amdMax, which corresponds to the Extended Amdahl Law(iii)
	* powSum, which corresponds to the Extended power Law(i)
	* powMax, which corresponds to the Extended power Law(iii)

This will generate a file in the result folder, named f_p1_p2_p3_m.txt which contain as much lines as there are files in the folder ; each lines corresponding to one sample and is in the form "name res mT mA LB HB",  where "res" is the value of our alogrithm, mT is the value of minTime algorithm, mA is the value of minArea algorithm, LB is a lower bound on the optimal schedule, HB should be a higher bound in the result, computed as LB*r(d), where r(d) is the ratio given in ICPP paper in theorem 1 (section 4.3).
If the ratio is not in [LB,HB], please write me an email.
This also sums up the results in the command line. You can remove the cout at the very end of simu.cc if you want.

Example: use "python3 master.py test 3 10 10 10 amdSum" to obtain the results of the algorithm on your generated workflow in step A, that will be writen in the file test_10_10_10_amdSum.

Note: This will also back up the allocation files in files/allocation/f/ so you can always check them later if you prefer.

C/ Additionnal parameters

Open master.py, you'll see at the begining 4 parameters that you can change if you want.
- The first one is "priority". This is a setting for the list scheduling: all jobs are sorted in the queue of all jobs that are ready. The current priority functions are:
	* length: longest tasks first.
	* rlength: shortest tasks first
	* area: largest area first
	* rarea: smallest area first
	* procs: largest number of resources first
	* rprocs: smallest number of resources first
	* rand: random
All the implemented priority functions are based on local parameters, but we could think of priority based on their released date or by the length of the longest remaining path from the task. Tell me if needed
- alpha and beta are some speedup parameters. We may lose an extra factor (1+alpha)(1+beta) in ratio, but the higher alpha and beta, the faster the algorithm will be. Set to 0, this corresponds to the algorithm descibed in the ICPP paper. However if for instance d=4 and p_i=1000, the number of possible allocation for any single task is 10^12!!! Which is way to much to consider
In a nutshell, a higher alpha discard more possible allocations (for instance with alpha=1, we only try the power of twos for the p_i, which in the worst case make us lose a factor 2 on the optimal time because the speedup is not superlinear). beta discards more dominating allocations in later steps of the algorithm: if an allocation is dominating and has time t, we discard all dominating allocations with time in [t/(1-beta);t], to reduce significantly the number of constraints for the Linear Solver. You might not need to use beta>0 but you should play with these to check which parameter is more efficient for your instances !
- debug is useful to see what's happening. Set to 0, only the results will be printed. Set to 1, additional infos are given, set to 2 all the infos will be given.

Example: try "python3 master.py simple 2 4 4 amdSum" while changing debug to 1 or 2.

Notes: -If you try this, the Phase A correspond to a very tricky yet beautiful algorithm to compute an approximation on the optimal Time/Cost tradeof. Some infos and an example are given in my internship report (Section 4.1), but the writing is poor. If you need more infos on how this work, tell me. I could write down an example sometimes.
- This is impractical if d or p_i or n is large, at least with debug="2"


