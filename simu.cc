#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>
#include <math.h>
#include <queue>
#include <time.h>
#include <sys/time.h>
#include <fstream>
#include <string>
#include <limits>
#include <stdlib.h>     /* srand, rand */
#include <ctime>
#include <fstream>

#include <omp.h>

#define DEBUG(X) cerr << X << "\n";
using namespace std;

template<typename T> void print_queue(T& q)
{
    while (!q.empty())
    {
        string s = q.top().getName();
        cout << s << " ";
        q.pop();
    }
    cout << "\n";
}

struct taskOptions
{
    int rooftopLimit; //Limit for rooftop model
    double amdhalSeq; //Sequential fraction of the job for Amdhal model
    double comComm; //Communication overhead for communication model
};

class Task
{
public:
    Task(string n, vector<long> p, function<double(vector<double>,vector<double>,vector<long>)> s, vector<double> ss, vector<double> as, int i) : name(n), speedup(s), np(p), si(ss), ai(as), index(i) {}
    void setName(string s)
    {
        name = s;
    }
    string getName() const
    {
        return name;
    }
    double time() const;
    double getPriority()
    {
        return priority;
    }
    void setPriority(double p)
    {
        priority = p;
    }
    vector<long> getProcs() const
    {
        return np;
    }
    void setProcs(vector<long> n)
    {
        np=n;
    }
    vector<long> getPrec() const
    {
        return preclist;
    }
    void setPrec(vector<long> n)
    {
        preclist=n;
    }
    vector<double> getSi() const
    {
        return si;
    }
    void setSi(vector<double> n)
    {
        si=n;
    }
    vector<double> getAi() const
    {
        return ai;
    }
    void setAi(vector<double> n)
    {
        ai=n;
    }
    int getIndex() const
    {
        return index;
    }

private:
    string name;
    vector<long> np;
    vector<long> preclist;
    vector<double> si;
    vector<double> ai;
    function<double(vector<double>,vector<double>,vector<long>)> speedup;
    double priority;
    int index;
    taskOptions options;
};

double Task::time() const
{
    /*cout<<"hey\n";
    cout<<si[0]<<"\n";
    cout<<ai[0]<<"\n";
    cout<<np[0]<<"\n";*/
    return speedup(si,ai,np);
}

struct Result
{
    double exec_time;
    vector<Task> tasks;
};


void printResults(Result r)
{
    cout << "Execution time: " << r.exec_time << " seconds.\n";
}


class Event
{
public:
    Event(double t, bool tp, Task rt) : time(t), related_task(rt), type(tp) {}
    bool type; //false = start, true = end
    double time;
    Task related_task;
};
auto cmpEvent = [](Event left, Event right)
{
    return left.time > right.time;
};

void printEvent(Event e)
{
    cerr << e.related_task.getName() << " " << e.time << " " << e.type << "\n";
}
void printEventQueue(priority_queue<Event,vector<Event>,decltype(cmpEvent)> eventQueue)
{
    return;
    priority_queue<Event,vector<Event>,decltype(cmpEvent)> tmpEventQueue(cmpEvent);
    while (!eventQueue.empty())
    {
        Event e = eventQueue.top();
        eventQueue.pop();
        printEvent(e);
        tmpEventQueue.push(e);
    }
    while (!tmpEventQueue.empty())
    {
        Event e = tmpEventQueue.top();
        tmpEventQueue.pop();
        eventQueue.push(e);
    }

}

class Simulator
{
public:
    Simulator(int s, double l, double v) : seed(s), lambda(l), verif_time(v) {}
    void init()
    {
        srand(seed);
    }
    bool fail(Task t);
    double getLambda() const
    {
        return lambda;
    }
    double getVerif() const
    {
        return verif_time;
    }
    void setSeed(double s)
    {
        seed=s;
    }
    void setLambda(double l)
    {
        lambda = l;
    }
    void setVerif(double v)
    {
        verif_time = v;
    }
    function<double(Task)> priority_fun;
    bool batch()
    {
        return batchVar>0;
    }
    bool naive()
    {
        return naiveVar>0;
    }
    bool shelves()
    {
        return shelfVar>0;
    }
    void setBatch(int b)
    {
        batchVar=b;
    }
    void setNaive(int b)
    {
        naiveVar=b;
    }
    void setShelves(int b)
    {
        shelfVar=b;
    }
private:
    double seed;
    double lambda;
    double verif_time;
    int batchVar, naiveVar, shelfVar;
};


