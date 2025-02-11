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
#include <algorithm>

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

void printResultsSHORT(Result r,vector<long> p)
{
    cout << r.exec_time << "\n";
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
    vector<long> Precs[v.size()];
    int i;
    int j;
    long m=0;
// cout<<"where\n";
// cout<<filename<<" " << v.size() << "\n";
    ifstream input(filename,ios::in);

    while (input >> i >> j)
    {
        Precs[j].push_back(i);
        m+=1;
    }


    for (int i=0; i<v.size(); i++)
    {
        v[i].setPrec(Precs[i]);
    }
    input.close();
    return m;
}

void assignProcs(vector<Task>& jobs,vector<long> P, int d, long m,string filename, double rho, int debuga,double alpha, double beta)
{
    string fileout=filename.substr(0, filename.size()-4)+"mA.txt";
    if(debuga>0)
    {
        cout<<"\n\n***********PHASE A: BUILDING LINEAR PROGRAM************\n\n";
    }
    ofstream myfile;
    myfile.open("tempForPython.txt");
    ofstream myfile2;
    myfile2.open(fileout);
    
    long i,j,k,l,x,y;
    long n;
    n=jobs.size();
    long comp=0;
    myfile << filename<< " ";
    myfile << debuga << " ";
    myfile << n <<" ";
    myfile << m << " ";
    myfile << d << " ";
    myfile << rho << " ";


    int Precs[m][2];
    vector<double> J[n][2];
    vector<double> T[n][2];
    vector<long> tempPrecs;

    for (i=0; i<n; i++)
    {
        myfile << jobs[i].getName()<<" ";
    }
    // Getting precedence constraints
    for (i=0; i<n; i++)
    {
        tempPrecs=jobs[i].getPrec();
        y=jobs[i].getIndex();
        for (j=0; j<tempPrecs.size(); j++)
        {
            x=tempPrecs[j];
            Precs[comp][0]=x;
            Precs[comp][1]=y;
            comp+=1;
            myfile << x << ' ' << y << ' ';
        }
    }



    double a,t;
    // Getting all possible allocations.

    long cmp=0;
    double temptime;
    double temparea;

    vector< tuple <double,double,long> > possible[n];
    int b;
    long Tottuple=1;
    long tempcmp;
    long Var[d];
    long init=1;
    vector<long> newP[d];
    vector<long> AliasVar;
    int helpBoy[d];

    for (b=0; b<d; b++)
    {
        i=1;
        while(i<P[b]+1)
        {
            newP[b].push_back(i);
            i=(int) (i*(1+alpha))+1;
        }

        //Tottuple*=P[b];
        Tottuple*=newP[b].size();
        init*=P[b];
    }
    if(Tottuple>100000){
        cout<<"!!!!!!!!!! I MIGHT SEGFAULT !!!!!!!!!! \n";
        cout << "This is because I have to much possibility to try ! (Base : "<<init<<" Reduced: "<<Tottuple<<"\n";
        cout << "You should increase alpha\n";
    }
    long PL[Tottuple][d];

    for (i=0; i<n; i++)
    {
        cout<<i<<"\n";
        for(b=0;b<d;b++){
            helpBoy[b]=0;
        }
        cmp=0;
        while(helpBoy[d-1]<newP[d-1].size())
        {
            AliasVar= {};
            for (b=0; b<d; b++)
            {
                AliasVar.push_back(newP[b][helpBoy[b]]);
                PL[cmp][b]=newP[b][helpBoy[b]];
            }
            jobs[i].setProcs(AliasVar);
            t=jobs[i].time();
            a=0;
            for (b=0; b<d; b++)
            {
                a+=AliasVar[b]*1.0/P[b];
            }
            possible[i].push_back(make_tuple(a/d,t,cmp));
            k=0;
            helpBoy[k]+=1;
            //cout<<cmp<<" "<<helpBoy[0]<<" "<<helpBoy[1]<<"\n";
            while(k<d and helpBoy[k]==newP[k].size())
            {
                helpBoy[k]=0;
                k+=1;
                helpBoy[k]+=1;
            }
            if(k==d){
                break;
            }
            cmp+=1;
        }
       // cout<<cmp<<'\n';
        //sort
        if(debuga>2)
        {
            cout << "\nStep 0: Project resources (for Task "<<i<<") (un sorted)\n\n";
            for(j=0; j<possible[i].size(); j++)
            {
                cout<< get<1>(possible[i][j]) << ' '<< get<0>(possible[i][j]) << ' ';
                for(b=0; b<d; b++)
                {
                    cout << PL[get<2>(possible[i][j])][b] << ' ';
                }
                cout<<'\n';
            }
            cout << '\n';
        }
        struct
        {
            bool operator()(tuple <double,double,long> a, tuple <double,double,long> b) const
            {
                return (get<0>(a) < get<0>(b)) or ((get<0>(a) == get<0>(b)) and (get<1>(a) < get<1>(b)));
            }
        } customLess;
        sort(possible[i].begin(),possible[i].end(),customLess);

        if(debuga>2)
        {
            cout << "\nStep 1: Project resources (for Task "<<i<<") (sorted)\n\n";
            for(j=0; j<possible[i].size(); j++)
            {
                cout<< get<1>(possible[i][j]) << ' '<< get<0>(possible[i][j]) << ' ';
                for(b=0; b<d; b++)
                {
                    cout << PL[get<2>(possible[i][j])][b] << ' ';
                }
                cout<<'\n';
            }
            cout << '\n';
        }

        //Erase non dominant choices
        temptime=get<1>(possible[i][0]);
        temparea=get<0>(possible[i][0]);
        j=1;
        while(j<possible[i].size())
        {
            if (get<0>(possible[i][j])==temparea)
            {
                possible[i].erase(possible[i].begin()+j);
            }
            else
            {
                temparea=get<0>(possible[i][j]);
                if (get<1>(possible[i][j])>=temptime/(1+beta))
                {
                    possible[i].erase(possible[i].begin()+j);
                }
                else
                {
                    temptime=get<1>(possible[i][j]);
                    j+=1;
                }
            }
        }

       // cout<<possible[i].size()<<'\n';
        if(debuga>2)
        {
            cout << "\nStep 2: Remove some bad choices (for Task "<<i<<")\n\n";
            for(j=0; j<possible[i].size(); j++)
            {
                cout<< get<1>(possible[i][j]) << ' '<< get<0>(possible[i][j]) << ' ';
                for(b=0; b<d; b++)
                {
                    cout << PL[get<2>(possible[i][j])][b] << ' ';
                }
                cout<<'\n';
            }
            cout << '\n';
        }
        
        
        for (j=0;j<possible[i].size();j++){
            get<0>(possible[i][j])=get<0>(possible[i][j])*get<1>(possible[i][j]);
        }
        
        temptime=get<1>(possible[i][possible[i].size()-1]);
        temparea=get<0>(possible[i][possible[i].size()-1]);
        j=possible[i].size()-2;
        while(j>=0)
        {
            if (temparea<get<0>(possible[i][j])+0.00001){
                possible[i].erase(possible[i].begin()+j);
            }
            else{
                temparea=get<0>(possible[i][j]);
            }
            j-=1;
        }


        //reverse(possible[i].begin(),possible[i].end());

        //Rebuild properly

        for (j=0; j<possible[i].size(); j++)
        {
            J[i][0].push_back(get<1>(possible[i][j]));
            J[i][1].push_back(get<0>(possible[i][j]));
        }
        if(debuga>1)
        {

            cout << "\nStep 3: Rebuild properly (for Task "<<i<<")\n\n";
            for(j=0; j<possible[i].size(); j++)
            {
                cout<< J[i][0][j] << ' ' << J[i][1][j] <<' ';
                for(b=0; b<d; b++)
                {
                    cout << PL[get<2>(possible[i][j])][b] << ' ';
                }
                cout<<'\n';
            }
            cout << '\n';
        }
    }


    //Get all numbers needed
    for (i=0; i<n; i++)
    {
        for (j=0; j<J[i][0].size()-1; j++)
        {
            T[i][1].push_back(J[i][1][j+1]-J[i][1][j]);   // Virtual cost
            T[i][0].push_back(J[i][0][j]);                // Virtual time
        }

        T[i][0].push_back(J[i][0][J[i][0].size()-1]);   // min time
        T[i][1].push_back(J[i][1][0]);

        if(debuga>0)
        {
            cout << "\nStep 4: Build virtualtasks (for Task "<<i<<")\n\n";
        }
        myfile << T[i][0].size()<<' ';
        if(debuga>1)
        {
            cout << "Number of Virtual Tasks for this task: "<<T[i][0].size()<<'\n';
        }
        myfile2<<jobs[i].getName() << " ";
        for(j=0; j<T[i][0].size(); j++)
        {
            myfile<< T[i][0][j] << ' ' << T[i][1][j] << ' ';
            if(debuga>0)
            {
                cout<< T[i][0][j] << ' ' << T[i][1][j] << ' ';
            }
            for(b=0; b<d; b++)
            {
                myfile << PL[get<2>(possible[i][j])][b] << ' ';
                if(j==0){
                    myfile2 << PL[get<2>(possible[i][j])][b] << ' ';
                }
                if (debuga>0)
                {
                    cout << PL[get<2>(possible[i][j])][b] << ' ';
                }
            }
            if(j==0){
                myfile2<<"\n";
            }
            if (debuga>0)
            {
                cout<<'\n';
            }
        }
    }
    myfile.close();
    myfile2.close();
}

