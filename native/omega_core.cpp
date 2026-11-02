// EvoLab Omega v10.6 / compact dynamic organism state, packed contact geometry, deep-time ledger; exact deep-time compute and evolution observatory and evolutionary atlas, preserving the v9.3 genome and world core.
// C++17, no external dependencies. This model does NOT reproduce the v7 JS model's physics.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <csignal>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <chrono>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;
static constexpr int WIDTH=1300,HEIGHT=850,GX=65,GY=43,NC=GX*GY,MAX_BOD=224,LEGACY_BOD=40,MAX_NEU=80,MAX_EDGE=352; // legacy world dimensions
static constexpr float PI=3.14159265358979323846f;
struct RNG{uint64_t state; explicit RNG(uint64_t s):state(s? s:1){} uint32_t next(){state^=state>>12;state^=state<<25;state^=state>>27;return uint32_t((state*2685821657736338717ULL)>>32);} float f(){return (next()+0.5f)/4294967296.f;} int pick(int n){return n>0?int(next()%uint32_t(n)):0;} float range(float a,float b){return a+(b-a)*f();} float step(){return f()+f()+f()+f()-2.f;}};
static float bound(float x,float a,float b){return max(a,min(b,x));}
// Reusable deterministic parallel executor: independent world cells only. Never changes RNG order.
class CellWorkers{
 int count; vector<thread> workers; mutex lock; condition_variable ready,finished; bool closing=false;
 size_t generation=0; int completed=0,extent=0; function<void(int,int)> job;
 void run(int id){size_t seen=0;unique_lock<mutex> guard(lock);while(true){ready.wait(guard,[&]{return closing||generation!=seen;});if(closing)return;seen=generation;auto fn=job;int len=extent;guard.unlock();fn(id*len/count,(id+1)*len/count);guard.lock();completed++;finished.notify_one();}}
 public:explicit CellWorkers(int n):count(max(1,n)){for(int k=1;k<count;k++)workers.emplace_back([this,k]{run(k);});}
 ~CellWorkers(){if(workers.empty())return;{lock_guard<mutex> g(lock);closing=true;}ready.notify_all();for(auto &t:workers)t.join();}
 void each(int extent_,function<void(int,int)> fn){if(count==1){fn(0,extent_);return;} {lock_guard<mutex> g(lock);extent=extent_;job=move(fn);completed=0;generation++;}ready.notify_all();job(0,extent/count);unique_lock<mutex> g(lock);finished.wait(g,[&]{return completed==count-1;});job=nullptr;}
};

// v10.1: finite-width, compliant tissue is part of the inherited genome.
// Legacy V1-V16 checkpoints used this exact original, smaller representation.
struct LegacyNode{uint8_t parent,type,ctrl,sense;float length,angle,growth,motor;};
struct LegacyNodeV17{uint8_t parent,type,ctrl,sense;float length,angle,growth,motor;float thickness,flexibility;};
struct LegacyNodeV18{uint8_t parent,type,ctrl,sense;float length,angle,growth,motor;float thickness,flexibility,conductance,stiffness;};
struct Node{uint8_t parent,type,ctrl,sense;float length,angle,growth,motor;float thickness=2.4f,flexibility=.65f,conductance=.6f,stiffness=.65f;uint8_t regulator=0;float regulatorWeight=0.f;};
static_assert(sizeof(LegacyNode)==20 && sizeof(LegacyNodeV17)==28 && sizeof(LegacyNodeV18)==36 && sizeof(Node)==44,"Unrecognized native body node binary ABI");
struct Edge{uint8_t from,to;float w;};
// Transient derived data: never saved or inherited. Each worker owns distinct slots.
struct BodyPreparation{array<float,MAX_BOD> waves{};float sensorBoost=0.f;};
struct Genome{uint8_t neurons=4;vector<Node> body;vector<Edge> edges;float metabolism=1,vision=1,threshold=1,signal=0.5f,drag=1,tempOpt=.5f,roleBias=0,collectivity=0.5f,niche=0.f,engineering=0.f;float assimilationBias=0.f,transportGene=.65f;};
// Hot fields are structure-of-arrays; genomes and recurrent networks are cold heterogenous sidecars.
struct Pop{
 vector<float>x,y,vx,vy,heading,energy,age,sig,neural,signalMemory,jointAngle,jointVelocity,tissueSignal;
 vector<vector<float>> cargoFood,cargoDetritus,flowFood,flowDetritus;vector<int>generation,alive,kind,offspring;vector<uint64_t>ancestor,uid,parentUid;vector<Genome>genomes;
 int cap,bodyStride,neuralStride;bool compact=false;
 vector<vector<float>> neuralSlabs,angleSlabs,velocitySlabs,signalSlabs;
 float& neuronAt(int i,int j){return compact?neuralSlabs[size_t(i)][size_t(j)]:neural[size_t(i)*neuralStride+j];}
 float const& neuronAt(int i,int j)const{return compact?neuralSlabs[size_t(i)][size_t(j)]:neural[size_t(i)*neuralStride+j];}
 float& jointAt(int i,int j){return compact?angleSlabs[size_t(i)][size_t(j)]:jointAngle[size_t(i)*bodyStride+j];}
 float const& jointAt(int i,int j)const{return compact?angleSlabs[size_t(i)][size_t(j)]:jointAngle[size_t(i)*bodyStride+j];}
 float& velocityAt(int i,int j){return compact?velocitySlabs[size_t(i)][size_t(j)]:jointVelocity[size_t(i)*bodyStride+j];}
 float const& velocityAt(int i,int j)const{return compact?velocitySlabs[size_t(i)][size_t(j)]:jointVelocity[size_t(i)*bodyStride+j];}
 float& signalAt(int i,int j){return compact?signalSlabs[size_t(i)][size_t(j)]:tissueSignal[size_t(i)*bodyStride+j];}
 float const& signalAt(int i,int j)const{return compact?signalSlabs[size_t(i)][size_t(j)]:tissueSignal[size_t(i)*bodyStride+j];}
 void allocateLife(int i,int body,int neurons){if(compact){neuralSlabs[size_t(i)].assign(neurons,0.f);angleSlabs[size_t(i)].assign(body,0.f);velocitySlabs[size_t(i)].assign(body,0.f);signalSlabs[size_t(i)].assign(body,0.f);}else{fill_n(neural.begin()+size_t(i)*neuralStride,neuralStride,0.f);fill_n(jointAngle.begin()+size_t(i)*bodyStride,bodyStride,0.f);fill_n(jointVelocity.begin()+size_t(i)*bodyStride,bodyStride,0.f);fill_n(tissueSignal.begin()+size_t(i)*bodyStride,bodyStride,0.f);}}
 uint64_t allocatedFloats()const{if(!compact)return neural.size()+jointAngle.size()+jointVelocity.size()+tissueSignal.size();uint64_t n=0;for(auto const&v:neuralSlabs)n+=v.size();for(auto const&v:angleSlabs)n+=v.size();for(auto const&v:velocitySlabs)n+=v.size();for(auto const&v:signalSlabs)n+=v.size();return n;}
 uint64_t activeFloats()const{uint64_t n=0;for(int i=0;i<cap;i++)if(alive[i]){n+=size_t(genomes[i].neurons)+3*genomes[i].body.size();}return n;}
 
 explicit Pop(int n,int bodyCap=40,int neuronCap=32,bool pool=false):cap(n),bodyStride(max(96,bodyCap)),neuralStride(max(32,neuronCap)),compact(pool){x.resize(n);y.resize(n);vx.resize(n);vy.resize(n);heading.resize(n);energy.resize(n);age.resize(n);sig.resize(n);signalMemory.resize(n);if(!compact){neural.resize(n*neuralStride);jointAngle.resize(n*bodyStride);jointVelocity.resize(n*bodyStride);tissueSignal.resize(n*bodyStride);}else{neuralSlabs.resize(n);angleSlabs.resize(n);velocitySlabs.resize(n);signalSlabs.resize(n);}generation.resize(n);ancestor.resize(n);uid.resize(n);parentUid.resize(n);alive.resize(n);kind.resize(n);offspring.resize(n);genomes.resize(n);cargoFood.resize(n);cargoDetritus.resize(n);flowFood.resize(n);flowDetritus.resize(n);}
};
struct Settings{uint64_t seed=261029;int initial=500,cap=2000,ticks=30000,sample=250,visual=90;float mutation=.13f;bool neutral=false;float resourceRate=1.f,season=.25f,climate=.5f,predation=.5f,sharing=.5f,cohesion=.85f,specialization=1.f,cooperation=0.8f,integration=0.f,scavenging=0.f,plasticity=0.f,ecology=0.f,development=0.f,grand=0.f,habitats=0.f,transport=0.f,regionality=0.f;int threads=1,worldScale=1,scheduler=0,compute=0,lifeParallel=0,collisionIndex=0,allContacts=0,bodyCap=40,founderNodes=0,contactDedupe=0,quietActivityLimit=12,neuronCap=32,edgeCap=128,founderNeurons=0,sparseBuckets=0,graphPool=0,contactCache=0,historyLedger=0;float quietFluxTolerance=0.f;float morphogenesis=0.f,softBody=0.f,tissueCoupling=0.f,regulatoryNetwork=0.f,metabolicEcology=0.f;string organAblation="none",genomePrune="none",genomeRescue="",ablate="none",csv="",record="",lineage="",organizationLog="",organizationFitnessLog="",checkpoint="",resume="";string regionalLedger="",eventLedger="";int checkpointEvery=0;int rate=0;bool specializationSpecified=false;float experimentalResource=-1.f,experimentalMigrationCost=0.f,experimentalIsolation=0.f;};
static void option(Settings &p,int argc,char **argv){for(int i=1;i<argc;i++){string s=argv[i];if(i+1>=argc){cerr<<"Option requires value: "<<s<<'\n';exit(2);}string v=argv[++i];try{if(s=="--experiment-resource")p.experimentalResource=stof(v);else if(s=="--experiment-migration-cost")p.experimentalMigrationCost=stof(v);else if(s=="--experiment-isolation")p.experimentalIsolation=stof(v);else if(s=="--genome-prune")p.genomePrune=v;else if(s=="--genome-rescue")p.genomeRescue=v;else if(s=="--organ-ablation")p.organAblation=v;else if(s=="--metabolic-ecology")p.metabolicEcology=stof(v);else if(s=="--regional-ledger")p.regionalLedger=v;else if(s=="--event-ledger")p.eventLedger=v;else if(s=="--graph-pool")p.graphPool=stoi(v);else if(s=="--contact-cache")p.contactCache=stoi(v);else if(s=="--history-ledger")p.historyLedger=stoi(v);else if(s=="--sparse-buckets")p.sparseBuckets=stoi(v);else if(s=="--founder-neurons")p.founderNeurons=stoi(v);else if(s=="--neuron-cap")p.neuronCap=stoi(v);else if(s=="--edge-cap")p.edgeCap=stoi(v);else if(s=="--body-cap")p.bodyCap=stoi(v);else if(s=="--founder-nodes")p.founderNodes=stoi(v);else if(s=="--contact-dedupe")p.contactDedupe=stoi(v);else if(s=="--quiet-activity-limit")p.quietActivityLimit=stoi(v);else if(s=="--quiet-flux-tolerance")p.quietFluxTolerance=stof(v);else if(s=="--all-contacts")p.allContacts=stoi(v);else if(s=="--regulatory-network")p.regulatoryNetwork=stof(v);else if(s=="--tissue-coupling")p.tissueCoupling=stof(v);else if(s=="--collision-index")p.collisionIndex=stoi(v);else if(s=="--soft-body")p.softBody=stof(v);else if(s=="--morphogenesis")p.morphogenesis=stof(v);else if(s=="--life-parallel")p.lifeParallel=stoi(v);else if(s=="--compute")p.compute=stoi(v);else if(s=="--scheduler")p.scheduler=stoi(v);else if(s=="--rate")p.rate=stoi(v);else if(s=="--threads")p.threads=stoi(v);else if(s=="--world-scale")p.worldScale=stoi(v);else if(s=="--grand")p.grand=stof(v);else if(s=="--habitats")p.habitats=stof(v);else if(s=="--transport")p.transport=stof(v);else if(s=="--regionality")p.regionality=stof(v);else if(s=="--seed")p.seed=stoull(v);else if(s=="--initial")p.initial=stoi(v);else if(s=="--capacity")p.cap=stoi(v);else if(s=="--ticks")p.ticks=stoi(v);else if(s=="--sample")p.sample=stoi(v);else if(s=="--visual")p.visual=stoi(v);else if(s=="--mutation")p.mutation=stof(v);else if(s=="--ablate")p.ablate=v;else if(s=="--neutral")p.neutral=(v=="1");else if(s=="--csv")p.csv=v;else if(s=="--record")p.record=v;else if(s=="--lineage")p.lineage=v;else if(s=="--organization-log")p.organizationLog=v;else if(s=="--organization-fitness-log")p.organizationFitnessLog=v;else if(s=="--resource")p.resourceRate=stof(v);else if(s=="--season")p.season=stof(v);else if(s=="--climate")p.climate=stof(v);else if(s=="--predation")p.predation=stof(v);else if(s=="--sharing")p.sharing=stof(v);else if(s=="--cohesion")p.cohesion=stof(v);else if(s=="--specialization"){p.specialization=stof(v);p.specializationSpecified=true;}else if(s=="--cooperation")p.cooperation=stof(v);else if(s=="--integration")p.integration=stof(v);else if(s=="--scavenging")p.scavenging=stof(v);else if(s=="--plasticity")p.plasticity=stof(v);else if(s=="--ecology")p.ecology=stof(v);else if(s=="--development")p.development=stof(v);else if(s=="--checkpoint")p.checkpoint=v;else if(s=="--resume")p.resume=v;else if(s=="--checkpoint-every")p.checkpointEvery=stoi(v);else{cerr<<"Unknown option: "<<s<<'\n';exit(2);}}catch(...){cerr<<"Invalid parameter: "<<s<<'\n';exit(2);}}
 if(!std::isfinite(p.experimentalResource)||!std::isfinite(p.experimentalMigrationCost)||!std::isfinite(p.experimentalIsolation)||p.experimentalResource< -1.f||p.experimentalResource>3.f||(p.experimentalResource<0.f&&p.experimentalResource!= -1.f)||p.experimentalMigrationCost<0.f||p.experimentalMigrationCost>3.f||p.experimentalIsolation<0.f||p.experimentalIsolation>1.f){cerr<<"Invalid experimental environment parameters\n";exit(2);}
 if(!(p.genomePrune=="none"||p.genomePrune=="absorber"||p.genomePrune=="digester"||p.genomePrune=="root")||!(p.organAblation=="none"||p.organAblation=="absorber"||p.organAblation=="digester"||p.organAblation=="root")||p.rate<0||p.rate>100000||p.cap<1||p.cap>100000||p.initial<1||p.initial>p.cap||p.ticks<1||p.ticks>5000000||p.sample<1||p.sample>100000||p.visual<0||p.visual>200||p.mutation<0||p.mutation>1||!(p.ablate=="none"||p.ablate=="motor"||p.ablate=="signal"||p.ablate=="climate"||p.ablate=="interaction"||p.ablate=="organization"||p.ablate=="specialization"||p.ablate=="cooperation"||p.ablate=="integration")||p.resourceRate<0||p.resourceRate>3||p.season<0||p.season>1||p.climate<0||p.climate>1||p.predation<0||p.predation>2||p.sharing<0||p.sharing>2||p.cohesion<0||p.cohesion>2||p.specialization<0||p.specialization>2||p.cooperation<0||p.cooperation>2||p.integration<0||p.integration>2||p.scavenging<0||p.scavenging>2||p.plasticity<0||p.plasticity>2||p.ecology<0||p.ecology>2||p.development<0||p.development>2||p.grand<0||p.grand>2||p.habitats<0||p.habitats>2||p.transport<0||p.transport>2||p.regionality<0||p.regionality>2||p.scheduler<0||p.scheduler>1||p.compute<0||p.compute>1||p.lifeParallel<0||p.lifeParallel>1||p.collisionIndex<0||p.collisionIndex>1||p.allContacts<0||p.allContacts>1||p.contactDedupe<0||p.contactDedupe>1||p.bodyCap<40||p.bodyCap>MAX_BOD||p.neuronCap<32||p.neuronCap>MAX_NEU||p.founderNeurons<0||p.founderNeurons>p.neuronCap||p.sparseBuckets<0||p.sparseBuckets>2||p.graphPool<0||p.graphPool>1||p.contactCache<0||p.contactCache>1||p.historyLedger<0||p.historyLedger>1||p.edgeCap<128||p.edgeCap>MAX_EDGE||p.founderNodes<0||p.founderNodes>p.bodyCap||p.quietActivityLimit<0||p.quietActivityLimit>100000||!std::isfinite(p.quietFluxTolerance)||p.quietFluxTolerance<0||p.quietFluxTolerance>.05f||!std::isfinite(p.regulatoryNetwork)||p.regulatoryNetwork<0||p.regulatoryNetwork>2||!std::isfinite(p.metabolicEcology)||p.metabolicEcology<0.f||p.metabolicEcology>2.f||(!std::isfinite(p.morphogenesis))||p.morphogenesis<0||p.morphogenesis>2||!std::isfinite(p.softBody)||p.softBody<0||p.softBody>2||!std::isfinite(p.tissueCoupling)||p.tissueCoupling<0||p.tissueCoupling>2||p.threads<1||p.threads>12||p.worldScale<1||p.worldScale>3||p.checkpointEvery<0||p.checkpointEvery>1000000){cerr<<"Arguments out of range\n";exit(2);} }