Result simulate(Simulator& s, vector<Task> jobs, vector<long> nbProcs, int d,int debuga)
{
    double cTime = 0;
    s.init();
    Result r;

    //Init the priority queue of jobs
    for (unsigned i=0; i<jobs.size(); i++)
    {
        jobs[i].setPriority(s.priority_fun(jobs[i]));
    }

    auto cmpTask = [](Task left, Task right)
    {
        return left.getPriority() < right.getPriority();
    };
    priority_queue<Task,vector<Task>,decltype(cmpTask)> jobQueue(cmpTask);
    priority_queue<Task,vector<Task>,decltype(cmpTask)> tmpJobQueue(cmpTask);

    for (Task j:jobs)
        jobQueue.push(j);

    vector<long> pAvail=nbProcs;
    vector<long> tmpProcs;
    bool Completed[jobs.size()];

    for (int k=0; k<jobs.size(); k++)
    {
        Completed[k]=false;
    }

    //Create a priority queue for the events (ends of tasks)
    //	auto cmpEvent = [](Event left, Event right) { return left.time > right.time; };
    priority_queue<Event,vector<Event>,decltype(cmpEvent)> eventQueue(cmpEvent);
    priority_queue<Event,vector<Event>,decltype(cmpEvent)> tmpEventQueue(cmpEvent);


begin:
    //Schedule the first tasks

    int cpt = 0;

    vector<long> Prec;
    if(debuga>0)
    {
        cout << "At time 0:\n";
    }
    while (!jobQueue.empty())
    {
        Task first = jobQueue.top();
        jobQueue.pop();
        //if (first.getProcs() <= pAvail) //If enough procs available
        tmpProcs=first.getProcs();
        bool isPossible=true;
        for (int k=0; k<tmpProcs.size(); k++)
        {
            isPossible=isPossible and pAvail[k]>=tmpProcs[k];
        }
        Prec=first.getPrec();
        for (int k=0; k<Prec.size(); k++)
        {
            isPossible=isPossible and Completed[Prec[k]];
        }

        if (isPossible)
        {
            //cerr << "Task " << first.getName() << " scheduled.\n";
            if(debuga>0)
            {
                cout << first.getName() << " is scheduled \n";
            }
            tmpProcs=first.getProcs();
            for (int k=0; k<tmpProcs.size(); k++)
            {
                pAvail[k]-=tmpProcs[k];
            }

            if(debuga>1)
            {
                cout << "Procs availables : ";
                for (int k=0; k<tmpProcs.size(); k++)
                {
                    cout << pAvail[k] << " ";
                }
                cout << "\n";
            }
            Event e(cTime+first.time()+s.getVerif(),true,first);
            eventQueue.push(e);
        }
        else
        {
            //otherwise needs to be rescheduled later
            //	cout << "Task " << first.getName() << " not scheduled.\n";
            tmpJobQueue.push(first);
        }
    }

    //Put the non-scheduled jobs back in the queue
    while (!tmpJobQueue.empty())
    {
        jobQueue.push(tmpJobQueue.top());
        tmpJobQueue.pop();
    }

    //MAIN LOOP
    while (!eventQueue.empty())
    {
        Event e = eventQueue.top();
        eventQueue.pop();
        cTime = e.time;

        if(debuga>0)
        {
            cout << "\n\nAt time " << cTime << ":\n";
        }
        if (e.type)
        {
            //It is an ending event so we need to schedule new tasks

            //printEventQueue(eventQueue);
            r.tasks.push_back(e.related_task);
            Completed[e.related_task.getIndex()]=true;
            tmpProcs=e.related_task.getProcs();
            for (int k=0; k<tmpProcs.size(); k++)
            {
                pAvail[k]+=tmpProcs[k];
            }
            if(debuga>0)
            {
                cout << "Task " << e.related_task.getName() << " is finished ";
                if (debuga>1)
                {
                    cout << jobQueue.size() << " tasks remain \n ";

                    cout << "Procs availables : ";
                    for (int k=0; k<tmpProcs.size(); k++)
                    {
                        cout << pAvail[k] << " ";
                    }
                    cout<<'\n';
                }
                else
                {
                    cout<<'\n';
                }
            }
            int cpt = 0;

            //Other jobs to schedule (backfilling)
            while (!jobQueue.empty())
            {
                Task first = jobQueue.top();
                //  cout << first.getName() << " " << jobQueue.size() << "\n";
                jobQueue.pop();
                tmpProcs=first.getProcs();
                Prec=first.getPrec();
                bool isPossible=true;
                for (int k=0; k<tmpProcs.size(); k++)
                {
                    isPossible=isPossible and pAvail[k]>=tmpProcs[k];
                }

                for (int k=0; k<Prec.size(); k++)
                {
                    isPossible=isPossible and Completed[Prec[k]];
                    //cout << Prec[k] << " " << jobs[Prec[k]].getName() << " " << Completed[Prec[k]] << "\n";
                }
                if (isPossible) //check with the existing reservations
                {
                    if(debuga>0)
                    {
                        cout << "--Task " << first.getName() << " scheduled.\n";
                    }
                    for (int k=0; k<tmpProcs.size(); k++)
                    {
                        pAvail[k]-=tmpProcs[k];
                    }
                    Event e(cTime+first.time()+s.getVerif(),true,first);
                    eventQueue.push(e);
                    if(debuga>1)
                    {
                        cout << "Procs availables : ";
                        for (int k=0; k<tmpProcs.size(); k++)
                        {
                            cout << pAvail[k] << " ";
                        }
                        cout << "\n";
                    }
                }
                else   //otherwise needs to be rescheduled later
                {
                    tmpJobQueue.push(first);
                    //cout << " ----Task " << first.getName() << " not scheduled.\n";
                }
            }
            //Put the non-scheduled jobs back in the queue
            while (!tmpJobQueue.empty())
            {
                jobQueue.push(tmpJobQueue.top());
                tmpJobQueue.pop();
            }

        }

    }

    //cout << "Execution time: " << cTime << "\n";

    r.exec_time = cTime;

    return r;
}