int main(int argc, char** argv)
{
    struct timeval time;
    int ok=0;
    int ok1=0;
    double mu;
    double rho;
    long m;
    int debuga;
    double alpha;
    double beta;

    double val = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    Simulator s(val, 0,0);
    vector<Task> jobs;

    srand((int) time.tv_sec*1000 + (time.tv_usec / 1000));


    ifstream input("args.txt",ios::in);

    string filename;
    string fileout;
    string preclist;
    string rule;
    string outfile;
    long d;
    input >> filename;
    input >> preclist;
    input >> fileout;
    input >> rule;
    input >> rho;
    input >> mu;
    input >> d;

    if (rho==0){
        rho=1/(sqrt(d*(1+sqrt(5))/2)+1);
    }
    if(mu==0){
        mu=(3-sqrt(5))/2;
    }

    //long d = {stoi(string(rd))};

    //./moldable jobs failures nb_iter num_procs priority_fun shelves?

    // rule=string(argv[3]);
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
    vector<long> num_procs = {};
    vector<long> num_procs2 = {};
    long p;
    for (int k=0; k<d; k++)
    {
        input >> p;
        num_procs.push_back(p);
        num_procs2.push_back(p);
    }
    string priority;
    input >> priority;
    input >> debuga;
    input >> alpha;
    input >> beta;
    input >> outfile;

    if(debuga==0)
    {
        cout<<"rho "<<rho<<"\n";
        cout<<"mu "<<mu<<"\n";
    }
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
    if (ok)
    {
        assignProcs(jobs,num_procs,d,m,fileout,rho,debuga,alpha,beta);
    }


    return 0;
}
