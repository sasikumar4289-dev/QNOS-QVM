#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <cctype>

namespace {
using Complex = std::complex<double>;
constexpr unsigned MAX_QUBITS=12;
struct Instruction {std::string opcode; std::vector<std::string> args; unsigned line;};
struct Program {std::string path; std::vector<Instruction> code; unsigned maxQ=0;};
std::string trim(std::string s) { auto p=s.find_first_not_of(" \t\r\n"); if(p==std::string::npos)return ""; auto e=s.find_last_not_of(" \t\r\n");return s.substr(p,e-p+1); }
std::string upper(std::string s){for(char &c:s)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));return s;}
unsigned indexOf(const std::string &s, char prefix) {if(s.size()<2 || s[0]!=prefix)throw std::runtime_error("expected register "+std::string(1,prefix)+"N, got "+s);for(size_t k=1;k<s.size();++k)if(!std::isdigit(static_cast<unsigned char>(s[k])))throw std::runtime_error("invalid register "+s);unsigned v=std::stoul(s.substr(1));if(v>=MAX_QUBITS)throw std::runtime_error("register exceeds configured maximum");return v;}
Program parse(const std::string &path){std::ifstream f(path);if(!f)throw std::runtime_error("cannot open "+path);Program p;p.path=path;std::string line;unsigned n=0;const std::map<std::string,unsigned> argc={{"QALLOC",1},{"QRELEASE",1},{"QRESET",1},{"QH",1},{"QX",1},{"QZ",1},{"QS",1},{"QCX",2},{"QMEASURE",2},{"QEND",0}};while(std::getline(f,line)){++n;auto comment=line.find_first_of(";#");if(comment!=std::string::npos)line.resize(comment);std::replace(line.begin(),line.end(),',',' ');std::istringstream ss(line);Instruction i; i.line=n;if(!(ss>>i.opcode))continue;i.opcode=upper(i.opcode);std::string a;while(ss>>a)i.args.push_back(a);auto it=argc.find(i.opcode);if(it==argc.end())throw std::runtime_error("line "+std::to_string(n)+": unknown instruction "+i.opcode);if(i.args.size()!=it->second)throw std::runtime_error("line "+std::to_string(n)+": wrong operand count for "+i.opcode);if(i.opcode!="QEND")for(size_t k=0;k<i.args.size();++k){if(i.opcode=="QMEASURE" && k==1){indexOf(i.args[k],'c');continue;}p.maxQ=std::max(p.maxQ,indexOf(i.args[k],'q')+1);}p.code.push_back(i);}if(p.code.empty()||p.code.back().opcode!="QEND")throw std::runtime_error("program requires QEND");return p;}
struct Result {std::map<unsigned,unsigned> hist;unsigned nqubits=0;unsigned ops=0;};
class Simulator {
 std::vector<Complex> state;std::array<bool,MAX_QUBITS> owned{};std::array<bool,MAX_QUBITS> measured{};std::array<int,MAX_QUBITS> cbits{};unsigned n;std::mt19937_64 &rng;double gateNoise,readoutNoise;unsigned ops=0;
 double uniform(){return std::generate_canonical<double,53>(rng);} 
 void require(unsigned q){if(q>=n||!owned[q])throw std::runtime_error("qubit q"+std::to_string(q)+" not allocated");}
 void gate(unsigned q,Complex a,Complex b,Complex c,Complex d){require(q);size_t mask=size_t(1)<<q;for(size_t k=0;k<state.size();++k)if(!(k&mask)){auto x=state[k],y=state[k|mask];state[k]=a*x+b*y;state[k|mask]=c*x+d*y;}++ops;if(gateNoise>0&&uniform()<gateNoise)rawX(q);}
 void rawX(unsigned q){size_t mask=size_t(1)<<q;for(size_t k=0;k<state.size();++k)if(!(k&mask))std::swap(state[k],state[k|mask]);}
 int measure(unsigned q){require(q);double prob=0;size_t mask=size_t(1)<<q;for(size_t k=0;k<state.size();++k)if(k&mask)prob+=std::norm(state[k]);int actual=(uniform()<prob)?1:0;double norm=std::sqrt(std::max(actual?prob:1-prob,1e-30));for(size_t k=0;k<state.size();++k){if(bool(k&mask)!=bool(actual))state[k]=0;else state[k]/=norm;}measured[q]=true;return actual;}
 public:
 Simulator(unsigned nq,std::mt19937_64 &r,double g,double rd):state(size_t(1)<<nq,Complex(0,0)),n(nq),rng(r),gateNoise(g),readoutNoise(rd){state[0]=1;}
 Result execute(const Program &p){bool ended=false;for(const auto &i:p.code){auto q=[&](size_t k){return indexOf(i.args.at(k),'q');};if(i.opcode=="QEND"){ended=true;break;}if(i.opcode=="QALLOC"){auto x=q(0);if(owned[x])throw std::runtime_error("line "+std::to_string(i.line)+": double allocation");owned[x]=true;}
 else if(i.opcode=="QRELEASE"){auto x=q(0);require(x);owned[x]=false;}
 else if(i.opcode=="QRESET"){auto x=q(0);int v=measure(x);if(v)rawX(x);measured[x]=false;}
 else if(i.opcode=="QH"){auto x=q(0);double s=1/std::sqrt(2.0);gate(x,s,s,s,-s);}
 else if(i.opcode=="QX"){auto x=q(0);gate(x,0,1,1,0);}
 else if(i.opcode=="QZ"){auto x=q(0);gate(x,1,0,0,-1);}
 else if(i.opcode=="QS"){auto x=q(0);gate(x,1,0,0,Complex(0,1));}
 else if(i.opcode=="QCX"){auto c=q(0),t=q(1);require(c);require(t);if(c==t)throw std::runtime_error("QCX requires distinct qubits");for(size_t k=0;k<state.size();++k)if((k&(size_t(1)<<c))&&!(k&(size_t(1)<<t)))std::swap(state[k],state[k|(size_t(1)<<t)]);++ops;if(gateNoise>0&&uniform()<gateNoise)rawX(t);}
 else if(i.opcode=="QMEASURE"){auto x=q(0),c=indexOf(i.args[1],'c');auto v=measure(x);if(readoutNoise>0&&uniform()<readoutNoise)v^=1;cbits[c]=v;}
 }
 if(!ended){throw std::runtime_error("no QEND");}
 Result res;res.nqubits=p.maxQ;res.ops=ops;unsigned value=0;for(unsigned j=0;j<MAX_QUBITS;++j)if(cbits[j])value|=(1u<<j);res.hist[value]=1;return res;}
};
struct Job {unsigned id;Program program;unsigned shots;unsigned seed;double gateNoise,readoutNoise;};
struct JobResult {unsigned id;std::string state;Result result;std::string error;};
class Runtime {
 std::queue<Job> jobs;std::map<unsigned,JobResult> history;unsigned nextId=1;
 public:
 unsigned submit(const Program &p,unsigned shots,unsigned seed,double gn,double rn){if(shots<1||shots>1000000)throw std::runtime_error("shots must be 1..1000000");unsigned id=nextId++;jobs.push({id,p,shots,seed,gn,rn});history[id]={id,"QUEUED",{},""};return id;}
 void dispatch(){if(jobs.empty())return;auto job=jobs.front();jobs.pop();auto &r=history.at(job.id);r.state="RUNNING";try{std::mt19937_64 rng(job.seed);for(unsigned shot=0;shot<job.shots;++shot){Simulator sim(std::max(1u,job.program.maxQ),rng,job.gateNoise,job.readoutNoise);auto single=sim.execute(job.program);for(const auto &[out,count]:single.hist)r.result.hist[out]+=count;r.result.nqubits=single.nqubits;r.result.ops=single.ops;}r.state="COMPLETE";}catch(const std::exception &e){r.state="ERROR";r.error=e.what();}}
 void all(){while(!jobs.empty())dispatch();}
 const JobResult &get(unsigned id)const{auto it=history.find(id);if(it==history.end())throw std::runtime_error("unknown job id");return it->second;}
 void list()const{for(const auto &[id,j]:history)std::cout<<"job "<<id<<" "<<j.state<<"\n";}
};
std::string bits(unsigned value,unsigned width){std::string s;for(unsigned j=0;j<width;++j)s.insert(s.begin(),(value&(1u<<j))?'1':'0');return s;}
void show(const JobResult &r){std::cout<<"job="<<r.id<<" status="<<r.state<<"\n";if(r.state=="ERROR"){std::cout<<"error="<<r.error<<"\n";return;}if(r.state!="COMPLETE")return;unsigned total=0;for(auto [b,n]:r.result.hist)total+=n;std::cout<<"qubits="<<r.result.nqubits<<" operations="<<r.result.ops<<" shots="<<total<<"\n";for(auto [b,n]:r.result.hist)std::cout<<"  "<<bits(b,std::max(1u,r.result.nqubits))<<" : "<<n<<" ("<<std::fixed<<std::setprecision(2)<<100.0*n/total<<"%)\n";}
void verify(const JobResult &r,const std::string &what){if(r.state!="COMPLETE")throw std::runtime_error("job did not complete");unsigned total=0,valid=0,mask=(1u<<r.result.nqubits)-1;for(auto [v,c]:r.result.hist){total+=c;if((what=="zero"&&v==0)||(what=="one"&&v==1)||(what=="bell"&&(v==0||v==3))||(what=="ghz"&&(v==0||v==mask)))valid+=c;}if(valid!=total)throw std::runtime_error("verification FAILED: "+what);std::cout<<"VERIFY "<<what<<" PASS ("<<valid<<"/"<<total<<")\n";}
unsigned num(const std::string &s){size_t pos=0;auto n=std::stoul(s,&pos);if(pos!=s.size())throw std::runtime_error("invalid integer "+s);return static_cast<unsigned>(n);}
void help(){std::cout<<"QNOS-QVM 0.1 | research quantum OS virtual machine\nCommands:\n  help, info, exit\n  load <file.qisa>\n  submit [shots] | dispatch | jobs | result <id>\n  noise <gate_probability> <readout_probability> (0.0..1.0)\nCLI:\n  qnos-qvm run <file.qisa> [--shots N] [--seed N] [--gate-noise P] [--readout-noise P] [--verify bell|ghz|zero|one]\n  qnos-qvm shell\n";}
}
int main(int argc,char **argv){try{std::string mode=argc>1?argv[1]:"shell";if(mode=="--help"||mode=="help"){help();return 0;}if(mode=="run"){if(argc<3)throw std::runtime_error("run requires a .qisa file");auto program=parse(argv[2]);unsigned shots=256,seed=12345;double g=0,r=0;std::string check;for(int i=3;i<argc;++i){std::string a=argv[i];if(i+1>=argc)throw std::runtime_error("missing value after "+a);std::string v=argv[++i];if(a=="--shots")shots=num(v);else if(a=="--seed")seed=num(v);else if(a=="--gate-noise")g=std::stod(v);else if(a=="--readout-noise")r=std::stod(v);else if(a=="--verify")check=v;else throw std::runtime_error("unknown flag "+a);}if(g<0||g>1||r<0||r>1)throw std::runtime_error("noise probabilities must be in [0,1]");Runtime rt;auto id=rt.submit(program,shots,seed,g,r);rt.all();auto &result=rt.get(id);show(result);if(result.state=="ERROR")return 2;if(!check.empty())verify(result,check);return 0;}
 if(mode!="shell"){throw std::runtime_error("unknown command "+mode);}
 help();Runtime rt;Program loaded;bool have=false;double gn=0,rn=0;unsigned seed=12345;std::string line;while(std::cout<<"qnos$ "&&std::getline(std::cin,line)){std::istringstream ss(trim(line));std::string cmd;ss>>cmd;try{if(cmd.empty())continue;if(cmd=="exit"||cmd=="quit")break;if(cmd=="help"){help();continue;}if(cmd=="info"){std::cout<<"backend=standalone_statevector max_qubits="<<MAX_QUBITS<<" queue=FIFO noise=stochastic-X/readout-flip\n";continue;}if(cmd=="load"){std::string path;ss>>path;loaded=parse(path);have=true;std::cout<<"loaded="<<path<<" qubits="<<loaded.maxQ<<"\n";continue;}if(cmd=="noise"){ss>>gn>>rn;if(!ss||gn<0||gn>1||rn<0||rn>1)throw std::runtime_error("usage noise <gate 0..1> <readout 0..1>");std::cout<<"noise updated\n";continue;}if(cmd=="submit"){if(!have)throw std::runtime_error("load a quantum program first");unsigned shots=256;std::string v;if(ss>>v)shots=num(v);auto id=rt.submit(loaded,shots,seed++,gn,rn);std::cout<<"job "<<id<<" QUEUED\n";continue;}if(cmd=="dispatch"){rt.dispatch();std::cout<<"dispatched one quantum job\n";continue;}if(cmd=="jobs"){rt.list();continue;}if(cmd=="result"){unsigned id;if(!(ss>>id))throw std::runtime_error("usage result <id>");show(rt.get(id));continue;}std::cout<<"unknown command; type help\n";}catch(const std::exception &e){std::cout<<"ERROR: "<<e.what()<<"\n";}}return 0;
 }catch(const std::exception &e){std::cerr<<"QNOS-QVM ERROR: "<<e.what()<<"\n";return 1;}}