// Versioned, integrity-checked checkpoint encoding. No native object-pointer serialization.
template<class T> static void put(ostream &o,T const&v){static_assert(is_trivially_copyable<T>::value,"pod");o.write(reinterpret_cast<char const*>(&v),sizeof v);}
template<class T> static T get(istream &i){T v{};i.read(reinterpret_cast<char*>(&v),sizeof v);if(!i)throw runtime_error("Truncated checkpoint");return v;}
template<class T> static void putvec(ostream &o,vector<T> const&v){static_assert(is_trivially_copyable<T>::value,"pod vector");uint64_t n=v.size();put(o,n);if(n)o.write(reinterpret_cast<char const*>(v.data()),n*sizeof(T));}
template<class T> static void getvec(istream &i,vector<T>&v,size_t boundN){uint64_t n=get<uint64_t>(i);if(n>boundN)throw runtime_error("Checkpoint vector exceeds safe bound");v.resize(size_t(n));if(n)i.read(reinterpret_cast<char*>(v.data()),n*sizeof(T));if(!i)throw runtime_error("Truncated checkpoint vector");}
template<class T,size_t N> static void putarray(ostream &o,array<T,N> const&v){o.write(reinterpret_cast<char const*>(v.data()),sizeof(T)*N);}
template<class T,size_t N> static void getarray(istream &i,array<T,N>&v){i.read(reinterpret_cast<char*>(v.data()),sizeof(T)*N);if(!i)throw runtime_error("Truncated checkpoint array");}
static void putarray(ostream &o,vector<float> const&v){if(!v.empty())o.write(reinterpret_cast<char const*>(v.data()),v.size()*sizeof(float));}
static void getarray(istream &i,vector<float>&v){if(!v.empty())i.read(reinterpret_cast<char*>(v.data()),v.size()*sizeof(float));if(!i)throw runtime_error("Truncated world grid");}
static uint64_t digest(string const&s){uint64_t h=14695981039346656037ULL;for(unsigned char c:s){h^=c;h*=1099511628211ULL;}return h;}
struct RegionalCrossing{uint64_t tick,uid,ancestor;int from,to,generation;};
struct Sim{
 Settings c;RNG rng;RNG metabolismRng;Pop p;const int WIDTH,HEIGHT,GX,GY,NC;vector<float>food,detritus,moisture,pheromone,baseMoisture,temperature,soil,climateWave,patchA,patchB;vector<vector<int>>bins;vector<int>occupiedCells;vector<int>cellRegions;vector<float>regionalPhases,regionalGains,regionalThermal;vector<uint64_t> migrationMatrix;vector<RegionalCrossing> recentCrossings;vector<float>transportDeltas,signalDeltas;uint64_t transportEvents=0,regionalMigrants=0,blockedCrossings=0,zeroTransportEdges=0,reusedOscillations=0;double migrationEnergyPaid=0.;double transportedDetritus=0,crossRegionMatter=0;vector<int>live,available;vector<BodyPreparation> preparedBody;vector<float> collisionReach;vector<uint8_t> collisionReachReady;uint64_t dedupeAvoided=0,tileDuplicatePrunes=0,skippedQuietEdges=0,quietRegionSteps=0;double skippedDetritusFlux=0,skippedSignalFlux=0;uint64_t contactCacheSlots=0;uint64_t broadphaseRejects=0,narrowphaseTests=0,conservativeRejects=0,globalCandidatePairs=0,globalExactPairs=0,globalCollisionPairs=0,regulatoryRewires=0,crossParentRecombinations=0;unique_ptr<CellWorkers> environmentalWorkers;uint64_t extendedHarvests=0,surfaceContacts=0;double extendedEnergy=0,membraneHarvest=0;uint64_t tick=0,births=0,deaths=0,contacts=0,exchanges=0,signals=0,predations=0,shares=0,mutations=0,structuralMutations=0,nextUID=0;ofstream *lineageLog=nullptr,*organizationLog=nullptr,*organizationFitnessLog=nullptr,*eventLedgerLog=nullptr;uint64_t eventLedgerHash=1469598103934665603ULL,eventLedgerCount=0;double foodCargoUptake=0,detritusCargoUptake=0,foodCargoConverted=0,detritusCargoConverted=0,metabolicDissipation=0,cargoReturnedOnDeath=0;
 // v10.8: per-tick observed substrate flux from inherited absorbers/digesters/root tips.
 // Transient measurement only. Never feeds back into the simulation, genetics or RNG.
 array<double,3> functionalUptake{};double anchorVelocityLoss=0;
  // Observer-only event ledger for expressed inherited organ carriers and noncarriers.
  // Arrays indexed by organ (absorber/digester/root) and presence (0=no,1=yes).
  array<array<uint64_t,2>,3> organExposure{},organOffspring{},organDeaths{},organInheritedBirths{};
  array<array<uint64_t,2>,3> organParentBirths{};uint64_t organLedgerSinceTick=0;
  uint64_t genomePrunedLives=0,genomePrunedNodes=0,genomeRescuedLives=0,genomeRescuedNodes=0;
  static int organIndex(uint8_t t){return t==2?0:t==5?1:t==7?2:-1;}
  bool carrier(int i,int k,bool mature=false)const{auto const&g=p.genomes[i];
    for(size_t b=1;b<g.body.size();b++)if(organIndex(g.body[b].type)==k&&(!mature||expressed(i,b)))return true;
    return false;}
  bool organDisabled(int type)const{return (type==2&&c.organAblation=="absorber")||(type==5&&c.organAblation=="digester")||(type==7&&c.organAblation=="root");}
  void recordBirth(int id,bool founder){if(founder)return;for(int k=0;k<3;k++)organInheritedBirths[k][carrier(id,k)?1:0]++;}
  void recordReproduction(int id){for(int k=0;k<3;k++)organParentBirths[k][carrier(id,k)?1:0]++;}
  void recordDeath(int id){for(int k=0;k<3;k++)organDeaths[k][carrier(id,k)?1:0]++;}
  void recordExposure(){for(int id:live)for(int k=0;k<3;k++)organExposure[k][carrier(id,k)?1:0]++;}
// Exact genomic intervention only for checkpoint-based research; NEVER called in public web requests.
 // Remove a specific node type AND its descendants (preserving parent<child topology).
 // Live tissue/joints are compacted to retained indexes; cargo mass from removed branches
 // is transferred into retained root cargo rather than created/destroyed.
 uint64_t pruneGenomes(string const&organ){
  int organType=organ=="absorber"?2:organ=="digester"?5:organ=="root"?7:-1;
  if(organType<0)return 0;
  uint64_t removed=0;
  for(int id:live){Genome &g=p.genomes[id];int n=int(g.body.size());if(n<2)continue;
   vector<int> mapIndex(size_t(n),-1);vector<Node> next;next.reserve(n);
   vector<float> newFood,newDet,newFF,newFD;newFood.reserve(n);newDet.reserve(n);newFF.reserve(n);newFD.reserve(n);
   vector<float> newAngle,newVel,newSignal;newAngle.reserve(n);newVel.reserve(n);newSignal.reserve(n);
   float surplusFood=0.f,surplusDet=0.f;bool changed=false;
   for(int j=0;j<n;j++){
    Node old=g.body[size_t(j)];bool cut=j>0&&(old.type==organType||old.parent>=j||mapIndex[size_t(old.parent)]<0);
    if(cut){changed=true;removed++;if(j<int(p.cargoFood[id].size()))surplusFood+=p.cargoFood[id][size_t(j)];if(j<int(p.cargoDetritus[id].size()))surplusDet+=p.cargoDetritus[id][size_t(j)];continue;}
    mapIndex[size_t(j)]=int(next.size());if(j>0)old.parent=uint8_t(mapIndex[size_t(old.parent)]);
    next.push_back(old);
    newFood.push_back(j<int(p.cargoFood[id].size())?p.cargoFood[id][size_t(j)]:0.f);
    newDet.push_back(j<int(p.cargoDetritus[id].size())?p.cargoDetritus[id][size_t(j)]:0.f);
    newFF.push_back(0.f);newFD.push_back(0.f);
    newAngle.push_back(p.jointAt(id,j));newVel.push_back(p.velocityAt(id,j));newSignal.push_back(p.signalAt(id,j));
   }
   if(!changed)continue;
   ++genomePrunedLives;
   // Shed organ cargo back into its local environment, conserving matter.
   food[index(p.x[id],p.y[id])]+=surplusFood;detritus[index(p.x[id],p.y[id])]+=surplusDet;
   g.body.swap(next);p.cargoFood[id].swap(newFood);p.cargoDetritus[id].swap(newDet);p.flowFood[id].swap(newFF);p.flowDetritus[id].swap(newFD);
   for(size_t j=0;j<newAngle.size();j++){p.jointAt(id,int(j))=newAngle[j];p.velocityAt(id,int(j))=newVel[j];p.signalAt(id,int(j))=newSignal[j];}
   if(p.compact){p.angleSlabs[id].resize(g.body.size());p.velocitySlabs[id].resize(g.body.size());p.signalSlabs[id].resize(g.body.size());}
  }
  genomePrunedNodes+=removed;
  return removed;
 }
 // Restore the exact original hereditary topology only for individuals whose UID
 // existed in the donor checkpoint. Descendants born since the donor cannot be
 // genetically reconstructed and remain unchanged. Existing energy, spatial state,
 // world resources and RNG remain from the intervened world.
 uint64_t rescueGenomes(Sim const&donor){
  if(c.seed!=donor.c.seed||donor.tick>tick||c.worldScale!=donor.c.worldScale||p.cap!=donor.p.cap)throw runtime_error("Rescue donor must come from same seed/geometry at or before current tick");
  unordered_map<uint64_t,int> byUID;byUID.reserve(donor.live.size());
  for(int j:donor.live)byUID.emplace(donor.p.uid[j],j);
  uint64_t changed=0;
  for(int id:live){auto it=byUID.find(p.uid[id]);if(it==byUID.end())continue;
   auto const &old=donor.p.genomes[it->second];auto &g=p.genomes[id];
   if(g.body.size()==old.body.size()&&equal(g.body.begin(),g.body.end(),old.body.begin(),[](Node const&a,Node const&b){return memcmp(&a,&b,sizeof(Node))==0;}))continue;
   if(old.body.size()>size_t(p.bodyStride))throw runtime_error("Rescue genome exceeds body allocation");
   float cargoFood=accumulate(p.cargoFood[id].begin(),p.cargoFood[id].end(),0.f),cargoDet=accumulate(p.cargoDetritus[id].begin(),p.cargoDetritus[id].end(),0.f);
   g.body=old.body;g.edges=old.edges;g.neurons=old.neurons;
   p.cargoFood[id].assign(g.body.size(),0.f);p.cargoDetritus[id].assign(g.body.size(),0.f);
   p.flowFood[id].assign(g.body.size(),0.f);p.flowDetritus[id].assign(g.body.size(),0.f);
   // Old organ cargo cannot be reconstructed; release it locally instead of inventing nutrient.
   food[index(p.x[id],p.y[id])]+=cargoFood;detritus[index(p.x[id],p.y[id])]+=cargoDet;
   if(p.compact){p.angleSlabs[id].resize(g.body.size(),0.f);p.velocitySlabs[id].resize(g.body.size(),0.f);p.signalSlabs[id].resize(g.body.size(),0.f);}
   // Old exact kinematic state cannot be reconstructed from a different world.
   for(size_t j=0;j<g.body.size();j++){p.jointAt(id,int(j))=0.f;p.velocityAt(id,int(j))=0.f;p.signalAt(id,int(j))=0.f;}
   ++changed;genomeRescuedNodes+=old.body.size();
  }
  genomeRescuedLives+=changed;return changed;
 }
bool lineageExtended=true,lineageEcologyExtended=true;int maxGen=0;unordered_map<uint64_t,int>heritage;unordered_map<int,int>generationPop;struct Interaction {uint64_t tick,a,b;int type;uint32_t count=1;};vector<Interaction>events;unordered_map<uint64_t,Interaction>relationships;struct Bond{uint64_t a,b;float rest;uint32_t age;float transferred;uint8_t origin=0;uint64_t groupId=0,parentGroupId=0;uint32_t groupGeneration=0;uint32_t offspringGroups=0;float coharvest=0,investment=0;};struct OldBondV85{uint64_t a,b;float rest;uint32_t age;float transferred;uint8_t origin=0;uint64_t groupId=0,parentGroupId=0;uint32_t groupGeneration=0;};struct OldBond{uint64_t a,b;float rest;uint32_t age;float transferred;};struct OldBondV84{uint64_t a,b;float rest;uint32_t age;float transferred;uint8_t origin=0;};vector<Bond>bonds;uint64_t bondsFormed=0,bondsBroken=0,propagulesBorn=0,propagulesMatured=0;double bondedEnergy=0;uint64_t nextGroupId=0,descendantGroupBirths=0,cooperativeDefenses=0;double preventedLoss=0,integratedHarvest=0,integratedCost=0;uint64_t integratedEvents=0,productiveOrganizations=0,scavenges=0;double scavengedEnergy=0;uint32_t maxGroupGeneration=0;uint64_t subtreeCopies=0,reparentedBranches=0,feedbackMutations=0,soilEdits=0,soilMineralizations=0,nicheHarvests=0,developmentMutations=0;double recycledMatter=0,ecologicalHarvest=0;
 explicit Sim(Settings s):c(s),rng(s.seed),metabolismRng(s.seed ^ 0xf3c8a501b72d9e43ULL),p(s.cap,s.bodyCap,s.neuronCap,s.graphPool==1),WIDTH(1300*s.worldScale),HEIGHT(850*s.worldScale),GX(65*s.worldScale),GY(43*s.worldScale),NC(GX*GY),food(NC),detritus(NC),moisture(NC),pheromone(NC),baseMoisture(NC),temperature(NC),soil(NC),climateWave(NC),patchA(NC),patchB(NC),bins(NC),cellRegions(NC),regionalPhases(s.worldScale*s.worldScale),regionalGains(s.worldScale*s.worldScale),regionalThermal(s.worldScale*s.worldScale),migrationMatrix(size_t(s.worldScale*s.worldScale*s.worldScale*s.worldScale),0),transportDeltas(NC),signalDeltas(NC){if(c.threads>1)environmentalWorkers=make_unique<CellWorkers>(c.threads);for(int i=0;i<NC;i++){cellRegions[i]=(i%GX)/65+s.worldScale*((i/GX)/43);float u=(i%GX)/(float)GX,v=(i/GX)/(float)GY;moisture[i]=bound(.55f+.2f*sin(u*14)*cos(v*11)+rng.range(-.1f,.1f),.08f,1);baseMoisture[i]=moisture[i];soil[i]=bound(.33f+.55f*baseMoisture[i],.05f,1.f);temperature[i]=bound(c.climate+.21f*sin(u*9)*cos(v*7),0,1);
   // Static terrain coefficients and v9.0 climate coefficients are deterministic, RNG-free.
   climateWave[i]=sin(float(i%GX)/GX*9)*cos(float(i/GX)/GY*7);
   patchA[i]=sin(u*31.f+v*11.f)*cos(v*26.f-u*7.f);
   patchB[i]=sin(u*19.f-v*23.f)*cos(v*13.f+u*17.f);
   food[i]=rng.range(8,23)*moisture[i];}
 collisionReach.resize(c.cap);collisionReachReady.resize(c.cap);for(int j=0;j<c.cap;j++)available.push_back(c.cap-1-j);
 for(int j=0;j<c.initial;j++){Genome g=founder();create(move(g),0,0,rng.range(0,WIDTH),rng.range(0,HEIGHT));}}
 int zone(float x,float y)const{return min(c.worldScale-1,int(bound(x,0,float(WIDTH))/1300.f))+c.worldScale*min(c.worldScale-1,int(bound(y,0,float(HEIGHT))/850.f));}
 static float zoneNoise(uint64_t seed,int z,int salt){uint64_t x=seed^(uint64_t(z+1)*0x9e3779b97f4a7c15ULL)^(uint64_t(salt)*0xbf58476d1ce4e5b9ULL);x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;x^=x>>31;return float((x>>40)&0xffffff)/8388607.5f-1.f;}
 void updateRegionsClimate(){for(int r=0;r<c.worldScale*c.worldScale;r++){if(c.regionality==0)continue;float offset=zoneNoise(c.seed,r,31)*1.8f*c.regionality;regionalPhases[r]=sin(float(tick)*(.0008f+zoneNoise(c.seed,r,32)*.00018f*c.regionality)+offset);regionalGains[r]=bound(1.f+c.regionality*.31f*zoneNoise(c.seed,r,33),.35f,1.65f);regionalThermal[r]=c.regionality*.17f*zoneNoise(c.seed,r,34);}}
 bool expressed(int i,size_t b)const{
   auto const&g=p.genomes[i];if(b>=g.body.size())return false;if(b==0)return c.morphogenesis<=0.f?p.age[i]/210.f>=g.body[0].growth:true;
   auto const&n=g.body[b];float maturity=p.age[i]/210.f;
   if(c.morphogenesis<=0.f)return maturity>=n.growth;
   float nourishment=bound((food[index(p.x[i],p.y[i])]-18.f)/44.f,-.4f,.4f);
   float reserve=bound((p.energy[i]-25.f)/60.f,-.4f,.4f);
   float inherited=(n.motor-1.f)*.075f+g.niche*.09f+g.roleBias*.10f+(float(int(n.sense%3))-1.f)*.025f;
   float response=c.morphogenesis*(reserve*.22f+nourishment*.19f+inherited);
   return maturity+response>=n.growth && expressed(i,size_t(n.parent));
 }
 int index(float x,float y)const{return int(bound(y/HEIGHT*GY,0,GY-1))*GX+int(bound(x/WIDTH*GX,0,GX-1));}
 int ecologicalRole(Genome const&g)const{
  bool soilOrgan=false,detritusOrgan=false,rootOrgan=false,propeller=false;
  for(auto const&n:g.body){if(n.type==6)soilOrgan=true;if(n.type==5)detritusOrgan=true;if(n.type==7)rootOrgan=true;if(n.type==0)propeller=true;}
  if(c.ecology>0.f&&soilOrgan&&fabs(g.engineering)>.18f)return 3;
  if((c.ecology>0.f||c.scavenging>0.f)&&detritusOrgan&&g.niche<-.14f)return 1;
  if(rootOrgan||g.niche>.20f)return 0;
  if(propeller)return 2;
  return 4;
 }
 struct Capsule{float ax,ay,bx,by,r;};
 static float capsuleDistance(Capsule const&a,Capsule const&b,float&nx,float&ny){
   const float ux=a.bx-a.ax,uy=a.by-a.ay,vx=b.bx-b.ax,vy=b.by-b.ay,wx=a.ax-b.ax,wy=a.ay-b.ay;
   const float aa=ux*ux+uy*uy,bb=ux*vx+uy*vy,cc=vx*vx+vy*vy,dd=ux*wx+uy*wy,ee=vx*wx+vy*wy;
   float s=0,t=0,D=aa*cc-bb*bb;
   if(aa<1.e-10f&&cc<1.e-10f){s=t=0;}
   else if(aa<1.e-10f){s=0;t=bound(ee/cc,0.f,1.f);}
   else if(cc<1.e-10f){t=0;s=bound(-dd/aa,0.f,1.f);}
   else{float sn=D<1.e-8f?0.f:bb*ee-cc*dd,sd=D<1.e-8f?1.f:D,tn=D<1.e-8f?ee:aa*ee-bb*dd,td=D<1.e-8f?cc:D;
     if(sn<0){sn=0;tn=ee;td=cc;}else if(sn>sd){sn=sd;tn=ee+bb;td=cc;}
     if(tn<0){tn=0;sn=bound(-dd/aa,0.f,1.f);sd=1;}else if(tn>td){tn=td;sn=bound((bb-dd)/aa,0.f,1.f);sd=1;}
     s=sn/(fabs(sd)<1.e-8f?1.f:sd);t=tn/(fabs(td)<1.e-8f?1.f:td);
   }
   float dx=wx+s*ux-t*vx,dy=wy+s*uy-t*vy,d=hypot(dx,dy);
   if(d>1.e-6f){nx=-dx/d;ny=-dy/d;}else{nx=1;ny=0;}
   return d;
 }
 static float maxBodyReach(Genome const&g){float total=3.5f,maxR=3.5f;
   for(size_t j=1;j<g.body.size();j++){auto const &n=g.body[j];total+=max(0.f,n.length);maxR=max(maxR,n.thickness);}return total+maxR;
 }
 static bool capsuleBoundsOverlap(Capsule const&a,Capsule const&b){
   // Inflated AABBs are a conservative broad phase: false positives allowed,
   // false negatives impossible for true capsule intersections.
   const float ar=a.r,br=b.r;
   return !(max(a.ax,a.bx)+ar<min(b.ax,b.bx)-br || max(b.ax,b.bx)+br<min(a.ax,a.bx)-ar ||
       max(a.ay,a.by)+ar<min(b.ay,b.by)-br || max(b.ay,b.by)+br<min(a.ay,a.by)-ar);
 }
 int bodyCapsules(int i,array<Capsule,MAX_BOD>&a)const{return bodyCapsules(i,a.data());}
 int bodyCapsules(int i,Capsule* a)const{
    auto const &g=p.genomes[i];array<float,MAX_BOD>xx{},yy{},theta{};int count=0;
    a[count++]={p.x[i],p.y[i],p.x[i],p.y[i],3.5f};
    for(size_t k=1;k<g.body.size();k++){auto const&node=g.body[k];int parent=min<size_t>(node.parent,k-1);
      theta[k]=theta[parent]+node.angle+p.jointAt(i,k);
      bool on=expressed(i,k);
      xx[k]=xx[parent]+(on?cos(p.heading[i]+theta[k])*node.length:0.f);
      yy[k]=yy[parent]+(on?sin(p.heading[i]+theta[k])*node.length:0.f);
      if(!on)continue;
      const float strain=bound(1.f-.17f*fabs(p.jointAt(i,k))-.06f*fabs(p.velocityAt(i,k)),.62f,1.f);
      a[count++]={p.x[i]+xx[parent],p.y[i]+yy[parent],p.x[i]+xx[k],p.y[i]+yy[k],node.thickness*strain};
    }
    return count;
 }
 uint64_t signature(Genome const&g)const{uint64_t h=1469598103934665603ULL;auto mix=[&](uint64_t v){h^=v+1;h*=1099511628211ULL;};mix(g.neurons);mix(g.body.size());for(auto &n:g.body){mix(n.type);mix(n.parent);mix(n.ctrl);mix(n.sense);if(c.softBody>0.f){mix(uint64_t(n.thickness*100));mix(uint64_t(n.flexibility*100));}if(c.tissueCoupling>0.f){mix(uint64_t(n.conductance*100));mix(uint64_t(n.stiffness*100));}if(c.regulatoryNetwork>0.f){mix(n.regulator);mix(uint64_t((n.regulatorWeight+2.f)*100));}}mix(g.edges.size()/6);if(c.ecology>0.f){mix(uint64_t(int((g.niche+1.f)*12.f)));mix(uint64_t(int((g.engineering+1.f)*12.f)));}if(c.metabolicEcology>0.f){mix(uint64_t((g.assimilationBias+1.f)*1000.f));mix(uint64_t(g.transportGene*1000.f));}if(c.plasticity>0.f||c.grand>0.f)for(auto const&e:g.edges){mix(e.from);mix(e.to);}return h;}
 void ledgerMark(uint64_t a,uint64_t b,int type){
   if(!c.historyLedger||tick==0)return;
   auto mix=[&](uint64_t x){eventLedgerHash^=x;eventLedgerHash*=1099511628211ULL;};
   mix(tick);mix(a);mix(b);mix(uint64_t(type));eventLedgerCount++;
   if(eventLedgerLog)(*eventLedgerLog)<<tick<<','<<a<<','<<b<<','<<type<<','<<eventLedgerHash<<'\n';
 }
 Genome founder(){Genome g;g.neurons=3+rng.pick(5);if(c.founderNeurons>0)g.neurons=uint8_t(c.founderNeurons);g.metabolism=rng.range(.7,1.3);g.vision=rng.range(.7,1.4);g.threshold=rng.range(.85,1.2);g.signal=rng.range(.2,1);g.drag=rng.range(.7,1.3);g.tempOpt=rng.range(.15,.85);g.roleBias=bound((g.tempOpt-.5f)*2.f,-1.f,1.f);g.collectivity=bound(g.signal*.72f,.05f,1.f);if(c.ecology>0.f){g.niche=rng.range(-.9f,.9f);g.engineering=rng.range(-.6f,.6f);}if(c.metabolicEcology>0.f){g.assimilationBias=metabolismRng.range(-.95f,.95f);g.transportGene=metabolismRng.range(.25f,1.2f);}
  g.body.push_back({0,2,0,0,0,0,0,0});int size=3+rng.pick(5);for(int j=1;j<size;j++){Node n{(uint8_t)rng.pick(j),(uint8_t)(c.ecology>0?uint8_t(rng.pick(8)):uint8_t(rng.pick(5))),(uint8_t)rng.pick(4),(c.ecology>0?uint8_t(rng.pick(9)):uint8_t(rng.pick(6))),rng.range(3,14),rng.range(-PI,PI),rng.range(.05,.7),rng.range(.2,1.2)};
    if(c.softBody>0.f){n.thickness=rng.range(1.4f,5.4f);n.flexibility=rng.range(.22f,1.3f);}if(c.tissueCoupling>0.f){n.conductance=rng.range(.12f,1.0f);n.stiffness=rng.range(.15f,1.5f);}
     if(c.regulatoryNetwork>0.f){n.regulator=uint8_t(rng.pick(j));n.regulatorWeight=rng.range(-.5f,.5f);}
     g.body.push_back(n);}
  // Explicit non-default stress scenario for extended heritable graph capacity.
  // Nodes attach to an earlier node, so the developmental graph is acyclic.
  if(c.founderNodes>0)while(int(g.body.size())<c.founderNodes){
   int parent=rng.pick(int(g.body.size()));Node n{uint8_t(parent),uint8_t(rng.pick(c.ecology>0?8:5)),uint8_t(rng.pick(4)),uint8_t(rng.pick(c.ecology>0?9:6)),rng.range(2.f,11.f),rng.range(-PI,PI),rng.range(.02f,.6f),rng.range(.15f,1.2f)};
   if(c.softBody>0.f){n.thickness=rng.range(1.f,3.6f);n.flexibility=rng.range(.2f,1.2f);}
   if(c.tissueCoupling>0.f){n.conductance=rng.range(.12f,1.f);n.stiffness=rng.range(.15f,1.5f);}
   if(c.regulatoryNetwork>0.f){n.regulator=uint8_t(rng.pick(int(g.body.size())));n.regulatorWeight=rng.range(-.4f,.4f);}
   g.body.push_back(n);
  }
  for(int j=0;j<g.neurons*3+4;j++){g.edges.push_back({(uint8_t)rng.pick(6),(uint8_t)rng.pick(g.neurons),rng.range(-1,1)});}
  return g;}
 int create(Genome &&g,uint64_t root,int gen,float x,float y,uint64_t parentUID=0,bool propagated=false){if(available.empty())return -1;int i=available.back();available.pop_back();p.alive[i]=1;p.genomes[i]=move(g);p.x[i]=bound(x,0,WIDTH);p.y[i]=bound(y,0,HEIGHT);p.vx[i]=p.vy[i]=0;p.heading[i]=rng.range(-PI,PI);p.energy[i]=parentUID==0?rng.range(33,49):rng.range(18,25);p.age[i]=0;p.sig[i]=0;p.signalMemory[i]=0;p.generation[i]=gen;p.uid[i]=++nextUID;p.parentUid[i]=parentUID;ledgerMark(p.uid[i],parentUID,20);p.ancestor[i]=root==0?p.uid[i]:root;p.kind[i]=int(signature(p.genomes[i])%2147483647);p.offspring[i]=0;p.allocateLife(i,int(p.genomes[i].body.size()),int(p.genomes[i].neurons));if(c.metabolicEcology>0.f){p.cargoFood[i].assign(p.genomes[i].body.size(),0.f);p.cargoDetritus[i].assign(p.genomes[i].body.size(),0.f);p.flowFood[i].assign(p.genomes[i].body.size(),0.f);p.flowDetritus[i].assign(p.genomes[i].body.size(),0.f);}live.push_back(i);recordBirth(i,parentUID==0);maxGen=max(maxGen,gen);heritage[signature(p.genomes[i])]++;if(lineageLog)(*lineageLog)<<tick<<','<<p.uid[i]<<','<<p.parentUid[i]<<','<<p.ancestor[i]<<','<<gen<<','<<signature(p.genomes[i])<<','<<p.genomes[i].body.size()<<','<<int(p.genomes[i].neurons)<<','<<p.genomes[i].edges.size()<<','<<p.genomes[i].tempOpt<<','<<p.genomes[i].metabolism;if(lineageExtended&&lineageLog)(*lineageLog)<<','<<p.genomes[i].roleBias<<','<<p.genomes[i].collectivity<<','<<(propagated?1:0);if(lineageLog&&lineageEcologyExtended)(*lineageLog)<<','<<p.genomes[i].niche<<','<<p.genomes[i].engineering;if(lineageLog&&c.metabolicEcology>0.f)(*lineageLog)<<','<<p.genomes[i].assimilationBias<<','<<p.genomes[i].transportGene<<','<<count_if(p.genomes[i].body.begin(),p.genomes[i].body.end(),[](Node const&n){return n.type==2;})<<','<<count_if(p.genomes[i].body.begin(),p.genomes[i].body.end(),[](Node const&n){return n.type==5;})<<','<<count_if(p.genomes[i].body.begin(),p.genomes[i].body.end(),[](Node const&n){return n.type==7;});if(lineageLog)(*lineageLog)<<'\n';return i;}
 Genome mutate(Genome const&old){Genome g=old;auto chance=[&](float q){return rng.f()<q*c.mutation;};for(size_t nodeIndex=0;nodeIndex<g.body.size();nodeIndex++){auto &n=g.body[nodeIndex];if(chance(.3f))n.angle=bound(n.angle+rng.step()*.25f,-PI,PI);if(chance(.28f))n.length=bound(n.length+rng.step()*1.5f,1.5f,23);if(chance(.12f))n.motor=bound(n.motor+rng.step()*.1f,0,2);if(chance(.16f))n.ctrl=rng.pick(4);if(chance(.16f))n.sense=rng.pick(c.ecology>0?9:6);if(chance(.04f))n.type=rng.pick(c.ecology>0?8:5);
  if(c.softBody>0.f){if(chance(.26f*c.softBody))n.thickness=bound(n.thickness+rng.step()*.37f,.55f,8.0f);if(chance(.28f*c.softBody))n.flexibility=bound(n.flexibility+rng.step()*.12f,.06f,1.8f);}if(c.tissueCoupling>0.f){if(chance(.24f*c.tissueCoupling))n.conductance=bound(n.conductance+rng.step()*.11f,.04f,1.4f);if(chance(.25f*c.tissueCoupling))n.stiffness=bound(n.stiffness+rng.step()*.13f,.05f,2.f);}
 if(c.regulatoryNetwork>0.f){if(chance(.29f*c.regulatoryNetwork))n.regulator=uint8_t(rng.pick(max(1,int(nodeIndex))));if(chance(.33f*c.regulatoryNetwork))n.regulatorWeight=bound(n.regulatorWeight+rng.step()*.2f,-1.5f,1.5f);}}if(chance(.48f)&&g.body.size()<size_t(c.bodyCap)){auto par=rng.pick(g.body.size());Node nn{(uint8_t)par,(c.ecology>0?uint8_t(rng.pick(8)):uint8_t(rng.pick(5))),(uint8_t)rng.pick(4),(c.ecology>0?uint8_t(rng.pick(9)):uint8_t(rng.pick(6))),rng.range(3,15),rng.range(-PI,PI),rng.range(.05,.8),rng.range(.1,1.3)};if(c.softBody>0.f){nn.thickness=rng.range(1.4f,5.4f);nn.flexibility=rng.range(.22f,1.3f);}if(c.tissueCoupling>0.f){nn.conductance=rng.range(.12f,1.0f);nn.stiffness=rng.range(.15f,1.5f);}
 if(c.regulatoryNetwork>0.f){nn.regulator=uint8_t(rng.pick(int(g.body.size())));nn.regulatorWeight=rng.range(-.5f,.5f);}
 g.body.push_back(nn);structuralMutations++;}if(chance(.22f)&&g.body.size()>2){g.body.pop_back();for(auto &n:g.body){n.parent=uint8_t(min<int>(n.parent,g.body.size()-1));if(c.regulatoryNetwork>0.f)n.regulator=uint8_t(min<int>(n.regulator,g.body.size()-1));}structuralMutations++;}
 if(chance(.2f)&&g.neurons<c.neuronCap){g.neurons++;structuralMutations++;}if(chance(.2f)&&g.neurons>2){g.neurons--;g.edges.erase(remove_if(g.edges.begin(),g.edges.end(),[&](Edge const&e){return e.to>=g.neurons;}),g.edges.end());structuralMutations++;}
 for(auto &e:g.edges){if(chance(.35f))e.w=bound(e.w+rng.step()*.18f,-2,2);if(chance(.04f))e.from=rng.pick(6);if(chance(.07f))e.to=rng.pick(g.neurons);}
 if(chance(.55f)&&g.edges.size()<size_t(c.edgeCap)){g.edges.push_back({(uint8_t)rng.pick(6),(uint8_t)rng.pick(g.neurons),rng.range(-1,1)});structuralMutations++;}if(chance(.3f)&&g.edges.size()>2){g.edges.erase(g.edges.begin()+rng.pick(g.edges.size()));structuralMutations++;}
 // v8.8: Optional topology-level heritable variation; default OFF preserves v8.7 trajectories.
 if(c.plasticity>0.f){
   const float scale=c.plasticity;
   // Copy a connected subtree with internal parent references remapped to the new copy.
   if(chance(.70f*scale)&&g.body.size()>2&&g.body.size()<size_t(c.bodyCap)){
     int first=1+rng.pick(int(g.body.size())-1);
     size_t original=g.body.size();vector<int> chosen;chosen.push_back(first);
     for(int j=first+1;j<int(original);j++)if(find(chosen.begin(),chosen.end(),int(g.body[j].parent))!=chosen.end())chosen.push_back(j);
     if(g.body.size()+chosen.size()<=size_t(c.bodyCap)){
       vector<int> remap(original,-1);
       int attachment=rng.pick(int(original));
       for(int old:chosen){Node clone=g.body[old];clone.parent=uint8_t(old==first?attachment:remap[clone.parent]);
         clone.angle=bound(clone.angle+rng.step()*.35f,-PI,PI);
         if(rng.f()<.35f)clone.ctrl=uint8_t(rng.pick(g.neurons));
         remap[old]=int(g.body.size());g.body.push_back(clone);
       }
       subtreeCopies++;structuralMutations++;
     }
   }
   // Reattachment can alter developmental architecture without changing body mass.
   if(chance(.80f*scale)&&g.body.size()>3){int id=2+rng.pick(int(g.body.size())-2);
      uint8_t par=uint8_t(rng.pick(id));if(par!=g.body[id].parent){g.body[id].parent=par;reparentedBranches++;structuralMutations++;}}
   // Neurons may form a directed recurrent network (source 6..6+neurons-1).
   if(chance(.85f*scale)&&g.edges.size()<size_t(c.edgeCap)){
      g.edges.push_back({uint8_t(6+rng.pick(g.neurons)),uint8_t(rng.pick(g.neurons)),rng.range(-1.3f,1.3f)});
      feedbackMutations++;structuralMutations++;
   }
   if(chance(.42f*scale)&&!g.edges.empty()){
      auto &e=g.edges[rng.pick(int(g.edges.size()))];e.from=uint8_t(6+rng.pick(g.neurons));feedbackMutations++;structuralMutations++;
   }
   for(size_t j=1;j<g.body.size();j++){if(chance(.18f*scale))g.body[j].ctrl=uint8_t(rng.pick(g.neurons));if(g.body[j].ctrl>=g.neurons)g.body[j].ctrl=uint8_t(g.body[j].ctrl%g.neurons);}
   // Drop invalid recurrent references if the network size decreased this generation.
   g.edges.erase(remove_if(g.edges.begin(),g.edges.end(),[&](Edge const&e){return e.from>=6&&e.from<64&&e.from>=6+g.neurons;}),g.edges.end());
 }
 if(chance(.4f))g.metabolism=bound(g.metabolism+rng.step()*.13f,.35,2);if(c.metabolicEcology>0.f){if(metabolismRng.f()<.82f*c.metabolicEcology*c.mutation)g.assimilationBias=bound(g.assimilationBias+metabolismRng.step()*.22f,-1.f,1.f);if(metabolismRng.f()<.70f*c.metabolicEcology*c.mutation)g.transportGene=bound(g.transportGene+metabolismRng.step()*.12f,.12f,1.6f);}
 if(chance(.4f))g.vision=bound(g.vision+rng.step()*.12f,.3,2.5);
 if(chance(.3f))g.signal=bound(g.signal+rng.step()*.12f,0,2);
 if(chance(.4f))g.threshold=bound(g.threshold+rng.step()*.08f,.55,1.6);
 if(chance(.3f))g.drag=bound(g.drag+rng.step()*.12f,.4,2.3);
 // v10.3: non-parent regulatory link rewiring; only backward references permit acyclic propagation.
 if(c.regulatoryNetwork>0.f&&g.body.size()>2&&chance(.65f*c.regulatoryNetwork)){
   int at=2+rng.pick(int(g.body.size())-2);auto&node=g.body[size_t(at)];
   int source=rng.pick(at);if(node.regulator!=source){node.regulator=uint8_t(source);regulatoryRewires++;structuralMutations++;}
 }
 // v9.0 heritable gradient sensing: edge codes 67/68 denote forward and lateral resource gradients.
 if(c.grand>0.f&&g.edges.size()<size_t(c.edgeCap)&&chance(.55f*c.grand)){
   g.edges.push_back({uint8_t(67+rng.pick(2)),uint8_t(rng.pick(g.neurons)),rng.range(-1.f,1.f)});structuralMutations++;
 }
 // v8.9: conditional genome mutation for ecological niches and heterogeneous development.
 if(c.ecology>0.f){if(chance(.65f*c.ecology))g.niche=bound(g.niche+rng.step()*.22f,-1,1);if(chance(.48f*c.ecology))g.engineering=bound(g.engineering+rng.step()*.20f,-1,1);
   if(chance(.43f*c.ecology)&&g.edges.size()<size_t(c.edgeCap)){g.edges.push_back({uint8_t(64+rng.pick(3)),uint8_t(rng.pick(g.neurons)),rng.range(-1,1)});structuralMutations++;}}
 if(c.development>0.f){for(size_t idx=1;idx<g.body.size();idx++){auto &n=g.body[idx];if(chance(.50f*c.development)){n.growth=bound(n.growth+rng.step()*.20f,0,1.45f);developmentMutations++;}
    if(chance(.30f*c.development)){n.motor=bound(n.motor+rng.step()*.16f,.02f,2.f);developmentMutations++;}
    if(chance(.20f*c.development)){n.sense=uint8_t(rng.pick(c.ecology>0?9:6));developmentMutations++;}}}
 if(chance(.25f))g.tempOpt=bound(g.tempOpt+rng.step()*.06f,0,1);
 if(c.specialization>0&&c.ablate!="specialization"){if(chance(.55f))g.roleBias=bound(g.roleBias+rng.step()*.21f,-1,1);if(chance(.35f))g.collectivity=bound(g.collectivity+rng.step()*.15f,0,1);}
 mutations++;return g;}
 void event(uint64_t a,uint64_t b,int type){
  Interaction ev{tick,a,b,type,1};events.push_back(ev);
  ledgerMark(a,b,type);if(events.size()>260)events.erase(events.begin(),events.begin()+events.size()-260);
  // Longer-lived relationship observations are separate from the short event queue.
  uint64_t key=a*0x9e3779b97f4a7c15ULL ^ (b+0x517cc1b727220a95ULL)*0xbf58476d1ce4e5b9ULL ^ uint64_t(type)*0x94d049bb133111ebULL;
  auto it=relationships.find(key);if(it!=relationships.end()&&it->second.a==a&&it->second.b==b&&it->second.type==type){it->second.tick=tick;it->second.count++;}
  else relationships[key]=ev;
 }
 static uint64_t bondKey(uint64_t a,uint64_t b){if(a>b)swap(a,b);return (a*0x9e3779b97f4a7c15ULL)^(b+0x517cc1b727220a95ULL)*0xbf58476d1ce4e5b9ULL;}
 // Organization IDs are observational lineages, not species classifications.
 Bond makeBond(uint64_t a,uint64_t b,float rest,uint8_t origin,uint64_t parentGroup=0,uint32_t gen=0){
   Bond bond{min(a,b),max(a,b),rest,0,0.f,origin,++nextGroupId,parentGroup,gen};
   maxGroupGeneration=max(maxGroupGeneration,gen);
   if(organizationLog)(*organizationLog)<<tick<<','<<bond.groupId<<','<<parentGroup<<','<<gen<<','<<bond.a<<','<<bond.b<<','<<int(origin)<<'\n';
   return bond;
 }
 // Cohesion is expressed through inherited signal strength and inherited structural modules.
 float affinity(int i)const{float a=p.genomes[i].signal;int attach=0;for(auto const &node:p.genomes[i].body)if(node.type==3||node.type==4)attach++;float inherited=(c.specialization>0&&c.ablate!="specialization")?(.75f+.5f*p.genomes[i].collectivity):1.f;return bound(a*(.65f+.14f*min(4,attach))*inherited,.02f,1.5f);}
 void closeGroup(Bond const&b){
  if(b.offspringGroups)productiveOrganizations++;
  if(organizationFitnessLog)(*organizationFitnessLog)<<b.groupId<<','<<b.parentGroupId<<','<<b.groupGeneration<<','<<int(b.origin)<<','<<(tick-b.age)<<','<<tick<<','<<b.age<<','<<b.offspringGroups<<','<<b.transferred<<','<<b.coharvest<<','<<b.investment<<'\n';
 }
 void updateBonds(unordered_map<uint64_t,int>const&lookup){
  if(bonds.empty())return;
  vector<Bond>next;next.reserve(bonds.size());
  for(auto b:bonds){auto ai=lookup.find(b.a),bi=lookup.find(b.b);
   if(ai==lookup.end()||bi==lookup.end()){bondsBroken++;closeGroup(b);event(b.a,b.b,5);continue;}
   int i=ai->second,j=bi->second;float dx=p.x[j]-p.x[i],dy=p.y[j]-p.y[i],d=sqrt(dx*dx+dy*dy+1.e-6f);
   if(d>150||b.age>850||c.cohesion<=0||c.ablate=="organization") {bondsBroken++;closeGroup(b);event(b.a,b.b,5);continue;}
   float nx=dx/d,ny=dy/d;float rel=(p.vx[j]-p.vx[i])*nx+(p.vy[j]-p.vy[i])*ny;
   float spring=bound(.012f*(d-b.rest)+.07f*rel,-.37f,.37f);
   p.vx[i]+=spring*nx;p.vy[i]+=spring*ny;p.vx[j]-=spring*nx;p.vy[j]-=spring*ny;
   float roleA=p.genomes[i].roleBias,roleB=p.genomes[j].roleBias;
   float complementary=(c.specialization>0&&c.ablate!="specialization"&&roleA*roleB<0)?min(fabs(roleA),fabs(roleB)):0.f;
   float diff=p.energy[i]-p.energy[j];float delta=bound(diff*.003f*min(affinity(i),affinity(j))*(1.f+c.specialization*.45f*complementary), -.18f,.18f);
   if(delta>0){p.energy[i]-=delta;p.energy[j]+=delta*.93f;}
   else{p.energy[j]+=delta;p.energy[i]-=delta*.93f;}
   float spent=fabs(delta)*.07f+.004f+((c.specialization>0&&c.ablate!="specialization")?.003f*c.specialization*(p.genomes[i].collectivity+p.genomes[j].collectivity):0.f);
   p.energy[i]-=spent;p.energy[j]-=spent;
   // Resource-conserving complementary foraging; genetic role and collectivity mediate the effect.
   // Joint action uses actual cells and charges costs, not an unconditional fitness multiplier.
   if(c.integration>0&&c.ablate!="integration"&&c.ablate!="organization"&&c.ablate!="specialization"&&(tick%4)==0){
    float complement=(roleA*roleB<0)?min(fabs(roleA),fabs(roleB)):0.f;
    float effort=c.integration*c.specialization*complement*min(p.genomes[i].collectivity,p.genomes[j].collectivity);
    if(effort>0.02f){
     int fi=index(p.x[i],p.y[i]),fj=index(p.x[j],p.y[j]);
     float request=bound(effort*.54f,0.f,1.2f);
     float ha=min(food[fi],request*.5f),hb=min(food[fj],request*.5f);
     food[fi]-=ha;food[fj]-=hb;
     float gain=(ha+hb)*.81f;float cost=.025f*effort+.009f;
     p.energy[i]=min(75.f,p.energy[i]+gain*.5f-cost*.5f);
     p.energy[j]=min(75.f,p.energy[j]+gain*.5f-cost*.5f);
     integratedHarvest+=gain;integratedCost+=cost;integratedEvents++;
     b.coharvest+=gain;b.investment+=cost;
    }
   }
   bondedEnergy+=fabs(delta);b.age++;if(b.origin&&b.age==50)propagulesMatured++;b.transferred+=fabs(delta);next.push_back(b);
  }
  bonds.swap(next);
 }
 void grid(){
  // Only reset previously occupied bins, preserving per-cell organism order.
  // This keeps exact old neighbor selection while avoiding work on empty regions
  // and avoids an extra stamp lookup in every neural neighbor query.
  for(int cell:occupiedCells){bins[cell].clear();}
  occupiedCells.clear();
  for(int i:live)if(p.alive[i]){
    int cell=index(p.x[i],p.y[i]);
    if(bins[cell].empty())occupiedCells.push_back(cell);
    bins[cell].push_back(i);
  }
 }
 void transportEcology(){
  if(c.transport<=0.f)return;
  // Pairwise finite-volume exchange; reflecting (no-flow) world boundaries.
  // Every edge contributes equal and opposite flux. Per cell outflux <= 4*a*pool,
  // where a <= .10; no negative pools or invented nutrients.
  std::fill(transportDeltas.begin(),transportDeltas.end(),0.f);std::fill(signalDeltas.begin(),signalDeltas.end(),0.f);
  float a=.045f*c.transport, scent=.065f*c.transport;
  double positiveDetritusFlow=0;
  vector<int> regionActivity;
  if(c.quietFluxTolerance>0.f){
    regionActivity.assign(size_t(c.worldScale*c.worldScale),0);
    for(int id:live)if(p.alive[id])regionActivity[size_t(zone(p.x[id],p.y[id]))]++;
    for(int count:regionActivity)if(count<=c.quietActivityLimit)quietRegionSteps++;
  }
  for(int y=0;y<GY;y++)for(int x=0;x<GX;x++){
    int i=y*GX+x;
    auto exchange=[&](int j){
      if(c.compute==1&&detritus[i]==0.f&&detritus[j]==0.f&&pheromone[i]==0.f&&pheromone[j]==0.f){zeroTransportEdges++;return;}
      float d=a*(detritus[j]-detritus[i]);
      float signal=scent*(pheromone[j]-pheromone[i]);
      // Bounded *local transport omission*, NOT a bound on future nonlinear ecology.
      if(c.quietFluxTolerance>0.f&&regionActivity[size_t(cellRegions[i])]<=c.quietActivityLimit&&regionActivity[size_t(cellRegions[j])]<=c.quietActivityLimit&&fabs(d)<=c.quietFluxTolerance&&fabs(signal)<=c.quietFluxTolerance){
        skippedDetritusFlux+=fabs(double(d));skippedSignalFlux+=fabs(double(signal));skippedQuietEdges++;return;
      }
      transportDeltas[i]+=d;transportDeltas[j]-=d;
      positiveDetritusFlow+=fabs(double(d));
      if(c.worldScale>1){int tx0=(i%GX)/65,ty0=(i/GX)/43,tx1=(j%GX)/65,ty1=(j/GX)/43;
        if(tx0!=tx1||ty0!=ty1)crossRegionMatter+=fabs(double(d));}
      signalDeltas[i]+=signal;signalDeltas[j]-=signal;
    };
    if(x+1<GX)exchange(i+1);
    if(y+1<GY)exchange(i+GX);
  }
  for(int i=0;i<NC;i++){detritus[i]=max(0.f,detritus[i]+transportDeltas[i]);pheromone[i]=max(0.f,pheromone[i]+signalDeltas[i]);}
  transportedDetritus+=positiveDetritusFlow;transportEvents++;
 }