double pTaskLength(Task t)
{
    return t.time();
}

double pTaskArea(Task t)
{

    int k;
    long tot=0;
    for (int k=0; k<t.getProcs().size(); k++)
    {
        tot+=t.getProcs()[k];
    }
    return tot*t.time();
}

double pTaskProcs(Task t)
{
    int k;
    long tot=0;
    for (int k=0; k<t.getProcs().size(); k++)
    {
        tot+=t.getProcs()[k];
    }
    return tot;
}

double pTaskLengthS(Task t)
{
    return -t.time();
}

double pTaskAreaS(Task t)
{

    int k;
    long tot=0;
    for (int k=0; k<t.getProcs().size(); k++)
    {
        tot-=t.getProcs()[k];
    }
    return -tot*t.time();
}

double pTaskProcsS(Task t)
{
    int k;
    long tot=0;
    for (int k=0; k<t.getProcs().size(); k++)
    {
        tot-=t.getProcs()[k];
    }
    return tot;
}

double pTaskRandom(Task t)
{
    return rand()/(double)RAND_MAX;
}

double amdSum(vector<double> si, vector<double> ai, vector<long> pi)
{
    double tot=si[0];
    for(int i=1; i<si.size(); i++)
    {
        tot+=si[i]/pi[i-1];
    }
    return(tot);
}

double amdMax(vector<double> si, vector<double> ai, vector<long> pi)
{
    double tot=0;
    for(int i=1; i<si.size(); i++)
    {
        tot=max(tot,si[i]/pi[i-1]);
    }
    tot+=si[0];
    return(tot);
}

double powSum(vector<double> si, vector<double> ai, vector<long> pi)
{
    double tot=0;
    for(int i=1; i<si.size(); i++)
    {
        tot=tot+si[i]/(pow(pi[i-1],ai[i-1]));
    }
    return(tot);
}