 void prepareCollisionReach(){
   if(c.collisionIndex==0||c.softBody<=0.f)return;
   // Dispatch is profitable only for genuinely large bodies; tiny organisms
   // use the unchanged direct narrow phase with no preparatory work.
   int complex=0;for(int id:live)if(p.alive[id]&&p.genomes[id].body.size()>=10)complex++;
   if(complex<24)return;
   fill(collisionReachReady.begin(),collisionReachReady.end(),0);
   auto job=[&](int from,int to){for(int k=from;k<to;k++){
       int id=live[size_t(k)];if(!p.alive[id])continue;
       collisionReach[size_t(id)]=maxBodyReach(p.genomes[id]);collisionReachReady[size_t(id)]=1;
   }};
   // The preparation reads immutable genome values and writes disjoint indexed slots.
   // Movement, neural control, collisions and all state mutations stay sequential.
   if(environmentalWorkers&&c.threads>1&&live.size()>=128)environmentalWorkers->each(int(live.size()),job);
   else job(0,int(live.size()));
 }
 float reachOf(int i){
   if(i<0||i>=p.cap)return 0;
   if(collisionReachReady[size_t(i)])return collisionReach[size_t(i)];
   float reach=maxBodyReach(p.genomes[i]);collisionReach[size_t(i)]=reach;collisionReachReady[size_t(i)]=1;return reach;
 }
 // Local subgrid exact neighbor candidates, no all-pairs scan.
 float habitatMultiplier(int i,float phaseCos,float phaseSin)const{
  if(c.habitats<=0.f)return 1.f;
  float field=.63f*patchA[i]*phaseCos+.37f*patchB[i]*phaseSin;
  return bound(1.f+.95f*c.habitats*field,.16f,2.8f);
 }
 // Exact deterministic region-task planner: cells belong to one region and are updated once.
 // This optimizes work distribution and exposes load information; it NEVER skips ecology.
 void regionalEnvironment(float patchCos,float patchSin,float phase){
   const int zones=c.worldScale*c.worldScale;
   vector<double> mineralMass(size_t(zones),0.0);
   vector<uint64_t> mineralCells(size_t(zones),0);
   auto job=[&](int begin,int end){
     for(int z=begin;z<end;z++){
       const int rx=z%c.worldScale,ry=z/c.worldScale;
       double localMatter=0;uint64_t localMineral=0;
       for(int iy=ry*43;iy<(ry+1)*43;iy++)for(int ix=rx*65;ix<(rx+1)*65;ix++){
         int i=iy*GX+ix;
         float resource=food[i],m=moisture[i];float localPhase=c.regionality>0.f?regionalPhases[z]:phase;
         moisture[i]=bound(baseMoisture[i]+c.season*.35f*localPhase,.04f,1.f);
         temperature[i]=c.habitats>0.f
           ?bound(c.climate+.21f*climateWave[i]+c.season*.22f*localPhase,0,1)
           :bound(c.climate+.21f*sin(float(i%GX)/GX*9)*cos(float(i/GX)/GY*7)+c.season*.22f*localPhase,0,1);
         if(c.regionality>0.f)temperature[i]=bound(temperature[i]+regionalThermal[z],0.f,1.f);
         if(c.ecology>0.f){
           float decay=detritus[i]*.007f,converted=detritus[i]*.014f*c.ecology;
           detritus[i]-=min(detritus[i],decay+converted);
           soil[i]=bound(soil[i]+converted*.003f+(.56f-soil[i])*.00008f,.07f,1.f);
           food[i]=c.habitats>0.f
             ?min(44.f,resource+.45f*c.resourceRate*m*habitatMultiplier(i,patchCos,patchSin)*(.55f+.65f*soil[i])*(1-resource/44.f)+converted*.70f)
             :min(44.f,resource+.45f*c.resourceRate*m*(.55f+.65f*soil[i])*(1-resource/44.f)+converted*.70f);
           localMatter+=converted;localMineral+=converted>0.0001f;
         }else{
           food[i]=c.habitats>0.f
             ?min(44.f,resource+.45f*c.resourceRate*m*habitatMultiplier(i,patchCos,patchSin)*(1-resource/44.f)+detritus[i]*.014f)
             :min(44.f,resource+.45f*c.resourceRate*m*(1-resource/44.f)+detritus[i]*.014f);
           detritus[i]*=.993f;
         }
         if(c.regionality>0.f)food[i]=bound(food[i]+.45f*c.resourceRate*m*(regionalGains[z]-1.f)*(1-resource/44.f),0.f,44.f);
         pheromone[i]*=.91f;
       }
       mineralMass[size_t(z)]=localMatter;mineralCells[size_t(z)]=localMineral;
     }
   };
   if(environmentalWorkers&&zones>1)environmentalWorkers->each(zones,job);
   else job(0,zones);
   for(int z=0;z<zones;z++){recycledMatter+=mineralMass[size_t(z)];soilMineralizations+=mineralCells[size_t(z)];}
 }
  // v10.3 all-individual, all-expressed-capsule broadphase and deterministic contact commit.
  // Complete for capsule intersections at the END-OF-TICK snapshot, not continuous-time CCD.
  // The narrowphase and mutable impulse submission are always ordered by stable slot pair.
  void allBodyContacts(){
   if(c.ablate=="interaction"||c.softBody<=0.f)return;
   constexpr int TILE=48;
   const int cols=(WIDTH+TILE-1)/TILE, rows=(HEIGHT+TILE-1)/TILE;
   struct BodyCache{vector<Capsule> legacy;Capsule* ref=nullptr;int size=0;};
   vector<BodyCache> cache(live.size());
   vector<Capsule> compactShapes;
   if(c.contactCache){size_t total=0;for(int id:live)if(p.alive[id])total+=p.genomes[id].body.size();compactShapes.resize(total);size_t offset=0;
      for(size_t k=0;k<live.size();k++)if(p.alive[live[k]]){cache[k].ref=compactShapes.data()+offset;offset+=p.genomes[live[k]].body.size();}
   }
   auto prepare=[&](int from,int to){for(int k=from;k<to;k++){
     const int id=live[size_t(k)];if(p.alive[id]){auto&b=cache[size_t(k)];if(!c.contactCache){b.legacy.resize(MAX_BOD);b.ref=b.legacy.data();}b.size=bodyCapsules(id,b.ref);}
   }};
   if(environmentalWorkers&&c.lifeParallel==1&&live.size()>=128)environmentalWorkers->each(int(live.size()),prepare);
   else prepare(0,int(live.size()));
   contactCacheSlots=c.contactCache?compactShapes.size():size_t(live.size())*size_t(MAX_BOD);
   vector<vector<int>> tiles;if(c.sparseBuckets==0)tiles.resize(size_t(cols)*size_t(rows));
   vector<uint64_t> flatOccurrences;if(c.sparseBuckets==2)flatOccurrences.reserve(live.size()*12);
   unordered_map<int,vector<int>> activeTiles;if(c.sparseBuckets==1)activeTiles.reserve(max(size_t(128),live.size()*4));
   for(size_t k=0;k<live.size();k++){
     if(!cache[k].size)continue;
     for(int a=0;a<cache[k].size;a++){
       auto const&part=cache[k].ref[a];
       int x0=clamp(int(floor((min(part.ax,part.bx)-part.r)/TILE)),0,cols-1);
       int x1=clamp(int(floor((max(part.ax,part.bx)+part.r)/TILE)),0,cols-1);
       int y0=clamp(int(floor((min(part.ay,part.by)-part.r)/TILE)),0,rows-1);
       int y1=clamp(int(floor((max(part.ay,part.by)+part.r)/TILE)),0,rows-1);
       for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
         if(c.sparseBuckets==2){flatOccurrences.push_back((uint64_t(uint32_t(y*cols+x))<<32)|uint32_t(k));continue;}
         auto &tile=c.sparseBuckets==1?activeTiles[y*cols+x]:tiles[size_t(y*cols+x)];
         if(c.contactDedupe==1&&!tile.empty()&&tile.back()==int(k)){tileDuplicatePrunes++;continue;}
         tile.push_back(int(k));
       }
     }
   }
   vector<uint64_t> pairs;
   unordered_set<uint64_t> uniquePairs;if(c.contactDedupe==1)uniquePairs.reserve(live.size()*12+16);
   unordered_set<uint64_t> linked;linked.reserve(bonds.size()*2+1);
   unordered_map<uint64_t,int> degree;for(auto const &b:bonds){linked.insert(bondKey(b.a,b.b));degree[b.a]++;degree[b.b]++;}
   vector<int> keys;if(c.sparseBuckets==1){keys.reserve(activeTiles.size());for(auto const&v:activeTiles)keys.push_back(v.first);sort(keys.begin(),keys.end());}
   for(size_t ti=0;ti<(c.sparseBuckets==1?keys.size():(c.sparseBuckets==2?size_t(0):tiles.size()));ti++){
     auto &tile=c.sparseBuckets==1?activeTiles.at(keys[ti]):tiles[ti];if(tile.size()<2)continue;
     if(c.contactDedupe!=1){sort(tile.begin(),tile.end());tile.erase(unique(tile.begin(),tile.end()),tile.end());}
     for(size_t a=0;a<tile.size();a++)for(size_t b=a+1;b<tile.size();b++)
       {uint64_t key=(uint64_t(uint32_t(tile[a]))<<32)|uint32_t(tile[b]);
         if(c.contactDedupe==1&&!uniquePairs.insert(key).second){dedupeAvoided++;continue;}
         if(pairs.size()>=8000000ULL)throw runtime_error("Full-body candidate budget exceeded; lower population density or enlarge the world");pairs.push_back(key);}
   }
   if(c.sparseBuckets==2){
     sort(flatOccurrences.begin(),flatOccurrences.end());
     auto v=unique(flatOccurrences.begin(),flatOccurrences.end());tileDuplicatePrunes+=uint64_t(flatOccurrences.end()-v);flatOccurrences.erase(v,flatOccurrences.end());
     for(size_t first=0;first<flatOccurrences.size();){
       size_t end=first+1;uint64_t tile=flatOccurrences[first]>>32;
       while(end<flatOccurrences.size()&&(flatOccurrences[end]>>32)==tile)end++;
       for(size_t a=first;a<end;a++)for(size_t b=a+1;b<end;b++){
         uint64_t key=(uint64_t(uint32_t(flatOccurrences[a]))<<32)|uint32_t(flatOccurrences[b]);
         if(pairs.size()>=16000000ULL)throw runtime_error("Flat sparse candidate budget exceeded");
         pairs.push_back(key);
       }first=end;
     }
   }
   sort(pairs.begin(),pairs.end());
   auto uniqueEnd=unique(pairs.begin(),pairs.end());
   if(c.sparseBuckets==2)dedupeAvoided+=uint64_t(pairs.end()-uniqueEnd);
   pairs.erase(uniqueEnd,pairs.end());
   globalCandidatePairs+=pairs.size();
   // Parallel READ-ONLY exact narrowphase over independent candidate slots.
   // No RNG, energy, motion or event state may be written by workers.
   struct ContactHit{float penetration=0.f,nx=1.f,ny=0.f;uint32_t exact=0;};
   vector<ContactHit> hits(pairs.size());
   auto narrow=[&](int from,int to){
     for(int k=from;k<to;k++){
       const uint64_t key=pairs[size_t(k)];int ai=int(key>>32),bi=int(uint32_t(key));
       int i=live[size_t(ai)],j=live[size_t(bi)];if(!p.alive[i]||!p.alive[j])continue;
       auto const&aa=cache[size_t(ai)],&bb=cache[size_t(bi)];auto &hit=hits[size_t(k)];
       for(int a=0;a<aa.size;a++)for(int b=0;b<bb.size;b++){
         auto const&u=aa.ref[a],&v=bb.ref[b];
         if(max(u.ax,u.bx)+u.r<min(v.ax,v.bx)-v.r||max(v.ax,v.bx)+v.r<min(u.ax,u.bx)-u.r||
            max(u.ay,u.by)+u.r<min(v.ay,v.by)-v.r||max(v.ay,v.by)+v.r<min(u.ay,u.by)-u.r)continue;
         hit.exact++;
         float xx=0.f,yy=0.f,d=capsuleDistance(u,v,xx,yy),overlap=u.r+v.r-d;
         if(overlap>hit.penetration){hit.penetration=overlap;hit.nx=xx;hit.ny=yy;}
       }
     }
   };
   if(environmentalWorkers&&c.lifeParallel==1&&pairs.size()>=384)environmentalWorkers->each(int(pairs.size()),narrow);
   else narrow(0,int(pairs.size()));
   // Deterministic order and exact accumulated totals, independent of worker count.
   for(size_t k=0;k<pairs.size();k++){
     const uint64_t key=pairs[k];int ai=int(key>>32),bi=int(uint32_t(key)),i=live[size_t(ai)],j=live[size_t(bi)];
     if(!p.alive[i]||!p.alive[j])continue;
     auto const&hit=hits[k];globalExactPairs+=hit.exact;
     const float penetration=hit.penetration,nx=hit.nx,ny=hit.ny;
     if(penetration<=0.f)continue;
     globalCollisionPairs++;contacts++;surfaceContacts++;
     const float correction=min(penetration,12.f)*.35f;
     p.x[i]=bound(p.x[i]-nx*correction,0.f,float(WIDTH));p.y[i]=bound(p.y[i]-ny*correction,0.f,float(HEIGHT));
     p.x[j]=bound(p.x[j]+nx*correction,0.f,float(WIDTH));p.y[j]=bound(p.y[j]+ny*correction,0.f,float(HEIGHT));
     float vn=(p.vx[j]-p.vx[i])*nx+(p.vy[j]-p.vy[i])*ny;
     if(vn<0.f){float impulse=-.32f*vn;p.vx[i]-=impulse*nx;p.vy[i]-=impulse*ny;p.vx[j]+=impulse*nx;p.vy[j]+=impulse*ny;}
     // Actual material contact, not fictitious proximity; small genotype-conditioned exchange.
     if(c.predation>0.f&&p.genomes[i].roleBias<-.25f&&p.energy[j]>7.f){float take=min(.20f*c.predation,p.energy[j]*.006f);p.energy[j]-=take;p.energy[i]+=take*.72f;predations++;event(p.uid[i],p.uid[j],2);}
     else if(c.sharing>0.f&&p.ancestor[i]==p.ancestor[j]&&p.energy[i]>p.energy[j]+5.f){float gift=min(.18f*c.sharing,(p.energy[i]-p.energy[j])*.01f);p.energy[i]-=gift;p.energy[j]+=gift*.95f;shares++;event(p.uid[i],p.uid[j],3);}
     if(c.cohesion>0.f&&c.ablate!="organization"&&bonds.size()<size_t(c.cap)&&p.energy[i]>10&&p.energy[j]>10&&degree[p.uid[i]]<2&&degree[p.uid[j]]<2){
       uint64_t bk=bondKey(p.uid[i],p.uid[j]);float affinityScore=min(affinity(i),affinity(j));
       if(!linked.count(bk)&&rng.f()<.045f*c.cohesion*affinityScore){
         bonds.push_back(makeBond(p.uid[i],p.uid[j],max(1.f,hypot(p.x[j]-p.x[i],p.y[j]-p.y[i])),0));
         linked.insert(bk);degree[p.uid[i]]++;degree[p.uid[j]]++;bondsFormed++;event(p.uid[i],p.uid[j],4);
       }
     }
   }
  }
 void evolve(){recordExposure();tick++;functionalUptake={0.,0.,0.};anchorVelocityLoss=0.;if((tick&3)==0){
   const float patchPhase=float(tick)*.00021f;
   const float patchCos=cos(patchPhase),patchSin=sin(patchPhase);
   const float phase=sin(float(tick)*.0008f);if(c.regionality>0.f)updateRegionsClimate();
   if(c.scheduler==1){regionalEnvironment(patchCos,patchSin,phase);}
   else if(c.threads==1||!environmentalWorkers){for(int i=0;i<NC;i++){float resource=food[i],m=moisture[i];float localPhase=c.regionality>0.f?regionalPhases[cellRegions[i]]:phase;moisture[i]=bound(baseMoisture[i]+c.season*.35f*localPhase,.04f,1.f);temperature[i]=c.habitats>0.f
      ?bound(c.climate+.21f*climateWave[i]+c.season*.22f*localPhase,0,1)
      :bound(c.climate+.21f*sin(float(i%GX)/GX*9)*cos(float(i/GX)/GY*7)+c.season*.22f*localPhase,0,1);if(c.regionality>0.f)temperature[i]=bound(temperature[i]+regionalThermal[cellRegions[i]],0.f,1.f);if(c.ecology>0.f){
   // Detritus conversion draws from its finite pool; habitat condition is a bounded quality index, not matter.
   float decay=detritus[i]*.007f,converted=detritus[i]*.014f*c.ecology;
   detritus[i]-=min(detritus[i],decay+converted);
   soil[i]=bound(soil[i]+converted*.003f+(.56f-soil[i])*.00008f,.07f,1.f);
   food[i]=c.habitats>0.f
      ?min(44.f,resource+.45f*c.resourceRate*m*habitatMultiplier(i,patchCos,patchSin)*(.55f+.65f*soil[i])*(1-resource/44.f)+converted*.70f)
      :min(44.f,resource+.45f*c.resourceRate*m*(.55f+.65f*soil[i])*(1-resource/44.f)+converted*.70f);
   recycledMatter+=converted;soilMineralizations+=converted>0.0001f;
  }else{food[i]=c.habitats>0.f
      ?min(44.f,resource+.45f*c.resourceRate*m*habitatMultiplier(i,patchCos,patchSin)*(1-resource/44.f)+detritus[i]*.014f)
      :min(44.f,resource+.45f*c.resourceRate*m*(1-resource/44.f)+detritus[i]*.014f);detritus[i]*=.993f;}
  if(c.regionality>0.f)food[i]=bound(food[i]+.45f*c.resourceRate*m*(regionalGains[cellRegions[i]]-1.f)*(1-resource/44.f),0.f,44.f);
  pheromone[i]*=.91f;}}
   else {
    vector<double> recycled(size_t(c.threads),0.0);vector<uint64_t> mineral(size_t(c.threads),0);
    // Each cell is read/written by one worker. Scalar totals are reduced in fixed order.
    auto fn=[&](int from,int to){int threadId=(from*c.threads+NC-1)/NC;double localMatter=0;uint64_t localMineral=0;
      for(int i=from;i<to;i++){
        float resource=food[i],m=moisture[i];float localPhase=c.regionality>0.f?regionalPhases[cellRegions[i]]:phase;
        moisture[i]=bound(baseMoisture[i]+c.season*.35f*localPhase,.04f,1.f);
        temperature[i]=c.habitats>0.f
      ?bound(c.climate+.21f*climateWave[i]+c.season*.22f*localPhase,0,1)
      :bound(c.climate+.21f*sin(float(i%GX)/GX*9)*cos(float(i/GX)/GY*7)+c.season*.22f*localPhase,0,1);
        if(c.regionality>0.f)temperature[i]=bound(temperature[i]+regionalThermal[cellRegions[i]],0.f,1.f);
        if(c.ecology>0.f){float decay=detritus[i]*.007f,converted=detritus[i]*.014f*c.ecology;
          detritus[i]-=min(detritus[i],decay+converted);
          soil[i]=bound(soil[i]+converted*.003f+(.56f-soil[i])*.00008f,.07f,1.f);
          food[i]=c.habitats>0.f
      ?min(44.f,resource+.45f*c.resourceRate*m*habitatMultiplier(i,patchCos,patchSin)*(.55f+.65f*soil[i])*(1-resource/44.f)+converted*.70f)
      :min(44.f,resource+.45f*c.resourceRate*m*(.55f+.65f*soil[i])*(1-resource/44.f)+converted*.70f);
          localMatter+=converted;localMineral+=converted>0.0001f;
        }else{food[i]=c.habitats>0.f
      ?min(44.f,resource+.45f*c.resourceRate*m*habitatMultiplier(i,patchCos,patchSin)*(1-resource/44.f)+detritus[i]*.014f)
      :min(44.f,resource+.45f*c.resourceRate*m*(1-resource/44.f)+detritus[i]*.014f);detritus[i]*=.993f;}
        if(c.regionality>0.f)food[i]=bound(food[i]+.45f*c.resourceRate*m*(regionalGains[cellRegions[i]]-1.f)*(1-resource/44.f),0.f,44.f);
        pheromone[i]*=.91f;
      }recycled[threadId]=localMatter;mineral[threadId]=localMineral;
    };
    environmentalWorkers->each(NC,fn);
    for(int k=0;k<c.threads;k++){recycledMatter+=recycled[k];soilMineralizations+=mineral[k];}
   }
  }
  transportEcology();
 // Reuse one stable-ID lookup for bonds and collaboration; avoid rebuilding it twice each tick.
 unordered_map<uint64_t,int> livingUID;livingUID.reserve(live.size()*2+1);
 for(int j:live)if(p.alive[j])livingUID[p.uid[j]]=j;
 updateBonds(livingUID);grid();struct Spawn{Genome genome;uint64_t root,parentUID;int gen;float x,y;int pair=-1;};vector<Spawn>pending;pending.reserve(100);
 unordered_set<uint64_t> bonded;bonded.reserve(bonds.size()*2+1);unordered_map<uint64_t,int> bondDegree;
 unordered_map<uint64_t,int> protector;protector.reserve(bonds.size()*2+1);
 for(auto const &b:bonds){bonded.insert(bondKey(b.a,b.b));bondDegree[b.a]++;bondDegree[b.b]++;
   auto ia=livingUID.find(b.a),ib=livingUID.find(b.b);if(ia==livingUID.end()||ib==livingUID.end())continue;
   auto consider=[&](uint64_t victim,int guard){auto prev=protector.find(victim);if(prev==protector.end()||p.genomes[guard].roleBias<p.genomes[prev->second].roleBias)protector[victim]=guard;};
   consider(b.a,ib->second);consider(b.b,ia->second);
 }
 // v9.7: parallel read-only body preparation; no shared state writes, RNG or environment edits.
 // Contact, sensing of mutable resources and reproduction remain in their original serial order.
 const float motorPhase=float(tick)*.13f;
 const bool parallelCandidate=c.lifeParallel==1&&c.threads>1&&live.size()>=128;
 prepareCollisionReach();
 // Small bodies are cheaper sequentially. Never claim threads provide a speedup unless
 // the observed organism workload is sufficiently complex to amortize dispatch costs.
 size_t parts=0;
 if(parallelCandidate)for(int id:live)if(p.alive[id])parts+=p.genomes[id].body.size();
 const bool preparingParallel=parallelCandidate&&parts>=live.size()*7;
 auto &prepared=preparedBody;
 if(preparingParallel){
   if(prepared.size()<live.size())prepared.resize(live.size());
   environmentalWorkers->each(int(live.size()),[&](int from,int to){
     for(int k=from;k<to;k++){
       int id=live[size_t(k)];if(!p.alive[id])continue;
       auto const& genome=p.genomes[id];auto &target=prepared[size_t(k)];target.sensorBoost=0.f;
       float maturity=p.age[id]/210.f;
       for(size_t part=0;part<genome.body.size();part++){
         const auto &node=genome.body[part];target.waves[part]=sin(motorPhase+node.angle);
         if(node.type==1&&(c.morphogenesis>0.f?expressed(id,part):maturity>=node.growth))target.sensorBoost+=.12f*node.motor;
       }
     }
   });
 }
 for(size_t liveOrdinal=0;liveOrdinal<live.size();liveOrdinal++){
 int i=live[liveOrdinal];if(!p.alive[i])continue;Genome &g=p.genomes[i];int cell=index(p.x[i],p.y[i]);int cy=cell/GX,cx=cell%GX;
   float nearestDX=0,nearestDY=0;int nearest=-1;float best=160*160;for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){int yy=cy+dy,xx=cx+dx;if(xx<0||xx>=GX||yy<0||yy>=GY)continue;for(int j:bins[yy*GX+xx]){if(j==i||!p.alive[j])continue;float vx=p.x[j]-p.x[i],vy=p.y[j]-p.y[i],d=vx*vx+vy*vy;if(d<best){best=d;nearest=j;nearestDX=vx;nearestDY=vy;}}}
   float sensorBoost=0;if(preparingParallel)sensorBoost=prepared[liveOrdinal].sensorBoost;else for(size_t b=0;b<g.body.size();b++){auto const&node=g.body[b];if(node.type==1&&expressed(i,b))sensorBoost+=.12f*node.motor;}
   array<float,11>sensor{bound(food[cell]/30.f,0,1),bound(p.energy[i]/40.f,0,2),bound((nearest<0?0:1-sqrt(best)/(160.f*g.vision*(1+sensorBoost))),0,1),bound(pheromone[cell]/20.f,0,1),bound(p.signalMemory[i],0,1),1,0,0,0,0,0};
   if(c.grand>0.f){
     auto resourceSignal=[&](float theta){int at=index(bound(p.x[i]+cos(theta)*31.f,0.f,float(WIDTH)),bound(p.y[i]+sin(theta)*31.f,0.f,float(HEIGHT)));
       return food[at]+detritus[at]*(.28f-.14f*g.niche);};
     float front=resourceSignal(p.heading[i]);float left=resourceSignal(p.heading[i]+.7f),right=resourceSignal(p.heading[i]-.7f);
     sensor[9]=bound((front-food[cell])/22.f,-1.f,1.f);
     sensor[10]=bound((left-right)/22.f,-1.f,1.f);
   }
   if(c.ecology>0.f){sensor[6]=bound(detritus[cell]/12.f,0,2);sensor[7]=soil[cell];sensor[8]=temperature[cell]-g.tempOpt;}
   // Detritus availability is an additional stimulus to scavenger-prone inherited phenotypes.
   // It reuses the existing nutrient sensor rather than inventing an unexpressed extra neural channel.
   if(c.scavenging>0){float scavenger=max(0.f,-g.roleBias);sensor[0]=bound(sensor[0]+c.scavenging*.22f*scavenger*detritus[cell]/12.f,0.f,1.f);}
   // Expressed appendages inject low-amplitude inheritable proprioception into their coded sensors.
   array<float,MAX_BOD> motorWaves{};
   if(c.compute==1){
     for(size_t b=0;b<g.body.size();b++){
       auto const &node=g.body[b];
       if(expressed(i,b)){
         float wave=preparingParallel?prepared[liveOrdinal].waves[b]:sin(motorPhase+node.angle);
         motorWaves[b]=wave;
         sensor[node.sense]=bound(sensor[node.sense]+.055f*node.motor*wave,-1,2);
       }
     }
   }else for(size_t b=0;b<g.body.size();b++){auto const&node=g.body[b];if(expressed(i,b))sensor[node.sense]=bound(sensor[node.sense]+.055f*node.motor*(preparingParallel?prepared[liveOrdinal].waves[b]:sin(motorPhase+node.angle)),-1,2);}
   array<float,4>out{};
   if(c.plasticity>0.f){
     array<float,MAX_NEU>previous{},accum{};
     for(int h=0;h<g.neurons;h++)previous[h]=p.neuronAt(i,h);
     for(auto const&e:g.edges){if(e.to>=g.neurons)continue;
       float input=e.from<6?sensor[e.from]:(e.from<6+g.neurons?previous[e.from-6]:(c.ecology>0&&e.from>=64&&e.from<67?sensor[6+e.from-64]:(c.grand>0.f&&e.from>=67&&e.from<69?sensor[9+e.from-67]:0.f)));
       accum[e.to]+=input*e.w;
     }
     for(int h=0;h<g.neurons;h++)p.neuronAt(i,h)=bound(previous[h]*.70f+tanh(accum[h])*.30f,-3.f,3.f);
   }else{
     for(auto const&e:g.edges){if((e.from>=6&&!(c.ecology>0&&e.from>=64&&e.from<67)&&!(c.grand>0.f&&e.from>=67&&e.from<69))||e.to>=g.neurons)continue;float target=p.neuronAt(i,e.to);float input=e.from<6?sensor[e.from]:(e.from<67?sensor[6+e.from-64]:sensor[9+e.from-67]);target=target*.62f+input*e.w*.38f;p.neuronAt(i,e.to)=bound(target,-3,3);}
   }
   for(int h=0;h<g.neurons;h++)out[h%4]+=tanh(p.neuronAt(i,h));
   for(float &v:out)v=tanh(v);
   // Explicit joint state: restoring spring torque, damping and inherited neural drive.
   float thrust=0,turn=0,absorber=0,shield=0,secretion=0,bodyLength=0,digester=0,soilTool=0,rootTool=0;
   array<float,MAX_BOD>jointAngles{},tipX{},tipY{};
   for(size_t b=1;b<g.body.size();b++){
      auto &n=g.body[b];float &q=p.jointAt(i,b),&omega=p.velocityAt(i,b);
      float neuralDrive=(c.ablate=="motor"?0.f:(n.ctrl<4?out[n.ctrl]:(c.plasticity>0&&n.ctrl<g.neurons?tanh(p.neuronAt(i,n.ctrl)):out[n.ctrl%4])));
      float wave=preparingParallel?prepared[liveOrdinal].waves[b]:((c.compute==1&&expressed(i,b))?motorWaves[b]:sin(motorPhase+n.angle));
      if(c.compute==1&&expressed(i,b))reusedOscillations++;
      float restoring=c.softBody>0.f?(.09f*bound(n.flexibility,.06f,1.8f)):.09f;
      float damping=c.softBody>0.f?(.17f*(1.65f-.35f*bound(n.flexibility,.06f,1.8f))):.17f;
      float torque=.035f*n.motor*neuralDrive+.019f*n.motor*wave-restoring*q-damping*omega;
      if((c.tissueCoupling>0.f||c.regulatoryNetwork>0.f)&&expressed(i,b)){
        const int parent=int(n.parent);
        float parentSignal=parent==0?out[0]:p.signalAt(i,parent);
        float &mySignal=p.signalAt(i,b);
        float lateral=0.f;
        if(c.regulatoryNetwork>0.f&&b>1){int source=min<int>(n.regulator,int(b)-1);lateral=c.regulatoryNetwork*n.regulatorWeight*(source==0?out[0]:p.signalAt(i,source));}
        mySignal=bound(.74f*mySignal+.26f*n.conductance*(.60f*parentSignal+.40f*neuralDrive+lateral),-1.f,1.f);
        float parentAngle=parent==0?0.f:p.jointAt(i,parent);
        torque+=(c.tissueCoupling+.25f*c.regulatoryNetwork)*(.025f*mySignal*n.motor-.011f*n.stiffness*(q-parentAngle));
        p.energy[i]-=.00028f*(c.tissueCoupling+.2f*c.regulatoryNetwork)*n.conductance;
      }
      omega=bound(omega+torque,-.45f,.45f);q=bound(q+omega,-1.1f,1.1f);
      if(q<=-1.1f||q>=1.1f)omega*=.45f;
      jointAngles[b]=jointAngles[n.parent]+n.angle+q;bodyLength+=n.length;
      if(c.grand>0.f||c.softBody>0.f||c.metabolicEcology>0.f){tipX[b]=tipX[n.parent]+cos(p.heading[i]+jointAngles[b])*n.length;tipY[b]=tipY[n.parent]+sin(p.heading[i]+jointAngles[b])*n.length;}
      if(!expressed(i,b))continue;
      float drive=(c.ablate=="motor"?0:n.motor*omega);
      float axis=jointAngles[b];
      if(n.type==0){thrust+=drive*.48f*cos(axis)*n.length/10.f;turn+=drive*.028f*sin(axis)*n.length/10.f;}
      else if(n.type==2)absorber+=.10f*n.motor;
      else if(n.type==3)shield+=.10f*n.motor;
      else if(n.type==4)secretion+=.08f*n.motor;
      else if(n.type==5)digester+=.08f*n.motor;
      else if(n.type==6)soilTool+=.06f*n.motor;
      else if(n.type==7)rootTool+=.08f*n.motor;
   }
   p.heading[i]+=bound(out[1]*.07f+turn,-.2f,.2f);float desired=.28f+thrust+out[0]*.1f;
   int previousRegion=c.worldScale>1?(min(c.worldScale-1,int(p.x[i]/1300.f))+c.worldScale*min(c.worldScale-1,int(p.y[i]/850.f))):0;
   float inertia=bound(.72f+.14f/g.drag,.68f,.95f);p.vx[i]=bound((p.vx[i]*inertia+cos(p.heading[i])*desired),-2.8f,2.8f);p.vy[i]=bound((p.vy[i]*inertia+sin(p.heading[i])*desired),-2.8f,2.8f);
   // v10.8: expressed, inherited root geometry mechanically drags against substrate.
   // No categorical "root species" behavior: force depends on actual contact area and soil.
   // Anchoring reduces dispersal; only real nutrient cells can compensate for this cost.
   if(c.metabolicEcology>0.f&&c.organAblation!="root"){
     float rootContact=0.f;
     for(size_t b=1;b<g.body.size();b++){auto const&n=g.body[b];if(n.type==7&&expressed(i,b))rootContact+=bound(n.thickness*n.length/35.f,0.f,2.5f);}
     if(rootContact>0.f){float before=hypot(p.vx[i],p.vy[i]);float traction=1.f/(1.f+.32f*c.metabolicEcology*rootContact*(.3f+.7f*soil[cell]));p.vx[i]*=traction;p.vy[i]*=traction;anchorVelocityLoss+=before-hypot(p.vx[i],p.vy[i]);}
   }
   // v10.11 controlled environmental ecology: the same inherited genome can face
   // different resource flow, border dispersal cost and spatial corridors.
   // These are world-level forces, NEVER preassigned organism strategies.
   float candidateX=bound(p.x[i]+p.vx[i],0,WIDTH),candidateY=bound(p.y[i]+p.vy[i],0,HEIGHT);
   if(c.worldScale>1&&zone(candidateX,candidateY)!=previousRegion){
     int oldX=min(c.worldScale-1,int(p.x[i]/1300.f)),newX=min(c.worldScale-1,int(candidateX/1300.f));
     int oldY=min(c.worldScale-1,int(p.y[i]/850.f)),newY=min(c.worldScale-1,int(candidateY/850.f));
     float halfGateX=650.f*(1.f-c.experimentalIsolation),halfGateY=425.f*(1.f-c.experimentalIsolation);
     bool blocked=(oldX!=newX&&fabs(fmod(candidateY,850.f)-425.f)>halfGateY)
              ||(oldY!=newY&&fabs(fmod(candidateX,1300.f)-650.f)>halfGateX);
     if(blocked){
       // Reflect only the blocked component, retain within the old spatial region.
       if(oldX!=newX){candidateX=p.x[i];p.vx[i]*=-.4f;}
       if(oldY!=newY){candidateY=p.y[i];p.vy[i]*=-.4f;}
       blockedCrossings++;
     }else if(c.experimentalMigrationCost>0.f){
       double paid=min(double(max(0.f,p.energy[i])),double(c.experimentalMigrationCost));
       p.energy[i]-=float(paid);migrationEnergyPaid+=paid;
     }
   }
   p.x[i]=candidateX;p.y[i]=candidateY;
   if(c.worldScale>1){int region=zone(p.x[i],p.y[i]);if(region!=previousRegion){regionalMigrants++;migrationMatrix[size_t(previousRegion)*size_t(c.worldScale*c.worldScale)+size_t(region)]++;recentCrossings.push_back({tick,p.uid[i],p.ancestor[i],previousRegion,region,p.generation[i]});if(recentCrossings.size()>96)recentCrossings.erase(recentCrossings.begin());}}
   int newCell=index(p.x[i],p.y[i]);
   // v9.0: inherited appendage placement creates spatially extended foraging.
   // Nothing is spawned or rewarded for visual complexity; harvested matter comes from actual grid cells.
   if(c.grand>0.f&&c.ecology>0.f){for(size_t b=1;b<g.body.size();b++){
     auto const&n=g.body[b];if(!expressed(i,b)||!(n.type==2||n.type==5||n.type==7))continue;
     int at=index(bound(p.x[i]+tipX[b],0.f,float(WIDTH)),bound(p.y[i]+tipY[b],0.f,float(HEIGHT)));
     if(at==newCell)continue; // extra reach only when extending into a distinct resource cell
     float amount=bound(.12f*c.grand*n.motor*(.5f+.5f*n.length/15.f),0.f,.5f);
     if(n.type==5){float take=min(detritus[at],amount*max(.08f,.6f-.3f*g.niche));take=min(take,max(0.f,(75.f-p.energy[i])/.63f));
       detritus[at]-=take;p.energy[i]+=take*.63f;extendedHarvests+=take>.0001f;extendedEnergy+=take*.63f;}
     else {float take=min(food[at],amount*max(.15f,1.f+.3f*g.niche));take=min(take,max(0.f,75.f-p.energy[i]));
       food[at]-=take;p.energy[i]+=take;extendedHarvests+=take>.0001f;extendedEnergy+=take;}
     p.energy[i]-=.0028f*c.grand*n.length/12.f;
   }}
   // Heritable finite-width collector membrane. The food is removed from actual remote cells;
   // this affects energy, competition, reproduction and natural selection, not appearance only.
   if(c.softBody>0.f){for(size_t b=1;b<g.body.size();b++){
      auto const &node=g.body[b];if(!expressed(i,b))continue;
      p.energy[i]-=c.softBody*.00025f*node.length*node.thickness;
      if(c.ecology<=0.f||organDisabled(node.type)||!(node.type==2||node.type==5||node.type==7))continue;
      int at=index(bound(p.x[i]+tipX[b]*.5f,0.f,float(WIDTH)),bound(p.y[i]+tipY[b]*.5f,0.f,float(HEIGHT)));
      if(at==newCell)continue;
      float area=bound(c.softBody*node.thickness*node.length*.0009f,0.f,.21f);
      if(node.type==5){float grab=min(detritus[at],min(area,max(0.f,(75.f-p.energy[i])/.63f)));detritus[at]-=grab;p.energy[i]+=grab*.63f;membraneHarvest+=grab*.63f;}
      else {float grab=min(food[at],min(area,max(0.f,75.f-p.energy[i])));food[at]-=grab;p.energy[i]+=grab;membraneHarvest+=grab;}
   }}
   // v10.7: inheritable resource-allocation strategy and per-body internal substrate transport.
   // Each uptake removes material from a real grid cell. Cargo moves child -> parent along
   // actual inherited connections and is converted into energy only at the core.
   if(c.metabolicEcology>0.f){
     auto &F=p.cargoFood[i],&D=p.cargoDetritus[i];
     if(F.size()!=g.body.size()||D.size()!=g.body.size()||p.flowFood[i].size()!=g.body.size()||p.flowDetritus[i].size()!=g.body.size())throw runtime_error("Invalid living cargo structure");
     fill(p.flowFood[i].begin(),p.flowFood[i].end(),0.f);fill(p.flowDetritus[i].begin(),p.flowDetritus[i].end(),0.f);
     const float bias=g.assimilationBias, strength=c.metabolicEcology;
     for(size_t b=1;b<g.body.size();b++){
       const Node &node=g.body[b];if(!expressed(i,b))continue;
       if(node.type!=2&&node.type!=5&&node.type!=7)continue;
       if(organDisabled(node.type))continue;
       int at=index(bound(p.x[i]+tipX[b],0.f,float(WIDTH)),bound(p.y[i]+tipY[b],0.f,float(HEIGHT)));
       if(node.type==5){
         float capacity=max(0.f,2.f-D[b]);
         float take=min(capacity,min(detritus[at],strength*(.055f+.08f*node.motor)*(.95f-.48f*bias)));
         if(take>0.f){detritus[at]-=take;D[b]+=take;detritusCargoUptake+=take;functionalUptake[1]+=take;}
       }else{
         float capacity=max(0.f,2.f-F[b]);
         // Root tips trade locomotion for contact and uptake capacity. Advantage requires
         // actual available food; neither substrate nor stored energy is conjured.
         float rootedGain=node.type==7?(1.f+.55f*soil[at]*bound(1.f-hypot(p.vx[i],p.vy[i])*.32f,0.f,1.f)):1.f;
         float take=min(capacity,min(food[at],strength*(.055f+.08f*node.motor)*(.95f+.48f*bias)*rootedGain));
         if(take>0.f){food[at]-=take;F[b]+=take;foodCargoUptake+=take;functionalUptake[node.type==7?2:0]+=take;}
       }
     }
     // Reverse topological traversal guarantees that only physically connected descendants
     // can deliver cargo. Transport capacity depends on inherited conductance and transport gene.
     for(int b=int(g.body.size())-1;b>0;b--){
       const Node &node=g.body[size_t(b)];if(!expressed(i,size_t(b))||!expressed(i,size_t(node.parent)))continue;
       int parent=int(node.parent);
       // Wider conductive tubes move more matter; longer paths impose real transport loss.
       float conductanceArea=bound(node.thickness/2.4f,.5f,1.7f);
       float lengthResistance=1.f/(1.f+.024f*node.length);
       float rate=strength*(.12f+.18f*bound(node.conductance,.04f,1.4f))*g.transportGene*conductanceArea*lengthResistance;
       float moveF=min(F[size_t(b)],min(rate,max(0.f,4.f-F[size_t(parent)])));
       F[size_t(b)]-=moveF;F[size_t(parent)]+=moveF;p.flowFood[i][size_t(b)]=moveF;
       float moveD=min(D[size_t(b)],min(rate,max(0.f,4.f-D[size_t(parent)])));
       D[size_t(b)]-=moveD;D[size_t(parent)]+=moveD;p.flowDetritus[i][size_t(b)]=moveD;
       p.energy[i]-=.0012f*(moveF+moveD)*strength; // Active transport is metabolically costly.
     }
     // Energy conversions have opposing hereditary yields: food versus detritus efficiency.
     // The difference is dissipated, not returned to the finite nutrient pools as false mass.
     float foodRate=min(F[0],min(strength*.45f,max(0.f,(75.f-p.energy[i])/(.70f+.15f*bias))));
     float detRate=min(D[0],min(strength*.30f,max(0.f,(75.f-p.energy[i]-foodRate*(.70f+.15f*bias))/(.56f-.18f*bias))));
     float foodYield=.70f+.15f*bias,detYield=.56f-.18f*bias;
     F[0]-=foodRate;D[0]-=detRate;
     float edible=foodRate*foodYield+detRate*detYield;
     p.energy[i]=min(75.f,p.energy[i]+edible);
     foodCargoConverted+=foodRate;detritusCargoConverted+=detRate;
     metabolicDissipation+=(foodRate+detRate)-edible;
     // Cost of maintaining the inherited network; prevents a free additional food source.
     p.energy[i]-=strength*(.003f+.001f*g.body.size()+.0025f*g.transportGene);
   }
   float specialty=(c.specialization>0&&c.ablate!="specialization")?c.specialization:0.f;
   float gather= max(0.f,g.roleBias)*specialty;
   float habitat=(c.ecology>0.f?soil[newCell]:.5f);
   float foodAffinity=(c.ecology>0.f?bound(1.f+.28f*g.niche+.22f*rootTool*habitat,.55f,1.55f):1.f);
   float gain=min(food[newCell],(.56f+absorber)*g.metabolism*(1.f+.28f*gather)*foodAffinity);food[newCell]-=gain;p.energy[i]=min(75.f,p.energy[i]+gain);if(c.ecology>0)soil[newCell]=bound(soil[newCell]-gain*.00019f*c.ecology,.07f,1.f);
   // Optional detritivore niche: inherited defensive role bias + absorber organ act together.
   // Matter comes out of the actual finite detritus pool; conversion is lossy.
   // Disabled by default for v8.6 bitwise trajectory compatibility.
   if((c.scavenging>0||c.ecology>0) && detritus[newCell]>0.02f && p.energy[i]<74.f){
     float scavenger=max(0.f,-g.roleBias)*max(.05f,absorber)+(c.ecology>0.f?max(0.f,-g.niche)*(.12f+digester):0.f);
     float uptake=min(detritus[newCell], (c.scavenging+.35f*c.ecology)*scavenger*.075f);
     if(uptake>0.0001f){
       detritus[newCell]-=uptake;
       float actual=min(75.f-p.energy[i],uptake*.68f);
       p.energy[i]+=actual;
       p.energy[i]-=.0018f*(c.scavenging+.35f*c.ecology);
       scavenges++;scavengedEnergy+=actual;if(c.ecology>0.f){nicheHarvests++;ecologicalHarvest+=actual;}
     }
   }
   // Organism-driven habitat modification, with a metabolic cost and bounded local persistence.
   float engineerCost=0.f;
   if(c.ecology>0.f&&soilTool>0&&fabs(g.engineering)>.03f){
     float delta=bound(c.ecology*soilTool*g.engineering*.0008f,-.0016f,.0016f);
     soil[newCell]=bound(soil[newCell]+delta,.07f,1.f);
     engineerCost=.002f+fabs(delta)*3.5f;soilEdits++;}
   float mismatch=fabs(temperature[newCell]-g.tempOpt);float eCost=(.10f+g.body.size()*.009f+g.neurons*.004f+g.edges.size()*.0008f+bodyLength*.00008f+fabs(thrust)*.04f+shield*.025f+gather*.035f+specialty*max(0.f,-g.roleBias)*.020f+(c.ablate=="climate"?0.f:.13f*mismatch)+engineerCost)/g.metabolism;p.energy[i]-=eCost;p.age[i]++;
   if((c.allContacts==0||c.softBody<=0.f)&&c.ablate!="interaction"&&nearest>=0&&best<(c.softBody>0.f?10000.f:1500.f)){int j=nearest;float d=sqrt(max(1.f,best));float radius=7.f+.10f*(g.body.size()+p.genomes[j].body.size());
      float nx=0,ny=0;bool surfaceContact=false;
      if(c.softBody>0.f){
         // No expressed point can exceed the sum of inherited segment lengths.
         // The capsule radii are bounded by inherited thickness.
         const bool heavy=c.collisionIndex==1&&g.body.size()*p.genomes[j].body.size()>=96;
         bool maybeTouch=!heavy||d<=reachOf(i)+reachOf(j);
         if(!maybeTouch&&heavy)conservativeRejects++;
         if(maybeTouch){
         array<Capsule,MAX_BOD> aa{},bb{};int na=bodyCapsules(i,aa.data()),nb=bodyCapsules(j,bb.data());
         // Deterministic per-part spatial bounding index. Build each capsule AABB
         // once, then test only intersecting candidates in original ia/ib order.
         struct Box4{float minx,maxx,miny,maxy;};
         array<Box4,MAX_BOD> aBounds{},bBounds{};
         bool indexed=heavy&&na*nb>=96;
         if(indexed){auto makeBounds=[](Capsule const&c)->Box4{return {min(c.ax,c.bx)-c.r,max(c.ax,c.bx)+c.r,min(c.ay,c.by)-c.r,max(c.ay,c.by)+c.r};};
           for(int m=0;m<na;m++)aBounds[m]=makeBounds(aa[m]);
           for(int m=0;m<nb;m++)bBounds[m]=makeBounds(bb[m]);}
         float penetration=0.f;for(int ia=0;ia<na;ia++)for(int ib=0;ib<nb;ib++){
           if(indexed&&(aBounds[ia].maxx<bBounds[ib].minx||bBounds[ib].maxx<aBounds[ia].minx||aBounds[ia].maxy<bBounds[ib].miny||bBounds[ib].maxy<aBounds[ia].miny)){broadphaseRejects++;continue;}
           if(c.collisionIndex==1)narrowphaseTests++;
           float xx=0,yy=0,dist=capsuleDistance(aa[ia],bb[ib],xx,yy),overlap=aa[ia].r+bb[ib].r-dist;
           if(overlap>penetration){penetration=overlap;nx=xx;ny=yy;}
         }
         if(penetration>0.f){surfaceContact=true;radius=d+penetration;}
         }
      }
      if(c.softBody>0.f?surfaceContact:d<radius){contacts++;if(surfaceContact)surfaceContacts++;if(c.softBody<=0.f){float otherAngle=atan2(nearestDY,nearestDX);nx=cos(otherAngle);ny=sin(otherAngle);}
       // Symmetric overlap correction and damped normal relative velocity.
       float overlap=radius-d;p.x[i]=bound(p.x[i]-nx*overlap*.35f,0,WIDTH);p.x[j]=bound(p.x[j]+nx*overlap*.35f,0,WIDTH);
       p.y[i]=bound(p.y[i]-ny*overlap*.35f,0,HEIGHT);p.y[j]=bound(p.y[j]+ny*overlap*.35f,0,HEIGHT);
       float vn=(p.vx[j]-p.vx[i])*nx+(p.vy[j]-p.vy[i])*ny;
       if(vn<0){float impulse=-.32f*vn;p.vx[i]-=impulse*nx;p.vy[i]-=impulse*ny;p.vx[j]+=impulse*nx;p.vy[j]+=impulse*ny;}
       if(out[2]>.3f&&p.energy[i]>p.energy[j]+10.f&&c.sharing>0){float flow=min(1.f,c.sharing*(p.energy[i]-p.energy[j])*.025f);p.energy[i]-=flow;p.energy[j]+=flow*.9f;exchanges++;event(p.uid[i],p.uid[j],1);}
       else if(out[2]<-.22f&&p.energy[j]>6.f&&c.predation>0){float flow=min(1.2f,c.predation*.35f*(1+fabs(out[2])));
        // Protective specialization is only useful when the target has a living bond.
        float protectedTarget=(bondDegree[p.uid[j]]>0&&specialty>0)?max(0.f,-p.genomes[j].roleBias)*specialty:0.f;
        flow*=max(.22f,1.f-.38f*protectedTarget);
        // A linked defensive partner may reduce predation at a real energetic cost.
        // This does NOT guarantee cooperation improves inclusive reproductive fitness.
        if(c.cooperation>0&&c.ablate!="cooperation"&&c.ablate!="organization"&&specialty>0){
          auto guard=protector.find(p.uid[j]);
          if(guard!=protector.end()&&guard->second!=i&&p.alive[guard->second]&&p.energy[guard->second]>4.f){
            int k=guard->second;float defense=max(0.f,-p.genomes[k].roleBias)*c.cooperation*specialty;
            float discount=bound(defense*.32f,0.f,.55f);
            if(discount>0){float blocked=flow*discount;float guardCost=blocked*.34f+.006f;
              flow-=blocked;p.energy[k]-=guardCost;cooperativeDefenses++;preventedLoss+=blocked;
              event(p.uid[k],p.uid[j],7);
            }
          }
        }
        p.energy[j]-=flow;p.energy[i]+=flow*.72f;predations++;event(p.uid[i],p.uid[j],2);}
       else if(out[2]>.2f&&p.ancestor[i]==p.ancestor[j]&&c.sharing>0){float gift=min(.7f,c.sharing*.25f);p.energy[i]-=gift;p.energy[j]+=gift*.95f;shares++;event(p.uid[i],p.uid[j],3);}
       // Form persistent but reversible connections. Neither multicellularity nor cooperative advantage is assumed.
       if(c.cohesion>0&&c.ablate!="organization"&&bonds.size()<size_t(c.cap)&&p.energy[i]>10&&p.energy[j]>10&&bondDegree[p.uid[i]]<2&&bondDegree[p.uid[j]]<2){
         uint64_t key=bondKey(p.uid[i],p.uid[j]);float a=min(affinity(i),affinity(j));
         if(!bonded.count(key)&&rng.f()<.045f*c.cohesion*a){
           bonds.push_back(makeBond(p.uid[i],p.uid[j],max(radius,d),0));
           bonded.insert(key);bondDegree[p.uid[i]]++;bondDegree[p.uid[j]]++;bondsFormed++;event(p.uid[i],p.uid[j],4);
         }
       }
      }
   }
   if(c.ablate!="signal"&&secretion>0){float sent=(secretion+max(0.f,out[3])*.2f)*g.signal;pheromone[newCell]+=sent;p.energy[i]-=sent*.05f;p.signalMemory[i]=sent;signals++;}else p.signalMemory[i]=0;
   if((c.neutral||p.energy[i]>35.f*g.threshold)&&p.age[i]>26&&pending.size()<available.size()){float prob=c.neutral?.022f:.04f; if(rng.f()<prob){Genome child=mutate(g);pending.push_back({move(child),p.ancestor[i],p.uid[i],p.generation[i]+1,p.x[i]+rng.range(-8,8),p.y[i]+rng.range(-8,8),-1});p.energy[i]-=18;p.offspring[i]++;recordReproduction(i);births++;}}
   float maxAge=c.neutral?170.f:bound(190.f+28.f*g.vision-5.f*g.body.size(),100.f,300.f);if(c.neutral?p.age[i]>maxAge:(p.energy[i]<=0||p.age[i]>maxAge)){detritus[newCell]+=max(0.f,p.energy[i])*.3f;
       if(c.metabolicEcology>0.f){float rem=0.f;for(float v:p.cargoFood[i])rem+=v;for(float v:p.cargoDetritus[i])rem+=v;detritus[newCell]+=rem;cargoReturnedOnDeath+=rem;fill(p.cargoFood[i].begin(),p.cargoFood[i].end(),0.f);fill(p.cargoDetritus[i].begin(),p.cargoDetritus[i].end(),0.f);}
       recordDeath(i);p.alive[i]=0;deaths++;ledgerMark(p.uid[i],0,21);available.push_back(i);}}
 if(c.allContacts==1)allBodyContacts();
 live.erase(remove_if(live.begin(),live.end(),[&](int i){return !p.alive[i];}),live.end());
 // A bonded pair may bud a two-member propagule, retaining both independently inheritable genomes.
 // Formation and reproduction remain contingent: this is NOT hard-coded multicellularity.
 int pairs=0;vector<pair<uint64_t,uint32_t>> parentGroups;
 if(c.specialization>0&&c.ablate!="specialization"&&c.ablate!="organization"&&c.cohesion>0&&pending.size()+2<=available.size()){
   unordered_map<uint64_t,int>byID;byID.reserve(live.size()*2+1);for(int id:live)byID[p.uid[id]]=id;
   for(auto &b:bonds){if(pending.size()+2>available.size()||pairs>=24)break;
     auto ia=byID.find(b.a),ib=byID.find(b.b);if(ia==byID.end()||ib==byID.end()||b.age<22)continue;
     int i=ia->second,j=ib->second;auto &a=p.genomes[i];auto &z=p.genomes[j];
     if(p.age[i]<25||p.age[j]<25||p.energy[i]<32||p.energy[j]<32)continue;
     float complement=(a.roleBias*z.roleBias<0)?min(fabs(a.roleBias),fabs(z.roleBias)):0.f;
     float probability=.060f*c.specialization*min(a.collectivity,z.collectivity)*(1.f+.6f*complement);
     if(rng.f()>=probability)continue;
     int pairing=pairs++;parentGroups.push_back({b.groupId,b.groupGeneration});b.offspringGroups++;
     Genome ga=mutate(a),gb=mutate(z);
     if(c.regulatoryNetwork>0.f){
       // Genuine donor-recipient subtree inheritance during bonded-pair reproduction.
       auto cross=[&](Genome &recipient,Genome const&donor){
         if(donor.body.size()<2||recipient.body.empty()||rng.f()>=.35f*c.regulatoryNetwork)return;
         const int root=1+rng.pick(int(donor.body.size())-1);vector<int> chosen{root};
         for(int j=root+1;j<int(donor.body.size());j++)if(find(chosen.begin(),chosen.end(),int(donor.body[j].parent))!=chosen.end())chosen.push_back(j);
         if(recipient.body.size()+chosen.size()>size_t(c.bodyCap))return;
         vector<int> remap(donor.body.size(),-1);const int attach=rng.pick(int(recipient.body.size()));
         for(int from:chosen){Node next=donor.body[from];next.parent=uint8_t(from==root?attach:remap[next.parent]);
           // Donor regulatory references outside the copied subtree reset to a viable local root.
           int src=int(next.regulator);next.regulator=uint8_t(src>=0&&src<int(remap.size())&&remap[size_t(src)]>=0?remap[size_t(src)]:0);
           remap[size_t(from)]=int(recipient.body.size());recipient.body.push_back(next);
         }
         crossParentRecombinations++;structuralMutations++;
       };
       cross(ga,z);cross(gb,a);
     }
     float ox=(p.x[i]+p.x[j])*.5f,oy=(p.y[i]+p.y[j])*.5f;
     pending.push_back({move(ga),p.ancestor[i],p.uid[i],p.generation[i]+1,ox-5,oy,pairing});
     pending.push_back({move(gb),p.ancestor[j],p.uid[j],p.generation[j]+1,ox+5,oy,pairing});
     p.energy[i]-=23;p.energy[j]-=23;p.offspring[i]++;p.offspring[j]++;recordReproduction(i);recordReproduction(j);births+=2;
   }
 }
 // Births commit after the active iteration; connect newborn partners only if both were born.
 vector<array<int,2>> pairChildren(pairs,{{-1,-1}});
 for(auto &birth:pending){int child=create(move(birth.genome),birth.root,birth.gen,birth.x,birth.y,birth.parentUID,birth.pair>=0);
   if(birth.pair>=0&&child>=0){auto &slot=pairChildren[birth.pair];if(slot[0]<0)slot[0]=child;else slot[1]=child;}}
 for(size_t k=0;k<pairChildren.size();k++){auto const &pair=pairChildren[k];if(pair[0]<0||pair[1]<0)continue;auto a=p.uid[pair[0]],b=p.uid[pair[1]];
   uint64_t parentGroup=parentGroups[k].first;uint32_t gen=parentGroups[k].second+1;
   bonds.push_back(makeBond(a,b,10.f,1,parentGroup,gen));bondsFormed++;propagulesBorn++;
   if(gen>=2){descendantGroupBirths++;}event(a,b,6);
 }
 if((tick%64)==0){for(auto it=relationships.begin();it!=relationships.end();){if(tick-it->second.tick>750)it=relationships.erase(it);else ++it;}if(relationships.size()>2500){vector<pair<uint64_t,uint64_t>> order;order.reserve(relationships.size());for(auto &e:relationships)order.push_back({e.second.tick,e.first});sort(order.begin(),order.end());for(size_t j=0;j<order.size()-2500;j++)relationships.erase(order[j].second);}}
 if((tick%4000)==0)events.clear();
 }
 // For exact resumption: preserve all hot arrays, dormant slots, cold genomes, RNG, ecology and slot order.
 void checkpoint(string const&path)const{
  if(path.empty())return;
  ostringstream out(ios::out|ios::binary);uint64_t magic=0x45564f4c41423831ULL;put(out,magic);put(out,uint32_t(23));put(out,uint8_t(p.compact?1:0));
  put(out,c.seed);put(out,c.initial);put(out,c.cap);put(out,c.mutation);put(out,c.neutral);
  put(out,c.resourceRate);put(out,c.season);put(out,c.climate);put(out,c.predation);put(out,c.sharing);
  uint8_t abl=c.ablate=="motor"?1:c.ablate=="signal"?2:c.ablate=="climate"?3:c.ablate=="interaction"?4:c.ablate=="organization"?5:c.ablate=="cooperation"?6:c.ablate=="integration"?7:0;put(out,abl);
  put(out,rng.state);put(out,tick);put(out,births);put(out,deaths);put(out,contacts);put(out,exchanges);put(out,signals);put(out,predations);put(out,shares);put(out,mutations);put(out,structuralMutations);put(out,nextUID);put(out,maxGen);
  put(out,c.worldScale);
  putarray(out,food);putarray(out,detritus);putarray(out,moisture);putarray(out,pheromone);putarray(out,baseMoisture);putarray(out,temperature);
  putvec(out,live);putvec(out,available);
  putvec(out,p.x);putvec(out,p.y);putvec(out,p.vx);putvec(out,p.vy);putvec(out,p.heading);putvec(out,p.energy);putvec(out,p.age);putvec(out,p.sig);if(p.compact){for(int i=0;i<p.cap;i++)putvec(out,p.neuralSlabs[i]);}else putvec(out,p.neural);
  putvec(out,p.signalMemory);
  if(p.compact){for(int i=0;i<p.cap;i++){putvec(out,p.angleSlabs[i]);putvec(out,p.velocitySlabs[i]);putvec(out,p.signalSlabs[i]);}}
  else{putvec(out,p.jointAngle);putvec(out,p.jointVelocity);putvec(out,p.tissueSignal);}
  putvec(out,p.generation);putvec(out,p.alive);putvec(out,p.kind);putvec(out,p.offspring);putvec(out,p.ancestor);putvec(out,p.uid);putvec(out,p.parentUid);
  for(int j=0;j<p.cap;j++){auto const&g=p.genomes[j];put(out,g.neurons);put(out,g.metabolism);put(out,g.vision);put(out,g.threshold);put(out,g.signal);put(out,g.drag);put(out,g.tempOpt);putvec(out,g.body);putvec(out,g.edges);put(out,g.roleBias);put(out,g.collectivity);}
  vector<pair<uint64_t,int>> hs(heritage.begin(),heritage.end());sort(hs.begin(),hs.end());put(out,uint64_t(hs.size()));for(auto const &h:hs){put(out,h.first);put(out,h.second);}
  putvec(out,events);vector<pair<uint64_t,Interaction>> r(relationships.begin(),relationships.end());sort(r.begin(),r.end(),[](auto const&a,auto const&b){return a.first<b.first;});put(out,uint64_t(r.size()));for(auto const&v:r){put(out,v.first);put(out,v.second);}
  put(out,c.cohesion);put(out,bondsFormed);put(out,bondsBroken);put(out,bondedEnergy);putvec(out,bonds);put(out,c.specialization);put(out,propagulesBorn);put(out,propagulesMatured);put(out,nextGroupId);put(out,descendantGroupBirths);put(out,maxGroupGeneration);put(out,c.cooperation);put(out,cooperativeDefenses);put(out,preventedLoss);put(out,c.integration);put(out,integratedHarvest);put(out,integratedCost);put(out,integratedEvents);put(out,productiveOrganizations);put(out,c.scavenging);put(out,scavenges);put(out,scavengedEnergy);put(out,c.plasticity);put(out,subtreeCopies);put(out,reparentedBranches);put(out,feedbackMutations);
  put(out,c.ecology);put(out,c.development);putarray(out,soil);
  for(int j=0;j<p.cap;j++){put(out,p.genomes[j].niche);put(out,p.genomes[j].engineering);}
  put(out,soilEdits);put(out,soilMineralizations);put(out,nicheHarvests);put(out,developmentMutations);put(out,recycledMatter);put(out,ecologicalHarvest);
  put(out,c.grand);put(out,extendedHarvests);put(out,extendedEnergy);put(out,c.habitats);put(out,c.transport);put(out,transportEvents);put(out,transportedDetritus);put(out,regionalMigrants);put(out,crossRegionMatter);put(out,c.regionality);putvec(out,migrationMatrix);putvec(out,recentCrossings);put(out,c.scheduler);put(out,reusedOscillations);put(out,zeroTransportEdges);put(out,c.morphogenesis);put(out,c.softBody);put(out,surfaceContacts);put(out,membraneHarvest);put(out,c.tissueCoupling);put(out,broadphaseRejects);put(out,conservativeRejects);put(out,narrowphaseTests);put(out,c.allContacts);put(out,c.regulatoryNetwork);put(out,globalCandidatePairs);put(out,globalExactPairs);put(out,globalCollisionPairs);put(out,regulatoryRewires);put(out,crossParentRecombinations);
  put(out,c.bodyCap);put(out,c.founderNodes);put(out,c.contactDedupe);put(out,c.quietActivityLimit);put(out,c.quietFluxTolerance);
  put(out,dedupeAvoided);put(out,tileDuplicatePrunes);put(out,skippedQuietEdges);put(out,quietRegionSteps);put(out,skippedDetritusFlux);put(out,skippedSignalFlux);put(out,c.neuronCap);put(out,c.edgeCap);put(out,c.founderNeurons);put(out,c.sparseBuckets);put(out,c.graphPool);put(out,c.contactCache);put(out,c.historyLedger);put(out,eventLedgerHash);put(out,eventLedgerCount);put(out,contactCacheSlots);
  put(out,c.metabolicEcology);put(out,metabolismRng.state);put(out,foodCargoUptake);put(out,detritusCargoUptake);put(out,foodCargoConverted);put(out,detritusCargoConverted);put(out,metabolicDissipation);put(out,cargoReturnedOnDeath);
  for(int j=0;j<p.cap;j++){put(out,p.genomes[j].assimilationBias);put(out,p.genomes[j].transportGene);putvec(out,p.cargoFood[j]);putvec(out,p.cargoDetritus[j]);putvec(out,p.flowFood[j]);putvec(out,p.flowDetritus[j]);}
  string bytes=out.str();uint64_t h=digest(bytes);string tmp=path+".tmp";ofstream f(tmp,ios::binary|ios::trunc);if(!f)throw runtime_error("Unable to create checkpoint: "+tmp);f.write(bytes.data(),bytes.size());put(f,h);f.close();if(!f)throw runtime_error("Checkpoint write failed");if(std::rename(tmp.c_str(),path.c_str())!=0)throw runtime_error("Checkpoint atomic rename failed");
 }
 void restore(string const&path){
  ifstream f(path,ios::binary);if(!f)throw runtime_error("Unable to open checkpoint");string all((istreambuf_iterator<char>(f)),istreambuf_iterator<char>());
  if(all.size()<32||all.size()>550000000)throw runtime_error("Invalid checkpoint size");
  uint64_t expected=0;memcpy(&expected,all.data()+all.size()-8,8);all.resize(all.size()-8);if(digest(all)!=expected)throw runtime_error("Checkpoint integrity mismatch");
  istringstream in(all,ios::binary|ios::in);if(get<uint64_t>(in)!=0x45564f4c41423831ULL)throw runtime_error("Unsupported checkpoint magic");uint32_t version=get<uint32_t>(in);if(version!=1&&version!=2&&version!=3&&version!=4&&version!=5&&version!=6&&version!=7&&version!=8&&version!=9&&version!=10&&version!=11&&version!=12&&version!=13&&version!=14&&version!=15&&version!=16&&version!=17&&version!=18&&version!=19&&version!=20&&version!=21&&version!=22&&version!=23)throw runtime_error("Unsupported checkpoint version");
  bool checkpointPool=version>=22?(get<uint8_t>(in)!=0):false;
  if(checkpointPool!=p.compact){ // checkpoint settings are authoritative, including restored v21 legacy arrays
    p.compact=checkpointPool;
    if(p.compact){p.neural.clear();p.jointAngle.clear();p.jointVelocity.clear();p.tissueSignal.clear();p.neuralSlabs.resize(p.cap);p.angleSlabs.resize(p.cap);p.velocitySlabs.resize(p.cap);p.signalSlabs.resize(p.cap);}
    else{p.neuralSlabs.clear();p.angleSlabs.clear();p.velocitySlabs.clear();p.signalSlabs.clear();}}
  uint64_t seed=get<uint64_t>(in);int initial=get<int>(in),cap=get<int>(in);float mutation=get<float>(in);bool neutral=get<bool>(in);
  if(cap!=p.cap||initial<1||initial>cap||cap>100000)throw runtime_error("Checkpoint capacity mismatch");
  c.seed=seed;c.initial=initial;c.mutation=mutation;c.neutral=neutral;
  c.resourceRate=get<float>(in);c.season=get<float>(in);c.climate=get<float>(in);c.predation=get<float>(in);c.sharing=get<float>(in);uint8_t abl=get<uint8_t>(in);c.ablate=abl==1?"motor":abl==2?"signal":abl==3?"climate":abl==4?"interaction":abl==5?"organization":abl==6?"cooperation":abl==7?"integration":"none";
  rng.state=get<uint64_t>(in);tick=get<uint64_t>(in);births=get<uint64_t>(in);deaths=get<uint64_t>(in);contacts=get<uint64_t>(in);exchanges=get<uint64_t>(in);signals=get<uint64_t>(in);predations=get<uint64_t>(in);shares=get<uint64_t>(in);mutations=get<uint64_t>(in);structuralMutations=get<uint64_t>(in);nextUID=get<uint64_t>(in);maxGen=get<int>(in);
  int storedScale=version>=12?get<int>(in):1;
  if(storedScale!=c.worldScale)throw runtime_error("World geometry mismatch: resume with the original --world-scale");
  getarray(in,food);getarray(in,detritus);getarray(in,moisture);getarray(in,pheromone);getarray(in,baseMoisture);getarray(in,temperature);
  getvec(in,live,p.cap);getvec(in,available,p.cap);
  getvec(in,p.x,p.cap);getvec(in,p.y,p.cap);getvec(in,p.vx,p.cap);getvec(in,p.vy,p.cap);getvec(in,p.heading,p.cap);getvec(in,p.energy,p.cap);getvec(in,p.age,p.cap);getvec(in,p.sig,p.cap);{
    if(checkpointPool){for(int i=0;i<p.cap;i++)getvec(in,p.neuralSlabs[i],MAX_NEU);}
    else{
    vector<float> stored;getvec(in,stored,size_t(p.cap)*MAX_NEU);
    int oldStride=version>=21?int(stored.size()/p.cap):32;
    if(stored.size()!=size_t(p.cap)*size_t(oldStride)||oldStride<32||oldStride>MAX_NEU)throw runtime_error("Invalid neural checkpoint stride");
    p.neuralStride=oldStride;p.neural=move(stored);
   }}getvec(in,p.signalMemory,p.cap);auto restoreBodyArray=[&](vector<float>&dest,bool present){
    fill(dest.begin(),dest.end(),0.f);if(!present)return;
    if(version>=20){getvec(in,dest,size_t(p.cap)*MAX_BOD);return;}
    vector<float>old;getvec(in,old,size_t(p.cap)*LEGACY_BOD);
    dest.assign(size_t(p.cap)*96,0.f);
    for(int slot=0;slot<p.cap;slot++)copy_n(old.begin()+size_t(slot)*LEGACY_BOD,LEGACY_BOD,dest.begin()+size_t(slot)*96);
  };
  if(checkpointPool){for(int i=0;i<p.cap;i++){getvec(in,p.angleSlabs[i],MAX_BOD);getvec(in,p.velocitySlabs[i],MAX_BOD);getvec(in,p.signalSlabs[i],MAX_BOD);}}
  else {restoreBodyArray(p.jointAngle,true);restoreBodyArray(p.jointVelocity,true);restoreBodyArray(p.tissueSignal,version>=18);}
  if(!checkpointPool&&(p.jointAngle.size()%size_t(p.cap)||p.jointAngle.size()!=p.jointVelocity.size()||p.jointAngle.size()!=p.tissueSignal.size()))throw runtime_error("Invalid body state stride");
  if(!checkpointPool)p.bodyStride=int(p.jointAngle.size()/p.cap);
  if(p.bodyStride<96||p.bodyStride>MAX_BOD)throw runtime_error("Invalid body stride");
  getvec(in,p.generation,p.cap);getvec(in,p.alive,p.cap);getvec(in,p.kind,p.cap);getvec(in,p.offspring,p.cap);getvec(in,p.ancestor,p.cap);getvec(in,p.uid,p.cap);getvec(in,p.parentUid,p.cap);
  for(int j=0;j<p.cap;j++){auto &g=p.genomes[j];g.neurons=get<uint8_t>(in);g.metabolism=get<float>(in);g.vision=get<float>(in);g.threshold=get<float>(in);g.signal=get<float>(in);g.drag=get<float>(in);g.tempOpt=get<float>(in);if(version>=19)getvec(in,g.body,version>=21?MAX_BOD:(version>=20?96:LEGACY_BOD));
       else if(version==18){vector<LegacyNodeV18>old;getvec(in,old,LEGACY_BOD);g.body.clear();g.body.reserve(old.size());for(auto const&n:old)g.body.push_back({n.parent,n.type,n.ctrl,n.sense,n.length,n.angle,n.growth,n.motor,n.thickness,n.flexibility,n.conductance,n.stiffness});}
      else if(version==17){vector<LegacyNodeV17> old;getvec(in,old,LEGACY_BOD);g.body.clear();g.body.reserve(old.size());for(auto const&n:old)g.body.push_back({n.parent,n.type,n.ctrl,n.sense,n.length,n.angle,n.growth,n.motor,n.thickness,n.flexibility});}
      else{vector<LegacyNode> old;getvec(in,old,LEGACY_BOD);g.body.clear();g.body.reserve(old.size());for(auto const&n:old)g.body.push_back({n.parent,n.type,n.ctrl,n.sense,n.length,n.angle,n.growth,n.motor});}getvec(in,g.edges,version>=21?MAX_EDGE:128);
   if(version>=3){g.roleBias=get<float>(in);g.collectivity=get<float>(in);}else{g.roleBias=bound((g.tempOpt-.5f)*2.f,-1.f,1.f);g.collectivity=bound(g.signal*.72f,.05f,1.f);}}
  uint64_t hn=get<uint64_t>(in);if(hn>2000000)throw runtime_error("Checkpoint lineage class count too large");heritage.clear();for(uint64_t k=0;k<hn;k++){auto hash=get<uint64_t>(in);auto count=get<int>(in);heritage[hash]=count;}getvec(in,events,260);uint64_t nrel=get<uint64_t>(in);if(nrel>100000)throw runtime_error("Checkpoint relationship count too large");relationships.clear();for(uint64_t j=0;j<nrel;j++){uint64_t key=get<uint64_t>(in);Interaction item=get<Interaction>(in);relationships[key]=item;}
  if(version>=2){c.cohesion=get<float>(in);bondsFormed=get<uint64_t>(in);bondsBroken=get<uint64_t>(in);bondedEnergy=get<double>(in);
    if(version==2){vector<OldBond> old;getvec(in,old,size_t(p.cap));bonds.clear();for(auto const &b:old)bonds.push_back({b.a,b.b,b.rest,b.age,b.transferred,0});}
    else if(version==3){vector<OldBondV84> old;getvec(in,old,size_t(p.cap));bonds.clear();for(auto const &b:old)bonds.push_back({b.a,b.b,b.rest,b.age,b.transferred,b.origin});}
    else if(version==4){vector<OldBondV85> old;getvec(in,old,size_t(p.cap));bonds.clear();for(auto const&b:old){Bond now{b.a,b.b,b.rest,b.age,b.transferred,b.origin,b.groupId,b.parentGroupId,b.groupGeneration};bonds.push_back(now);}}
    else getvec(in,bonds,size_t(p.cap));}
  else{c.cohesion=0;bonds.clear();bondsFormed=bondsBroken=0;bondedEnergy=0;}
  if(version>=3){c.specialization=get<float>(in);propagulesBorn=get<uint64_t>(in);propagulesMatured=get<uint64_t>(in);}else{if(!c.specializationSpecified)c.specialization=0;propagulesBorn=propagulesMatured=0;}
  if(version>=4){nextGroupId=get<uint64_t>(in);descendantGroupBirths=get<uint64_t>(in);maxGroupGeneration=get<uint32_t>(in);c.cooperation=get<float>(in);cooperativeDefenses=get<uint64_t>(in);preventedLoss=get<double>(in);}
  else {for(auto &b:bonds){b.groupId=++nextGroupId;b.parentGroupId=0;b.groupGeneration=0;}descendantGroupBirths=0;maxGroupGeneration=0;c.cooperation=0;cooperativeDefenses=0;preventedLoss=0;}
  if(version>=5){c.integration=get<float>(in);integratedHarvest=get<double>(in);integratedCost=get<double>(in);integratedEvents=get<uint64_t>(in);productiveOrganizations=get<uint64_t>(in);}
  else{c.integration=0;integratedHarvest=integratedCost=0;integratedEvents=productiveOrganizations=0;}
  if(version>=6){c.scavenging=get<float>(in);scavenges=get<uint64_t>(in);scavengedEnergy=get<double>(in);}else{c.scavenging=0;scavenges=0;scavengedEnergy=0;}
  if(version>=7){c.plasticity=get<float>(in);subtreeCopies=get<uint64_t>(in);reparentedBranches=get<uint64_t>(in);feedbackMutations=get<uint64_t>(in);}else{c.plasticity=0;subtreeCopies=reparentedBranches=feedbackMutations=0;}
  if(version>=8){c.ecology=get<float>(in);c.development=get<float>(in);getarray(in,soil);
   for(int j=0;j<p.cap;j++){p.genomes[j].niche=get<float>(in);p.genomes[j].engineering=get<float>(in);}
   soilEdits=get<uint64_t>(in);soilMineralizations=get<uint64_t>(in);nicheHarvests=get<uint64_t>(in);developmentMutations=get<uint64_t>(in);recycledMatter=get<double>(in);ecologicalHarvest=get<double>(in);
  }else{c.ecology=0;c.development=0;for(int j=0;j<NC;j++)soil[j]=bound(.33f+.55f*baseMoisture[j],.05f,1.f);
   for(auto &g:p.genomes){g.niche=0;g.engineering=0;}}
  if(version>=9){c.grand=get<float>(in);extendedHarvests=get<uint64_t>(in);extendedEnergy=get<double>(in);}else{c.grand=0;extendedHarvests=0;extendedEnergy=0;}
  c.habitats=version>=10?get<float>(in):0.f;
  c.transport=version>=11?get<float>(in):0.f;
  transportEvents=version>=11?get<uint64_t>(in):0;
  transportedDetritus=version>=11?get<double>(in):0;
  regionalMigrants=version>=12?get<uint64_t>(in):0;
  crossRegionMatter=version>=12?get<double>(in):0;
  c.regionality=version>=13?get<float>(in):0.f;
  if(version>=13){getvec(in,migrationMatrix,size_t(c.worldScale*c.worldScale*c.worldScale*c.worldScale));getvec(in,recentCrossings,96);}
  else{fill(migrationMatrix.begin(),migrationMatrix.end(),0);recentCrossings.clear();}
  c.scheduler=version>=14?get<int>(in):0;
  reusedOscillations=version>=15?get<uint64_t>(in):0;
  zeroTransportEdges=version>=15?get<uint64_t>(in):0;
  c.morphogenesis=version>=16?get<float>(in):0.f;
  c.softBody=version>=17?get<float>(in):0.f;
  surfaceContacts=version>=17?get<uint64_t>(in):0;membraneHarvest=version>=17?get<double>(in):0;c.tissueCoupling=version>=18?get<float>(in):0.f;broadphaseRejects=version>=18?get<uint64_t>(in):0;conservativeRejects=version>=18?get<uint64_t>(in):0;narrowphaseTests=version>=18?get<uint64_t>(in):0;
   c.allContacts=version>=19?get<int>(in):0;c.regulatoryNetwork=version>=19?get<float>(in):0.f;
   globalCandidatePairs=version>=19?get<uint64_t>(in):0;globalExactPairs=version>=19?get<uint64_t>(in):0;
   globalCollisionPairs=version>=19?get<uint64_t>(in):0;regulatoryRewires=version>=19?get<uint64_t>(in):0;crossParentRecombinations=version>=19?get<uint64_t>(in):0;
   if(version>=20){c.bodyCap=get<int>(in);c.founderNodes=get<int>(in);c.contactDedupe=get<int>(in);c.quietActivityLimit=get<int>(in);c.quietFluxTolerance=get<float>(in);dedupeAvoided=get<uint64_t>(in);tileDuplicatePrunes=get<uint64_t>(in);skippedQuietEdges=get<uint64_t>(in);quietRegionSteps=get<uint64_t>(in);skippedDetritusFlux=get<double>(in);skippedSignalFlux=get<double>(in);if(version>=21){c.neuronCap=get<int>(in);c.edgeCap=get<int>(in);c.founderNeurons=get<int>(in);c.sparseBuckets=get<int>(in);}else{c.neuronCap=32;c.edgeCap=128;c.founderNeurons=0;c.sparseBuckets=0;}}
   else {c.neuronCap=32;c.edgeCap=128;c.bodyCap=40;c.founderNodes=0;c.contactDedupe=0;c.quietActivityLimit=12;c.quietFluxTolerance=0;dedupeAvoided=tileDuplicatePrunes=skippedQuietEdges=quietRegionSteps=0;skippedDetritusFlux=skippedSignalFlux=0;}
   if(c.bodyCap<40||c.bodyCap>MAX_BOD||c.neuronCap<32||c.neuronCap>MAX_NEU||c.edgeCap<128||c.edgeCap>MAX_EDGE||c.founderNeurons<0||c.founderNeurons>c.neuronCap||c.sparseBuckets<0||c.sparseBuckets>2||c.contactDedupe<0||c.contactDedupe>1||c.founderNodes<0||c.founderNodes>c.bodyCap||!std::isfinite(c.quietFluxTolerance)||c.quietFluxTolerance<0||c.quietFluxTolerance>.05f)throw runtime_error("Invalid adaptive world state");
  if(version>=22){c.graphPool=get<int>(in);c.contactCache=get<int>(in);c.historyLedger=get<int>(in);eventLedgerHash=get<uint64_t>(in);eventLedgerCount=get<uint64_t>(in);contactCacheSlots=get<uint64_t>(in);}else{c.graphPool=0;c.contactCache=0;c.historyLedger=0;eventLedgerHash=1469598103934665603ULL;eventLedgerCount=contactCacheSlots=0;}
  if(version>=23){
    c.metabolicEcology=get<float>(in);metabolismRng.state=get<uint64_t>(in);
    foodCargoUptake=get<double>(in);detritusCargoUptake=get<double>(in);foodCargoConverted=get<double>(in);detritusCargoConverted=get<double>(in);metabolicDissipation=get<double>(in);cargoReturnedOnDeath=get<double>(in);
    if(!std::isfinite(c.metabolicEcology)||c.metabolicEcology<0.f||c.metabolicEcology>2.f)throw runtime_error("Invalid metabolism mode");
    for(int j=0;j<p.cap;j++){
      auto &g=p.genomes[j];g.assimilationBias=get<float>(in);g.transportGene=get<float>(in);
      getvec(in,p.cargoFood[j],MAX_BOD);getvec(in,p.cargoDetritus[j],MAX_BOD);getvec(in,p.flowFood[j],MAX_BOD);getvec(in,p.flowDetritus[j],MAX_BOD);
      if(!std::isfinite(g.assimilationBias)||g.assimilationBias< -1.f||g.assimilationBias>1.f||!std::isfinite(g.transportGene)||g.transportGene<.1f||g.transportGene>1.7f)throw runtime_error("Invalid inherited resource strategy");
      if(c.metabolicEcology>0.f&&p.alive[j]&&(p.cargoFood[j].size()!=g.body.size()||p.cargoDetritus[j].size()!=g.body.size()||p.flowFood[j].size()!=g.body.size()||p.flowDetritus[j].size()!=g.body.size()))throw runtime_error("Invalid internal cargo length");
      for(float v:p.cargoFood[j])if(!std::isfinite(v)||v<0.f||v>5.f)throw runtime_error("Invalid food cargo");
      for(float v:p.cargoDetritus[j])if(!std::isfinite(v)||v<0.f||v>5.f)throw runtime_error("Invalid detritus cargo");
      for(float v:p.flowFood[j])if(!std::isfinite(v)||v<0.f||v>5.f)throw runtime_error("Invalid food flow");
      for(float v:p.flowDetritus[j])if(!std::isfinite(v)||v<0.f||v>5.f)throw runtime_error("Invalid detritus flow");
    }
  }else{c.metabolicEcology=0.f;foodCargoUptake=detritusCargoUptake=foodCargoConverted=detritusCargoConverted=metabolicDissipation=cargoReturnedOnDeath=0.;}
  if(c.graphPool!=(p.compact?1:0)||c.graphPool<0||c.graphPool>1||c.contactCache<0||c.contactCache>1||c.historyLedger<0||c.historyLedger>1)throw runtime_error("Invalid graph pool flags");
  if(c.allContacts<0||c.allContacts>1||!std::isfinite(c.regulatoryNetwork)||c.regulatoryNetwork<0||c.regulatoryNetwork>2||c.scheduler<0||c.scheduler>1||(!std::isfinite(c.morphogenesis))||c.morphogenesis<0||c.morphogenesis>2||(!std::isfinite(c.softBody))||c.softBody<0||c.softBody>2||!std::isfinite(c.tissueCoupling)||c.tissueCoupling<0.f||c.tissueCoupling>2.f)throw runtime_error("Invalid scheduler or morphogenesis mode in checkpoint");
  if(migrationMatrix.size()!=size_t(c.worldScale*c.worldScale*c.worldScale*c.worldScale))throw runtime_error("Invalid directed migration matrix");
  if(c.ecology<0||c.ecology>2||c.development<0||c.development>2||c.grand<0||c.grand>2||c.habitats<0||c.habitats>2||c.transport<0||c.transport>2||c.regionality<0||c.regionality>2)throw runtime_error("Invalid ecological parameters in checkpoint");
  if(checkpointPool){for(int i=0;i<p.cap;i++){
    auto const&g=p.genomes[i];if(p.alive[i]){if(p.neuralSlabs[i].size()!=g.neurons||p.angleSlabs[i].size()!=g.body.size()||p.velocitySlabs[i].size()!=g.body.size()||p.signalSlabs[i].size()!=g.body.size())throw runtime_error("Invalid compact organism state sizes");}
   } }
  if(c.bodyCap>p.bodyStride||c.neuronCap>p.neuralStride)throw runtime_error("Checkpoint capacity/stride inconsistency");
  for(auto const&g:p.genomes){if(g.neurons>c.neuronCap||g.body.size()>size_t(c.bodyCap)||g.edges.size()>size_t(c.edgeCap))throw runtime_error("Genome exceeds stored capacity");}
  if(in.peek()!=char_traits<char>::eof())throw runtime_error("Trailing checkpoint data");
  for(auto const&b:bonds){if(b.groupId==0||b.groupId>nextGroupId||b.groupGeneration>maxGroupGeneration)throw runtime_error("Invalid organization lineage reference");}
  if(live.size()+available.size()!=size_t(p.cap))throw runtime_error("Checkpoint slots inconsistent");
  for(int j:live)if(j<0||j>=p.cap||!p.alive[j])throw runtime_error("Checkpoint live slots invalid");
 }
 // Exact, non-intrusive state audit: no skipped life/environment update.
 // Per-region aggregates are computed from EVERY world cell and EVERY living agent.
 void regionalAudit(ostream&out)const{
   const int zones=c.worldScale*c.worldScale;
   vector<uint64_t> count(zones,0),uidHash(zones,1469598103934665603ULL);
   vector<double>energy(zones,0),nutrient(zones,0),debris(zones,0),signal(zones,0);
   for(int i:live){int r=zone(p.x[i],p.y[i]);count[r]++;energy[r]+=p.energy[i];uint64_t u=p.uid[i];uidHash[r]^=u;uidHash[r]*=1099511628211ULL;}
   for(int k=0;k<NC;k++){int r=cellRegions[k];nutrient[r]+=food[k];debris[r]+=detritus[k];signal[r]+=pheromone[k];}
   for(int r=0;r<zones;r++)out<<tick<<','<<r<<','<<count[r]<<','<<setprecision(17)<<energy[r]<<','<<nutrient[r]<<','<<debris[r]<<','<<signal[r]<<','<<uidHash[r]<<','<<eventLedgerCount<<','<<eventLedgerHash<<'\n';
 }
 void report(ostream &out,bool visual=true){
  // Connected components are observational labels, not predefined multicellular species.
  unordered_map<uint64_t,int> ids;ids.reserve(live.size()*2+1);for(size_t k=0;k<live.size();k++)ids[p.uid[live[k]]]=int(k);
  vector<int>parent(live.size());iota(parent.begin(),parent.end(),0);
  auto root=[&](int a){while(parent[a]!=a){parent[a]=parent[parent[a]];a=parent[a];}return a;};
  int attached=0,complementaryPairs=0,livingGroupMax=0;vector<char>flag(live.size());for(auto const &b:bonds){auto a=ids.find(b.a),c=ids.find(b.b);if(a==ids.end()||c==ids.end())continue;int ra=root(a->second),rb=root(c->second);if(ra!=rb)parent[rb]=ra;flag[a->second]=flag[c->second]=1;if(p.genomes[live[a->second]].roleBias*p.genomes[live[c->second]].roleBias<0)complementaryPairs++;livingGroupMax=max(livingGroupMax,int(b.groupGeneration));}
  for(char x:flag)attached+=x;
  unordered_map<int,int>sizes;for(size_t k=0;k<live.size();k++)if(flag[k])sizes[root(int(k))]++;
  int groups=sizes.size(),largest=0;for(auto const&v:sizes)largest=max(largest,v.second);
  double avgNodes=0,avgNeurons=0,meanEnergy=0,meanMismatch=0,meanTempOpt=0,meanRole=0,meanFeedback=0;int feedbackCarriers=0,maxBodyNodes=0,g=0,nicheSensorCarriers=0,habitatOrganCarriers=0,gradientCarriers=0;double meanNiche=0;unordered_set<uint64_t>types;unordered_set<uint64_t>bodyArchitectures;int largeBodyCarriers=0;
 for(int i:live){auto &gm=p.genomes[i];
   // Observer-only body topology signature: actual parent-child structure + organ types.
   // This does not alter the selection algorithm or label the result a species.
   uint64_t topology=1469598103934665603ULL;
   for(auto const &part:gm.body){topology^=uint64_t(part.parent)+1;topology*=1099511628211ULL;topology^=uint64_t(part.type)+1;topology*=1099511628211ULL;}
   topology^=uint64_t(gm.body.size());bodyArchitectures.insert(topology);
   largeBodyCarriers+=gm.body.size()>=10;avgNodes+=gm.body.size();avgNeurons+=gm.neurons;int recurrent=int(count_if(gm.edges.begin(),gm.edges.end(),[](Edge const&e){return e.from>=6&&e.from<64;}));meanFeedback+=recurrent;feedbackCarriers+=recurrent>0;maxBodyNodes=max(maxBodyNodes,int(gm.body.size()));meanEnergy+=p.energy[i];meanMismatch+=fabs(temperature[index(p.x[i],p.y[i])]-gm.tempOpt);meanTempOpt+=gm.tempOpt;meanRole+=gm.roleBias;meanNiche+=gm.niche;nicheSensorCarriers+=any_of(gm.edges.begin(),gm.edges.end(),[](Edge const&e){return e.from>=64&&e.from<67;});habitatOrganCarriers+=any_of(gm.body.begin(),gm.body.end(),[](Node const&n){return n.type>=5;});gradientCarriers+=any_of(gm.edges.begin(),gm.edges.end(),[](Edge const&e){return e.from>=67&&e.from<69;});g=max(g,p.generation[i]);types.insert(signature(gm));}int n=live.size();if(n){avgNodes/=n;avgNeurons/=n;meanFeedback/=n;meanEnergy/=n;meanMismatch/=n;meanTempOpt/=n;meanRole/=n;meanNiche/=n;}out<<fixed<<setprecision(4)<<"{\"type\":\"state\",\"tick\":"<<tick<<",\"population\":"<<n<<",\"maxGeneration\":"<<maxGen<<",\"livingMaxGeneration\":"<<g<<",\"births\":"<<births<<",\"deaths\":"<<deaths<<",\"contacts\":"<<contacts<<",\"exchanges\":"<<exchanges<<",\"signals\":"<<signals<<",\"predations\":"<<predations<<",\"shares\":"<<shares<<",\"resourceRate\":"<<c.resourceRate<<",\"experimentalMigrationCost\":"<<c.experimentalMigrationCost<<",\"experimentalIsolation\":"<<c.experimentalIsolation<<",\"blockedCrossings\":"<<blockedCrossings<<",\"migrationEnergyPaid\":"<<migrationEnergyPaid<<",\"season\":"<<c.season<<",\"climate\":"<<c.climate<<",\"mutations\":"<<mutations<<",\"structuralMutations\":"<<structuralMutations<<",\"heritageClasses\":"<<heritage.size()<<",\"livingClasses\":"<<types.size()<<",\"meanNodes\":"<<avgNodes<<",\"meanNeurons\":"<<avgNeurons<<",\"meanEnergy\":"<<meanEnergy<<",\"meanThermalMismatch\":"<<meanMismatch<<",\"meanTempOpt\":"<<meanTempOpt<<",\"rngState\":"<<rng.state<<",\"cohesion\":"<<c.cohesion<<",\"activeBonds\":"<<bonds.size()<<",\"bondedAgents\":"<<attached<<",\"connectedGroups\":"<<groups<<",\"largestGroup\":"<<largest<<",\"bondsFormed\":"<<bondsFormed<<",\"bondsBroken\":"<<bondsBroken<<",\"bondedEnergy\":"<<bondedEnergy<<",\"specialization\":"<<c.specialization<<",\"meanRoleBias\":"<<meanRole<<",\"complementaryPairs\":"<<complementaryPairs<<",\"propagulesBorn\":"<<propagulesBorn<<",\"propagulesMatured\":"<<propagulesMatured<<",\"organizationLineages\":"<<nextGroupId<<",\"maxOrganizationGeneration\":"<<maxGroupGeneration<<",\"livingOrganizationGeneration\":"<<livingGroupMax<<",\"descendantOrganizationBirths\":"<<descendantGroupBirths<<",\"cooperation\":"<<c.cooperation<<",\"cooperativeDefenses\":"<<cooperativeDefenses<<",\"preventedLoss\":"<<preventedLoss<<",\"integration\":"<<c.integration<<",\"integratedEvents\":"<<integratedEvents<<",\"integratedHarvest\":"<<integratedHarvest<<",\"integratedCost\":"<<integratedCost<<",\"productiveOrganizations\":"<<productiveOrganizations<<",\"scavenging\":"<<c.scavenging<<",\"scavenges\":"<<scavenges<<",\"scavengedEnergy\":"<<scavengedEnergy<<",\"plasticity\":"<<c.plasticity<<",\"subtreeCopies\":"<<subtreeCopies<<",\"reparentedBranches\":"<<reparentedBranches<<",\"feedbackMutations\":"<<feedbackMutations<<",\"feedbackCarriers\":"<<feedbackCarriers<<",\"meanFeedbackEdges\":"<<meanFeedback<<",\"maxBodyNodes\":"<<maxBodyNodes<<",\"meanNiche\":"<<meanNiche<<",\"nicheSensorCarriers\":"<<nicheSensorCarriers<<",\"habitatOrganCarriers\":"<<habitatOrganCarriers<<",\"ecology\":"<<c.ecology<<",\"development\":"<<c.development<<",\"soilEdits\":"<<soilEdits<<",\"soilMineralizations\":"<<soilMineralizations<<",\"nicheHarvests\":"<<nicheHarvests<<",\"developmentMutations\":"<<developmentMutations<<",\"recycledMatter\":"<<recycledMatter<<",\"ecologicalHarvest\":"<<ecologicalHarvest<<",\"grand\":"<<c.grand<<",\"habitats\":"<<c.habitats<<",\"transport\":"<<c.transport<<",\"transportEvents\":"<<transportEvents<<",\"transportedDetritus\":"<<transportedDetritus<<",\"threads\":"<<c.threads<<",\"lifeParallel\":"<<c.lifeParallel<<",\"lifeParallelActive\":"<<(c.lifeParallel==1&&c.threads>1&&live.size()>=128&&accumulate(live.begin(),live.end(),size_t(0),[&](size_t v,int i){return v+p.genomes[i].body.size();})>=live.size()*7?1:0)<<",\"extendedHarvests\":"<<extendedHarvests<<",\"extendedEnergy\":"<<extendedEnergy<<",\"gradientCarriers\":"<<gradientCarriers<<",\"bodyArchitectures\":"<<bodyArchitectures.size()<<",\"largeBodyCarriers\":"<<largeBodyCarriers<<",\"worldScale\":"<<c.worldScale<<",\"worldWidth\":"<<WIDTH<<",\"worldHeight\":"<<HEIGHT<<",\"worldGridWidth\":"<<GX<<",\"worldGridHeight\":"<<GY<<",\"regionalMigrants\":"<<regionalMigrants<<",\"crossRegionMatter\":"<<crossRegionMatter<<",\"regionality\":"<<c.regionality<<",\"morphogenesis\":"<<c.morphogenesis<<",\"softBody\":"<<c.softBody<<",\"surfaceContacts\":"<<surfaceContacts<<",\"membraneHarvest\":"<<membraneHarvest<<",\"collisionIndex\":"<<c.collisionIndex<<",\"broadphaseRejects\":"<<broadphaseRejects<<",\"conservativeRejects\":"<<conservativeRejects<<",\"narrowphaseTests\":"<<narrowphaseTests<<",\"allContacts\":"<<c.allContacts<<",\"regulatoryNetwork\":"<<c.regulatoryNetwork<<",\"globalCandidatePairs\":"<<globalCandidatePairs<<",\"globalExactPairs\":"<<globalExactPairs<<",\"globalCollisionPairs\":"<<globalCollisionPairs<<",\"regulatoryRewires\":"<<regulatoryRewires<<",\"crossParentRecombinations\":"<<crossParentRecombinations<<",\"contactCacheSlots\":"<<contactCacheSlots<<",\"eventLedgerHash\":"<<eventLedgerHash<<",\"eventLedgerCount\":"<<eventLedgerCount<<",\"graphPool\":"<<c.graphPool<<",\"contactCache\":"<<c.contactCache<<",\"historyLedger\":"<<c.historyLedger<<",\"allocatedStateFloats\":"<<p.allocatedFloats()<<",\"activeStateFloats\":"<<p.activeFloats()<<",\"bodyCap\":"<<c.bodyCap<<",\"neuronCap\":"<<c.neuronCap<<",\"edgeCap\":"<<c.edgeCap<<",\"founderNeurons\":"<<c.founderNeurons<<",\"sparseBuckets\":"<<c.sparseBuckets<<",\"bodyStride\":"<<p.bodyStride<<",\"neuralStride\":"<<p.neuralStride<<",\"founderNodes\":"<<c.founderNodes<<",\"contactDedupe\":"<<c.contactDedupe<<",\"dedupeAvoided\":"<<dedupeAvoided<<",\"tileDuplicatePrunes\":"<<tileDuplicatePrunes<<",\"quietFluxTolerance\":"<<c.quietFluxTolerance<<",\"quietActivityLimit\":"<<c.quietActivityLimit<<",\"quietRegionSteps\":"<<quietRegionSteps<<",\"skippedQuietEdges\":"<<skippedQuietEdges<<",\"omittedDetritusFlux\":"<<skippedDetritusFlux<<",\"omittedSignalFlux\":"<<skippedSignalFlux<<",\"metabolicEcology\":"<<c.metabolicEcology<<",\"meanAssimilationBias\":"<<([&](){double v=0;for(int id:live)v+=p.genomes[id].assimilationBias;return live.empty()?0.:v/live.size();}())<<",\"cargoFoodUptake\":"<<foodCargoUptake<<",\"cargoDetritusUptake\":"<<detritusCargoUptake<<",\"cargoFoodProcessed\":"<<foodCargoConverted<<",\"cargoDetritusProcessed\":"<<detritusCargoConverted<<",\"metabolicHeat\":"<<metabolicDissipation<<",\"cargoReturnedOnDeath\":"<<cargoReturnedOnDeath<<",\"liveCargo\":"<<([&](){double total=0;for(int li:live){for(float v:p.cargoFood[li])total+=v;for(float v:p.cargoDetritus[li])total+=v;}return total;}())<<",\"tissueCoupling\":"<<c.tissueCoupling<<",\"meanTissueSignal\":"<<([&](){double n=0;size_t count=0;for(int id:live)for(size_t j=1;j<p.genomes[id].body.size();j++)if(expressed(id,j)){n+=fabs(p.signalAt(id,int(j)));count++;}return count?n/count:0.;}());
 // Functional observatory: independently measured actual substrate uptake for this tick,
 // plus inherited organ carrier counts and generation/energy statistics for all living agents.
 // A single mixed body can belong to multiple carrier groups; these are NOT species counts.
 if(c.metabolicEcology>0.f){
   array<int,3> carriers{};array<double,3> generationSum{},energySum{};
   for(int id:live){array<bool,3> seen{};for(size_t b=1;b<p.genomes[id].body.size();b++){
     auto const&node=p.genomes[id].body[b];if(!expressed(id,b))continue;
     if(node.type==2)seen[0]=true;else if(node.type==5)seen[1]=true;else if(node.type==7)seen[2]=true;
   }for(int k=0;k<3;k++)if(seen[k]){carriers[k]++;generationSum[k]+=p.generation[id];energySum[k]+=p.energy[id];}}
   out<<",\"functionalUptake\":["<<functionalUptake[0]<<','<<functionalUptake[1]<<','<<functionalUptake[2]<<"]";
   out<<",\"functionalCarriers\":["<<carriers[0]<<','<<carriers[1]<<','<<carriers[2]<<"]";
   out<<",\"functionalMeanGeneration\":[";
   for(int k=0;k<3;k++){if(k)out<<',';out<<(carriers[k]?generationSum[k]/carriers[k]:0.);}
   out<<"],\"functionalMeanEnergy\":[";
   for(int k=0;k<3;k++){if(k)out<<',';out<<(carriers[k]?energySum[k]/carriers[k]:0.);}
   out<<"],\"anchorVelocityLoss\":"<<anchorVelocityLoss;
    out<<",\"organAblation\":\""<<c.organAblation<<"\"";
    out<<",\"genomePrune\":\""<<c.genomePrune<<"\",\"genomePrunedLives\":"<<genomePrunedLives<<",\"genomePrunedNodes\":"<<genomePrunedNodes<<",\"genomeRescuedLives\":"<<genomeRescuedLives<<",\"genomeRescuedNodes\":"<<genomeRescuedNodes;
    out<<",\"organLedgerSinceTick\":"<<organLedgerSinceTick;
    out<<",\"organCompetition\":[";
    for(int k=0;k<3;k++){
      if(k)out<<',';out<<'[';
      // [exposureNo,exposureYes,offspringParentNo,offspringParentYes,deathNo,deathYes,
      //  newbornGenotypeNo,newbornGenotypeYes] : lifetime event counts since start, not taxonomy.
      for(int present=0;present<2;present++){if(present)out<<',';out<<organExposure[k][present];}
      for(int present=0;present<2;present++)out<<','<<organParentBirths[k][present];
      for(int present=0;present<2;present++)out<<','<<organDeaths[k][present];
      for(int present=0;present<2;present++)out<<','<<organInheritedBirths[k][present];
      out<<']';
    }out<<']';
 }
 // Region map covers every living agent, not only rendered individuals.
 out<<",\"regions\":[";for(int ry=0;ry<c.worldScale;ry++)for(int rx=0;rx<c.worldScale;rx++){
   if(rx||ry){out<<',';}int nregion=0;double energyRegion=0;int maxGenerationRegion=0;
   for(int agent:live)if(min(c.worldScale-1,int(p.x[agent]/1300.f))==rx&&min(c.worldScale-1,int(p.y[agent]/850.f))==ry){nregion++;energyRegion+=p.energy[agent];maxGenerationRegion=max(maxGenerationRegion,p.generation[agent]);}
   double regionFood=0,regionDetritus=0;for(int gy=ry*43;gy<(ry+1)*43;gy++)for(int gx=rx*65;gx<(rx+1)*65;gx++){int j=gy*GX+gx;regionFood+=food[j];regionDetritus+=detritus[j];}
   out<<'['<<rx<<','<<ry<<','<<nregion<<','<<energyRegion<<','<<maxGenerationRegion<<','<<regionFood<<','<<regionDetritus<<']';}
 out<<']';
 // Region observatory aggregates over all living individuals; classifications are functional,
 // ancestor roots are founder-lineage references, NOT species names.
 const int zones=c.worldScale*c.worldScale;
 vector<array<int,5>> regionRoles(zones);
 vector<unordered_set<uint64_t>> regionRoots(zones);
 vector<int> regionPop(zones),regionLarge(zones);vector<double> regionBodyNodes(zones,0);vector<unordered_set<uint64_t>> regionBodies(zones);vector<unordered_map<uint64_t,int>> regionAncestors(zones);
 for(int i:live){int z=zone(p.x[i],p.y[i]);regionPop[z]++;regionRoles[z][ecologicalRole(p.genomes[i])]++;regionRoots[z].insert(p.ancestor[i]);regionBodyNodes[z]+=p.genomes[i].body.size();regionLarge[z]+=p.genomes[i].body.size()>=10;regionAncestors[z][p.ancestor[i]]++;
   uint64_t h=1469598103934665603ULL;for(auto const&node:p.genomes[i].body){h^=uint64_t(node.parent)+1;h*=1099511628211ULL;h^=uint64_t(node.type)+1;h*=1099511628211ULL;}h^=uint64_t(p.genomes[i].body.size());regionBodies[z].insert(h);
 }
 out<<",\"regionProfiles\":[";
 for(int z=0;z<zones;z++){if(z)out<<',';int in=0,outgoing=0;for(int q=0;q<zones;q++){in+=int(migrationMatrix[size_t(q)*zones+z]);outgoing+=int(migrationMatrix[size_t(z)*zones+q]);}
   int top=0;for(int role=1;role<5;role++)if(regionRoles[z][role]>regionRoles[z][top])top=role;
   float resourceFactor=c.regionality>0.f?bound(1.f+c.regionality*.31f*zoneNoise(c.seed,z,33),.35f,1.65f):1.f;
   float thermalOffset=c.regionality>0.f?c.regionality*.17f*zoneNoise(c.seed,z,34):0.f;
   uint64_t majorRoot=0;int majorSize=0;for(auto const&it:regionAncestors[z])if(it.second>majorSize||(it.second==majorSize&&it.first<majorRoot)){majorRoot=it.first;majorSize=it.second;}
   out<<"{\"bodyArchitectures\":"<<regionBodies[z].size()<<",\"meanBodyNodes\":"<<(regionPop[z]?regionBodyNodes[z]/regionPop[z]:0)<<",\"largeBodyCarriers\":"<<regionLarge[z]<<",\"dominantAncestor\":"<<majorRoot<<",\"ancestorShare\":"<<(regionPop[z]?double(majorSize)/regionPop[z]:0)<<",\"id\":"<<z<<",\"x\":"<<(z%c.worldScale)<<",\"y\":"<<(z/c.worldScale)<<",\"population\":"<<regionPop[z]<<",\"founderLineages\":"<<regionRoots[z].size()<<",\"dominantRole\":"<<top<<",\"resourceFactor\":"<<resourceFactor<<",\"thermalOffset\":"<<thermalOffset<<",\"immigration\":"<<in<<",\"emigration\":"<<outgoing<<",\"roles\":[";
   for(int role=0;role<5;role++){if(role)out<<',';out<<regionRoles[z][role];}out<<"]}";}
 out<<']';
 out<<",\"scheduler\":"<<c.scheduler<<",\"regionJobs\":"<<(c.scheduler?c.worldScale*c.worldScale:0)<<",\"compute\":"<<c.compute<<",\"reusedOscillations\":"<<reusedOscillations<<",\"zeroTransportEdges\":"<<zeroTransportEdges;
 out<<",\"migrationMatrix\":[";for(size_t k=0;k<migrationMatrix.size();k++){if(k)out<<',';out<<migrationMatrix[k];}out<<']';
 out<<",\"regionCrossings\":[";for(size_t k=0;k<recentCrossings.size();k++){if(k)out<<',';auto const&v=recentCrossings[k];out<<'['<<v.tick<<','<<v.uid<<','<<v.ancestor<<','<<v.from<<','<<v.to<<','<<v.generation<<']';}out<<']';
 // Ecotypes classify observed functioning, not taxonomic species or programmed progression.
 array<int,5> ecocount{};array<double,5> econodes{},ecoenergy{},ecogen{};vector<array<int,5>> spatialRoles(size_t(160*c.worldScale*c.worldScale));
 for(int i:live){int r=ecologicalRole(p.genomes[i]);ecocount[r]++;econodes[r]+=p.genomes[i].body.size();ecoenergy[r]+=p.energy[i];ecogen[r]+=p.generation[i];
   int renderW=16*c.worldScale,renderH=10*c.worldScale;int x=clamp(int(p.x[i]/WIDTH*float(renderW)),0,renderW-1),y=clamp(int(p.y[i]/HEIGHT*float(renderH)),0,renderH-1);spatialRoles[y*renderW+x][r]++;}
 out<<",\"ecotypes\":[";for(int k=0;k<5;k++){if(k)out<<',';out<<'['<<k<<','<<ecocount[k]<<','<<(ecocount[k]?econodes[k]/ecocount[k]:0)<<','<<(ecocount[k]?ecoenergy[k]/ecocount[k]:0)<<','<<(ecocount[k]?ecogen[k]/ecocount[k]:0)<<']';}out<<']';
 if(visual){out<<",\"densityGrid\":[";for(int q=0;q<int(spatialRoles.size());q++){if(q)out<<',';int count=0;for(int r=0;r<5;r++)count+=spatialRoles[q][r];out<<count;}out<<']';}
 if(visual){out<<",\"ecotypeGrid\":[";for(int q=0;q<int(spatialRoles.size());q++){if(q)out<<',';int dominant=-1,top=0;for(int r=0;r<5;r++)if(spatialRoles[q][r]>top){top=spatialRoles[q][r];dominant=r;}out<<dominant;}out<<']';}
 if(visual){out<<",\"individuals\":[";
  // Prioritize both participants of real bonds, especially inherited propagules.
  // Rendering only the first N living IDs made many relationship lines invisible.
  vector<int>display;display.reserve(min<int>(c.visual,live.size()));unordered_set<int>used;
  auto keep=[&](int i){if(i>=0&&display.size()<size_t(c.visual)&&used.insert(i).second)display.push_back(i);};
  // Always sample the largest currently living body so rare morphology remains observable
  // even when dense worlds are rendered with a finite organism budget.
  int morphologyLeader=-1;for(int i:live)if(morphologyLeader<0||p.genomes[i].body.size()>p.genomes[morphologyLeader].body.size())morphologyLeader=i;
  keep(morphologyLeader);
  unordered_map<uint64_t,int>byUid;for(int i:live)byUid[p.uid[i]]=i;
  vector<Bond>visibleBonds=bonds;stable_sort(visibleBonds.begin(),visibleBonds.end(),[](Bond const&a,Bond const&b){return a.origin!=b.origin?a.origin>b.origin:a.age>b.age;});
  for(auto const&b:visibleBonds){auto a=byUid.find(b.a),z=byUid.find(b.b);if(a!=byUid.end()&&z!=byUid.end()&&display.size()+size_t(!used.count(a->second))+size_t(!used.count(z->second))<=size_t(c.visual)){keep(a->second);keep(z->second);}}
  // Deterministic breadth-first observer survey: cover spatial regions and functional niches.
  // The observer never changes reproduction, selection, RNG or physical states.
  array<vector<int>,200> survey{};
  for(int i:live){int rx=clamp(int(p.x[i]/WIDTH*8.f),0,7),ry=clamp(int(p.y[i]/HEIGHT*5.f),0,4);
    survey[ecologicalRole(p.genomes[i])*40+ry*8+rx].push_back(i);}
  size_t depth=0;while(display.size()<size_t(c.visual)){
    bool added=false;for(int role=0;role<5;role++)for(int cell=0;cell<40;cell++){
      auto const&bucket=survey[role*40+cell];if(depth<bucket.size()){size_t before=display.size();keep(bucket[depth]);added|=display.size()!=before;}}
    if(!added)break;
    depth++;
  }
  int cnt=0;for(int i:display){if(cnt++)out<<',';out<<'['<<p.uid[i]<<','<<p.x[i]<<','<<p.y[i]<<','<<p.heading[i]<<','<<p.energy[i]<<','<<p.generation[i]<<','<<p.genomes[i].body.size()<<','<<int(p.genomes[i].neurons)<<','<<p.age[i]<<","<<p.vx[i]<<","<<p.vy[i]<<",[";
  auto &genome=p.genomes[i];for(size_t b=0;b<genome.body.size();b++){auto &node=genome.body[b];if(b)out<<',';out<<'['<<int(node.parent)<<','<<int(node.type)<<','<<node.length<<','<<node.angle<<','<<node.growth<<','<<p.jointAt(i,b)<<','<<int(node.ctrl)<<','<<int(node.sense)<<','<<node.motor;if(c.morphogenesis>0.f||c.softBody>0.f||c.tissueCoupling>0.f||c.regulatoryNetwork>0.f)out<<','<<(expressed(i,b)?1:0);
  if(c.softBody>0.f){float strain=bound(1.f-.17f*fabs(p.jointAt(i,b))-.06f*fabs(p.velocityAt(i,b)),.62f,1.f);out<<','<<node.thickness<<','<<node.flexibility<<','<<(node.thickness*strain);}if(c.tissueCoupling>0.f&&c.softBody>0.f){out<<','<<node.conductance<<','<<node.stiffness<<','<<p.signalAt(i,b);}
  if(c.regulatoryNetwork>0.f){out<<','<<int(node.regulator)<<','<<node.regulatorWeight;}
  out<<']';}out<<"],"<<genome.roleBias<<","<<genome.collectivity<<","<<count_if(genome.edges.begin(),genome.edges.end(),[](Edge const&e){return e.from>=6&&e.from<64;})<<","<<genome.niche<<","<<genome.engineering<<","<<ecologicalRole(genome)<<","<<count_if(genome.edges.begin(),genome.edges.end(),[](Edge const&e){return e.from>=67&&e.from<69;});
  if(c.metabolicEcology>0.f){out<<",[";for(size_t b=0;b<p.cargoFood[i].size();b++){if(b)out<<',';out<<p.cargoFood[i][b];}out<<"],[";for(size_t b=0;b<p.cargoDetritus[i].size();b++){if(b)out<<',';out<<p.cargoDetritus[i][b];}out<<"],"<<genome.assimilationBias<<","<<genome.transportGene<<",[";
   for(size_t b=0;b<p.flowFood[i].size();b++){if(b)out<<',';out<<p.flowFood[i][b];}out<<"],[";
   for(size_t b=0;b<p.flowDetritus[i].size();b++){if(b)out<<',';out<<p.flowDetritus[i][b];}out<<"]";}
  out<<"]";}out<<']';
  // Read-only observer projection: sampled UIDs link real parent/ancestor genealogy
  // with the exact present hereditary body (never inferred role labels).
  out<<",\"lineageLinks\":[";
  int linkCount=0;for(int i:display){const auto&g=p.genomes[i];if(linkCount++)out<<',';
    out<<'['<<p.uid[i]<<','<<p.parentUid[i]<<','<<p.ancestor[i]<<','<<p.generation[i]
       <<",\""<<signature(g)<<"\","<<g.body.size()<<','
       <<count_if(g.body.begin(),g.body.end(),[](const Node&n){return n.type==2;})<<','
       <<count_if(g.body.begin(),g.body.end(),[](const Node&n){return n.type==5;})<<','
       <<count_if(g.body.begin(),g.body.end(),[](const Node&n){return n.type==7;})<<']';}
  out<<']';out<<",\"resourceGrid\":[";for(int gy=0;gy<10*c.worldScale;gy++)for(int gx=0;gx<16*c.worldScale;gx++){int ix=min(GX-1,int((gx+.5f)*GX/(16*c.worldScale)));int iy=min(GY-1,int((gy+.5f)*GY/(10*c.worldScale)));if(gy||gx)out<<',';out<<food[iy*GX+ix];}out<<']';out<<",\"habitatGrid\":[";float hp=float(tick)*.00021f,hc=cos(hp),hs=sin(hp);for(int gy=0;gy<10*c.worldScale;gy++)for(int gx=0;gx<16*c.worldScale;gx++){int ix=min(GX-1,int((gx+.5f)*GX/(16*c.worldScale)));int iy=min(GY-1,int((gy+.5f)*GY/(10*c.worldScale)));if(gy||gx)out<<',';out<<habitatMultiplier(iy*GX+ix,hc,hs);}out<<']';
 out<<",\"detritusGrid\":[";for(int gy=0;gy<10*c.worldScale;gy++)for(int gx=0;gx<16*c.worldScale;gx++){int ix=min(GX-1,int((gx+.5f)*GX/(16*c.worldScale)));int iy=min(GY-1,int((gy+.5f)*GY/(10*c.worldScale)));if(gy||gx)out<<',';out<<detritus[iy*GX+ix];}out<<']';out<<",\"soilGrid\":[";for(int gy=0;gy<10*c.worldScale;gy++)for(int gx=0;gx<16*c.worldScale;gx++){int ix=min(GX-1,int((gx+.5f)*GX/(16*c.worldScale)));int iy=min(GY-1,int((gy+.5f)*GY/(10*c.worldScale)));if(gy||gx)out<<',';out<<soil[iy*GX+ix];}out<<']';out<<",\"bonds\":[";for(size_t k=0;k<bonds.size()&&k<150;k++){auto const&b=bonds[k];if(k)out<<',';out<<'['<<b.a<<','<<b.b<<','<<b.age<<','<<b.transferred<<','<<int(b.origin)<<','<<b.groupId<<','<<b.parentGroupId<<','<<b.groupGeneration<<']';}out<<']';out<<",\"interactions\":[";for(size_t ii=0;ii<events.size();ii++){auto &ev=events[ii];if(ii)out<<',';out<<'['<<ev.tick<<','<<ev.a<<','<<ev.b<<','<<ev.type<<']';}out<<']';out<<",\"relationships\":[";size_t ri=0;vector<Interaction> rs;rs.reserve(relationships.size());for(auto const&entry:relationships)rs.push_back(entry.second);sort(rs.begin(),rs.end(),[](Interaction const&a,Interaction const&b){return a.tick!=b.tick?a.tick>b.tick:a.a!=b.a?a.a<b.a:a.b!=b.b?a.b<b.b:a.type<b.type;});for(auto const&ev:rs){if(ri++>=320)break;if(ri>1)out<<',';out<<'['<<ev.tick<<','<<ev.a<<','<<ev.b<<','<<ev.type<<','<<ev.count<<']';}out<<']';}
 out<<"}\n";out.flush();}
};
// Rate controls simulation throughput, NOT physical time. Zero means uncapped CPU throughput.
static atomic<int> runtimeRate{0};
static atomic<bool> runtimePaused{false};
static atomic<int> runtimeSteps{0};
static atomic<bool> runtimeCheckpoint{false};
static volatile sig_atomic_t shutdownRequested=0;
static void requestShutdown(int){shutdownRequested=1;}
int main(int argc,char**argv){ios::sync_with_stdio(false);Settings cfg;option(cfg,argc,argv);Sim sim(cfg);if(!cfg.resume.empty())try{sim.restore(cfg.resume);sim.c.organAblation=cfg.organAblation;sim.organLedgerSinceTick=sim.tick;}catch(exception const&e){cerr<<"Resume failed: "<<e.what()<<'\n';return 4;}
 // Apply experimental environment after restoring the immutable initial checkpoint.
 // Migration controls are runtime parameters and must be repeated for later resumes.
 if(cfg.experimentalResource>=0.f)sim.c.resourceRate=cfg.experimentalResource;
 sim.c.experimentalMigrationCost=cfg.experimentalMigrationCost;
 sim.c.experimentalIsolation=cfg.experimentalIsolation;
 // Intervention order is explicit: clone a checkpoint first, prune existing bodies,
 // optionally rescue reference-UID genotypes from a separate donor checkpoint.
 // The donor file is never written or mutated. Neither operation consumes RNG.
 if(cfg.resume.empty()&&(cfg.genomePrune!="none"||!cfg.genomeRescue.empty())){cerr<<"Genomic edits require --resume from a checkpoint\n";return 2;}
 if(cfg.genomePrune!="none")sim.pruneGenomes(cfg.genomePrune);
 if(!cfg.genomeRescue.empty()){
  try{Settings d=cfg;d.genomePrune="none";d.genomeRescue="";Sim donor(d);donor.restore(cfg.genomeRescue);sim.rescueGenomes(donor);}
  catch(exception const&e){cerr<<"Genome rescue failed: "<<e.what()<<'\n';return 4;}
 }
 std::signal(SIGTERM,requestShutdown);std::signal(SIGINT,requestShutdown);
 runtimeRate.store(cfg.rate);
 // The dedicated command reader must not flush partial JSON records on cout.
 // By default std::cin is tied to std::cout, so concurrent getline can split a record.
 std::cin.tie(nullptr);
 std::thread([]{std::string cmd;while(std::getline(std::cin,cmd)){
  if(cmd=="PAUSE")runtimePaused.store(true);
  else if(cmd=="RESUME")runtimePaused.store(false);
  else if(cmd=="STEP"){runtimePaused.store(true);runtimeSteps.fetch_add(1);}
  else if(cmd=="CHECKPOINT")runtimeCheckpoint.store(true);
  else if(cmd.rfind("RATE ",0)==0){try{int v=std::stoi(cmd.substr(5));if(v>=0&&v<=100000)runtimeRate.store(v);}catch(...){}}
 }}).detach();

 ofstream csv,record,lineage,orglog,fitnesslog,regionalAudit,eventAudit;bool csvExtended=true,csvV85=true,csvV86=true,csvV89=true,csvV90=true;
 if(cfg.resume.size()&&cfg.csv.size()){ifstream check(cfg.csv);string head;if(getline(check,head)){if(head.find("propagulesBorn")==string::npos)csvExtended=false;if(head.find("organizationLineages")==string::npos)csvV85=false;if(head.find("integratedEvents")==string::npos)csvV86=false;if(head.find("soilEdits")==string::npos)csvV89=false;if(head.find("extendedHarvests")==string::npos)csvV90=false;}}
 if(cfg.resume.size()&&cfg.lineage.size()){ifstream check(cfg.lineage);string head;if(getline(check,head)){if(head.find("role_bias")==string::npos)sim.lineageExtended=false;if(head.find("niche_preference")==string::npos)sim.lineageEcologyExtended=false;}}
 if(cfg.organizationFitnessLog.size()){
  fitnesslog.open(cfg.organizationFitnessLog,cfg.resume.empty()?ios::out:ios::app);
  if(!fitnesslog){cerr<<"Unable to create group fitness log\n";return 3;}
  if(cfg.resume.empty())fitnesslog<<"group_id,parent_group_id,generation,inherited_pair,start_tick,end_tick,lifespan,offspring_groups,energy_transferred,group_harvest,group_investment\n";
  sim.organizationFitnessLog=&fitnesslog;
 }
 if(cfg.organizationLog.size()){orglog.open(cfg.organizationLog,cfg.resume.empty()?ios::out:ios::app);if(!orglog){cerr<<"Unable to create organization log\n";return 3;}if(cfg.resume.empty())orglog<<"tick,group_id,parent_group_id,organization_generation,member_a,member_b,inherited_pair\n";sim.organizationLog=&orglog;}
 if(cfg.lineage.size()){lineage.open(cfg.lineage,cfg.resume.empty()?ios::out:ios::app);if(!lineage){cerr<<"Unable to create lineage CSV\n";return 3;}if(cfg.resume.empty())lineage<<"tick,individual_id,parent_id,root_ancestor,generation,structure_hash,body_nodes,neurons,connections,thermal_optimum,metabolism,role_bias,collectivity,propagule_birth,niche_preference,environment_engineering";if(sim.c.metabolicEcology>0.f)lineage<<",food_detritus_allocation,transport_gene,absorber_nodes,digester_nodes,root_nodes";lineage<<"\n";sim.lineageLog=&lineage;for(int i:cfg.resume.empty()?sim.live:vector<int>{}){const auto &g=sim.p.genomes[i];lineage<<0<<','<<sim.p.uid[i]<<","<<0<<","<<sim.p.ancestor[i]<<","<<0<<","<<sim.signature(g)<<","<<g.body.size()<<","<<int(g.neurons)<<","<<g.edges.size()<<','<<g.tempOpt<<','<<g.metabolism<<','<<g.roleBias<<','<<g.collectivity<<",0,"<<g.niche<<","<<g.engineering;if(sim.c.metabolicEcology>0.f)lineage<<","<<g.assimilationBias<<","<<g.transportGene<<","<<count_if(g.body.begin(),g.body.end(),[](Node const&n){return n.type==2;})<<","<<count_if(g.body.begin(),g.body.end(),[](Node const&n){return n.type==5;})<<","<<count_if(g.body.begin(),g.body.end(),[](Node const&n){return n.type==7;});lineage<<"\n";}}if(cfg.csv.size()){csv.open(cfg.csv,cfg.resume.empty()?ios::out:ios::app);if(!csv){cerr<<"Unable to create CSV\n";return 3;}if(cfg.resume.empty())csv<<"tick,population,maxGeneration,livingClasses,heritageClasses,meanNodes,meanNeurons,meanEnergy,births,deaths,exchanges,signals,predations,shares,activeBonds,propagulesBorn,propagulesMatured,organizationLineages,maxOrganizationGeneration,descendantOrganizationBirths,cooperativeDefenses,preventedLoss,integratedEvents,integratedHarvest,integratedCost,productiveOrganizations,soilEdits,soilMineralizations,nicheHarvests,developmentMutations,recycledMatter,ecologicalHarvest,extendedHarvests,extendedEnergy\n";}if(cfg.record.size()){record.open(cfg.record,cfg.resume.empty()?ios::out:ios::app);if(!record){cerr<<"Unable to create JSONL\n";return 3;}}
 if(sim.c.historyLedger){
   if(!cfg.regionalLedger.empty()){regionalAudit.open(cfg.regionalLedger,cfg.resume.empty()?ios::out:ios::app);if(!regionalAudit){cerr<<"Cannot open regional ledger\n";return 3;}if(cfg.resume.empty())regionalAudit<<"tick,region,living,energy,nutrient,detritus,signal,uid_hash,event_count,event_hash\n";}
   if(!cfg.eventLedger.empty()){eventAudit.open(cfg.eventLedger,cfg.resume.empty()?ios::out:ios::app);if(!eventAudit){cerr<<"Cannot open event ledger\n";return 3;}if(cfg.resume.empty())eventAudit<<"tick,uid_a,uid_b,type,rolling_hash\n";sim.eventLedgerLog=&eventAudit;}
 }
 cout<<"{\"type\":\"hello\",\"engine\":\"native-cpp17-genomic-causality-v10.10\",\"seed\":"<<cfg.seed<<",\"initial\":"<<cfg.initial<<",\"capacity\":"<<cfg.cap<<",\"ticks\":"<<cfg.ticks<<",\"ablate\":\""<<cfg.ablate<<"\",\"organAblation\":\""<<cfg.organAblation<<"\",\"neutral\":"<<(cfg.neutral?"true":"false")<<"}\n"<<flush;
 auto nextStep=std::chrono::steady_clock::now();int previousRate=-1;
 for(uint64_t t=sim.tick+1;t<=uint64_t(cfg.ticks);t++){
  if(shutdownRequested)break;
  while(runtimePaused.load()&&runtimeSteps.load()<=0&&!shutdownRequested){
    if(runtimeCheckpoint.exchange(false)&&!cfg.checkpoint.empty()){
      sim.checkpoint(cfg.checkpoint);
      cout<<"{\"type\":\"checkpoint\",\"tick\":"<<sim.tick<<"}\n"<<flush;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  if(shutdownRequested)break;
  bool manualStep=runtimePaused.load();if(manualStep&&runtimeSteps.load()>0)runtimeSteps.fetch_sub(1);
  if(!manualStep){
    int rate=runtimeRate.load();if(rate!=previousRate){previousRate=rate;nextStep=std::chrono::steady_clock::now();}
    if(rate>0){
      while(!runtimePaused.load()&&!shutdownRequested&&std::chrono::steady_clock::now()<nextStep){
        auto remain=nextStep-std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::min(std::chrono::milliseconds(20),std::chrono::duration_cast<std::chrono::milliseconds>(std::max(remain,decltype(remain)::zero()))));
      }
      if(runtimePaused.load()){--t;continue;}
      auto now=std::chrono::steady_clock::now();
      nextStep=std::max(nextStep,now)+std::chrono::microseconds(std::max(1,1000000/rate));
    }
  }
  try{sim.evolve();}catch(std::exception const&err){cerr<<"Simulation step failed: "<<err.what()<<"\n";return 6;}
  if(runtimeCheckpoint.exchange(false)&&!cfg.checkpoint.empty()){
      sim.checkpoint(cfg.checkpoint);
      cout<<"{\"type\":\"checkpoint\",\"tick\":"<<sim.tick<<"}\n"<<flush;
    }
  if(cfg.checkpointEvery>0&&t%cfg.checkpointEvery==0&&!cfg.checkpoint.empty()){
    sim.checkpoint(cfg.checkpoint);
    cout<<"{\"type\":\"checkpoint\",\"tick\":"<<sim.tick<<"}\n"<<flush;
  }
  if(t==1||manualStep||(runtimeRate.load()>0&&runtimeRate.load()<=10&&t%uint64_t(runtimeRate.load()>3?2:1)==0)||t%cfg.sample==0||t==uint64_t(cfg.ticks)||sim.live.empty()){if(regionalAudit)sim.regionalAudit(regionalAudit);ostringstream r;sim.report(r);string txt=r.str();cout<<txt;if(record)record<<txt;if(csv){int n=sim.live.size();double body=0,neur=0,eng=0;unordered_set<uint64_t>liveKinds;for(int i:sim.live){auto&g=sim.p.genomes[i];body+=g.body.size();neur+=g.neurons;eng+=sim.p.energy[i];liveKinds.insert(sim.signature(g));}csv<<sim.tick<<','<<n<<','<<sim.maxGen<<','<<liveKinds.size()<<','<<sim.heritage.size()<<','<<(n?body/n:0)<<','<<(n?neur/n:0)<<','<<(n?eng/n:0)<<','<<sim.births<<','<<sim.deaths<<','<<sim.exchanges<<','<<sim.signals<<','<<sim.predations<<','<<sim.shares;if(csvExtended)csv<<','<<sim.bonds.size()<<','<<sim.propagulesBorn<<','<<sim.propagulesMatured;if(csvV85)csv<<','<<sim.nextGroupId<<','<<sim.maxGroupGeneration<<','<<sim.descendantGroupBirths<<','<<sim.cooperativeDefenses<<','<<sim.preventedLoss;if(csvV86)csv<<','<<sim.integratedEvents<<','<<sim.integratedHarvest<<','<<sim.integratedCost<<','<<sim.productiveOrganizations;if(csvV89)csv<<','<<sim.soilEdits<<','<<sim.soilMineralizations<<','<<sim.nicheHarvests<<','<<sim.developmentMutations<<','<<sim.recycledMatter<<','<<sim.ecologicalHarvest;if(csvV90)csv<<','<<sim.extendedHarvests<<','<<sim.extendedEnergy;csv<<'\n';}}
 if(sim.live.empty())break;}
 if(!cfg.checkpoint.empty())sim.checkpoint(cfg.checkpoint);
 cout<<"{\"type\":\"complete\",\"tick\":"<<sim.tick<<",\"population\":"<<sim.live.size()<<",\"maxGeneration\":"<<sim.maxGen<<",\"births\":"<<sim.births<<",\"deaths\":"<<sim.deaths<<",\"rngState\":"<<sim.rng.state<<"}\n";return 0;
}