double powMax(vector<double> si, vector<double> ai, vector<long> pi)
{
    double tot=0;
    for (int i=1; i<si.size(); i++)
    {
        tot=max(tot,si[i]/pow(pi[i-1],ai[i-1]));
    }
    return (tot);
}

int readInput(string filename, vector<Task> &v, int d, function<double(vector<double>,vector<double>,vector<long>)> speedup)
{
    //double lambda,verif,length;
    string name;
    int proc;
    vector<double> si;
    vector<double> ai;
    vector<long> pi;
    pi= {};
    double s;
    double a;
    int k=0;

    ifstream input(filename,ios::in);

    while (input >> name)
    {
        si= {};
        ai= {};
        for (int i=0; i<d+1; i++)
        {
            input >> s;
            si.push_back(s);
        }

        for (int i=0; i<d; i++)
        {
            input >> a;
            ai.push_back(a);
        }

        v.push_back(Task(name,pi,speedup,si,ai,k));
        k=k+1;
    }


    //cout << "A lower bound on the optimal makespan: " << lower_bound/(*p) << ".\n";

    input.close();

    return 1;
}



int readPrecs(string filename, vector<Task> &v, int d)
{
    //cout<<"comeon\n";
    //cout<<filename<< "\n";
    vector<long> Precs[v.size()];
    int i;
    int j;
    long m=0;
    ifstream input(filename,ios::in);

    while (input >> i >> j)
    {
        //cout<<i<< ' ' << j << '\n';
        Precs[j].push_back(i);
        m+=1;
    }


    for (int i=0; i<v.size(); i++)
    {
        v[i].setPrec(Precs[i]);
    }


    //cout << "A lower bound on the optimal makespan: " << lower_bound/(*p) << ".\n";

    input.close();

    return m;
}

void reduceAlloc(vector<Task>& jobs, int d, vector<long> num_procs, int debuga, double mu){
    
    vector<long> Procs;
    for (int i=0;i<jobs.size();i++){
        Procs=jobs[i].getProcs();
        if(debuga>0)
        {
            cout<<jobs[i].getName() << ", Final allocation for ALGO: ";
        }
        for (int j=0; j<d; j++)
        {
            if (mu*num_procs[j]<Procs[j])
            {
                Procs[j]=ceil(mu*num_procs[j]);
            }
            if(debuga>0)
            {
                cout<<Procs[j]<<" ";
            }
        }
        jobs[i].setProcs(Procs);
        
        if(debuga>0)
        {
            cout<<", Time: "<< jobs[i].time() << "\n";
        }
    }
}

float getAlloc(vector<Task>& jobs, int d, string filename, double mu, vector<long> num_procs,int debuga, int w)
{
    int i,j;
    vector<long> Procs;
    
    vector<string> Heuristics;
    Heuristics.push_back("Algo");
    Heuristics.push_back("MinArea");
    Heuristics.push_back("MinTime");
    if(w==0){
        ifstream input(filename,ios::in);
        string name;
        vector<long> Procs;
        long tmpProcs;
        i=0;

        float LowerBound;
        input >> LowerBound;

        while (input >> name)
        {
            Procs= {};
            for (j=0; j<d; j++)
            {
                input >> tmpProcs;
                Procs.push_back(tmpProcs);
            }

            jobs[i].setProcs(Procs);

            i++;
        }
        return LowerBound;
    }
    else if(w==1){
        if(debuga>0){
            cout << "***********Allocation for MinArea ***************\n\n";
        }
        ifstream input(filename,ios::in);
        string name;
        vector<long> Procs;
        long tmpProcs;
        i=0;

        float LowerBound;
        while (input >> name)
        {
            Procs= {};
            if(debuga>0){
                cout << name << " ";
            }
            for (j=0; j<d; j++)
            {
                input >> tmpProcs;
                Procs.push_back(tmpProcs);
                if (debuga>0){
                    cout << tmpProcs << " ";
                }
            }
            if (debuga>0){
                cout<<"\n";
            }

            jobs[i].setProcs(Procs);

            i++;
        }
        
        for (i=0;i<jobs.size();i++){
            if(debuga>0)
            {
                cout<<jobs[i].getName()<<", Time: "<< jobs[i].time() << "\n";
            }
        }
        
        return 0;
    }
    else if (w==2){
        if(debuga>0){
            cout << "***********Allocation for MinTime ***************\n\n";
            cout << "Allocation for all tasks\n";
        }
        for (j=0;j<d;j++){
            Procs.push_back(num_procs[j]);
            if (debuga>0){
                cout << Procs[j]<<' ';
            }
        }
        if (debuga>0){
            cout << "\n";
        }
        for (i=0;i<jobs.size();i++){
            jobs[i].setProcs(Procs);
            if(debuga>0)
            {
                cout<<jobs[i].getName()<<", Time: "<< jobs[i].time() << "\n";
            }
        }
        return 0;
    }
}

float compute_area(vector<Task>& jobs, vector<long> num_procs,long d){
    long i;
    long b;
    float t;
    float  Area=0;
    vector<long> procs;
    
    for (i=0;i<jobs.size();i++){
        t=jobs[i].time();
        procs=jobs[i].getProcs();
        for (b=0;b<d;b++){
            Area+=t*procs[b]/num_procs[b];
        }
        //cout << t << " " << procs[0] << procs[1] << procs[2]<< " " << jobs[i].getName() << " " << Area<<"\n";
    }
    return Area/d;
}


int main(int argc, char** argv)
{
    struct timeval time;
    int ok=0;
    int ok1=0;
    int debuga;
    float alpha;
    float beta;
    float CP; // Critical Path
    float HBCP; //higher bound on Critical Path
    float Area; // Area
    float HBArea; // higher bound on area
    double HB;
    double RLB;
    double rho;
    long m;
    vector<string> Heuristics;
    Heuristics.push_back("Algo");
    Heuristics.push_back("MinArea");
    Heuristics.push_back("MinTime");

    double val = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    Simulator s(val, 0,0);
    vector<Task> jobs;

    srand((int) time.tv_sec*1000 + (time.tv_usec / 1000));


    ifstream input("args.txt",ios::in);

    string filename;
    string fileout;
    string preclist;
    string rule;
    long d;
    double mu;

    input >> filename;
    input >> preclist;
    input >> fileout;
    input >> rule;
    input >> rho;
    input >> mu;
    input >> d;

    
    //long d = {stoi(string(rd))};

    vector<long> num_procs = {};
    vector<long> num_procs2 = {};
    long p;

    for (int k=0; k<d; k++)
    {
        input >> p;
        num_procs.push_back(p);
    }

    ofstream errfile;
    errfile.open("error.txt", ios_base::app);
    string priority;
    int w;
    string outfile;
    input >> priority;
    input >> debuga;
    input >> alpha;
    input >> beta;
    input >> outfile;
    
    if (rho==0){
        rho=1/(sqrt(d*(1+sqrt(5))/2)+1);
    }
    if(mu==0){
        mu=(3-sqrt(5))/2;
    }
    //rho=0.75;
    HB=1/rho+d/(1-mu)/(1-rho);
    //./moldable jobs failures nb_iter num_procs priority_fun shelves?

    // rule=string(argv[3]);
    for (w=0; w<3; w++)
    {
        if (rule=="amdSum")
        {
            ok = readInput(filename,jobs,d,amdSum);
        }
        else if (rule=="amdMax")
        {
            ok = readInput(filename,jobs,d,amdMax);
        }
        else if (rule=="powSum")
        {
            ok = readInput(filename,jobs,d,powSum);
        }
        else if (rule=="powMax")
        {
            ok = readInput(filename,jobs,d,powMax);
        }
        else
        {
            cerr << "Unrecognized time rule: " << rule << ".\n";
            return -1;
        }
        /*double lambda = atof(argv[2]);
          s.setLambda(lambda);*/
        if (priority == "length")
            s.priority_fun = pTaskLength;
        else if (priority == "procs")
            s.priority_fun = pTaskProcs;
        else if (priority == "area")
            s.priority_fun = pTaskArea;
        else if (priority == "rlength")
            s.priority_fun = pTaskLengthS;
        else if (priority == "rprocs")
            s.priority_fun = pTaskProcsS;
        else if (priority == "rarea")
            s.priority_fun = pTaskAreaS;
        else if (priority == "rand")
            s.priority_fun = pTaskRandom;
        else
        {
            cerr << "Unrecognized priority function: " << priority << ".\n";
            return -1;
        }
        /*string assign_fun = string(argv[6]);
          if (assign_fun != "FF" && assign_fun != "EJ" && assign_fun != "FW") {
          cerr << "Unrecognized assignment cost function: " << assign_fun << ".\n";
          return -1;
          }*/
        m=readPrecs(preclist,jobs,d);
        vector<long> tempprocs;
        float LB;
        if (ok)
        {
            double avg_time = 0;
            int nb_fail_tmp = 0;
            if (w==1){
                fileout=fileout.substr(0, fileout.size()-4)+"mA.txt";
                LB=getAlloc(jobs,d,fileout,mu,num_procs,debuga,w);
                /*for(int i=0;i<jobs.size();i++){
                    cout << jobs[i].getProcs()[0]<< " " << jobs[i].getProcs()[1] << " " << jobs[i].getProcs()[2] << "\n"; 
                }*/
            }
            else{
                LB=getAlloc(jobs,d,fileout,mu,num_procs,debuga,w);
            }
            if (w==0)
            {
                RLB=LB;
            }
            if(debuga>0)
            {
                cout << "\n\n*************PHASE C: START OF SIMULATION FOR "+Heuristics[w]+"************* \n\n";
            }
            if (w<3){ 
            
                for (int i=0;i<num_procs.size();i++){
                    num_procs2.push_back(num_procs[i]*jobs.size());
                    //cout<<num_procs2[i] << " ";
                }
                Result r=simulate(s,jobs,num_procs2,d,0);
                CP=r.exec_time;
                Area=compute_area(jobs,num_procs,d);
                
                HBCP=LB/rho;
                HBArea=LB/(1-rho);
                //cout<<LB<< " " << HBCP << " " << rho << "\n";
                if(CP>HBCP and w==0){
                    errfile << "ERROR !!! Critical Path higher than expected\n";
                    errfile<<filename<<"\n";
                }
                if (Area>HBArea and w==0){
                    errfile << "ERROR !!! Area higher than expected\n";
                    errfile<<filename<<"\n";
                }
                cout << "CP' "<< r.exec_time << " A' "<<Area<< "\n";
            }
            if(w==0){
                reduceAlloc(jobs, d, num_procs, debuga, mu);
                
                Result r=simulate(s,jobs,num_procs2,d,0);
                CP=r.exec_time;
                Area=compute_area(jobs,num_procs,d);
                cout << "CP " << r.exec_time << " A " << Area << "\n";
                
            }
            if(w>0){
                debuga=0;
            }
            Result r=simulate(s,jobs,num_procs,d,debuga);
            
            if (w==0 and r.exec_time >HB and mu<=0.5){
                errfile << "ERROR : EXECUTION TIME SHOULD BE SMALLER\n";
                errfile<<filename<<"\n";
            }
            
            if (debuga>0)
            {
                //cout << "\n\n*************FINAL RESULTS************* \n\n";
            }
            if (w==0)
            {
                HB=LB*HB;
                RLB=LB/(1+alpha)/(1+beta);
            }
            
            ofstream myfile;
            myfile.open(outfile, ios_base::app);
            if (w==0){
                myfile << filename << ' ';
            }
            if (w<2){
                myfile << r.exec_time << ' ';
                cout << Heuristics[w]<<" : Total time of execution: " << r.exec_time << "\n";
            }
            else{
                myfile << r.exec_time << ' ' << RLB << ' '<< HB << '\n';
                cout << Heuristics[w]<<" : Total time of execution: " << r.exec_time << "\n";
                cout << "Lower bound: " << RLB << "\n";
                cout << "Higher bound: " << HB << "\n\n";
            }
            myfile.close();
        }
        //cout<<rho<<"\n";
        jobs={};
    }
    errfile.close();
    return 0;
}
