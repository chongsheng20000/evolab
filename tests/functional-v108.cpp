// Controlled mechanism-level checks for functional organ effects.
// Reuses production Sim implementation directly without a test-only physics model.
#define main evolab_production_cli_main
#include "../native/omega_core.cpp"
#undef main
#include <cassert>
#include <iostream>

struct Run {double food,detritus,root,speed,drag;size_t bodies;uint64_t nodeHash;};
static Run trial(int organ, float length=14.f, float thickness=2.4f) {
  Settings cfg;cfg.seed=8822;cfg.initial=1;cfg.cap=10;cfg.ticks=100;cfg.sample=10;cfg.visual=0;
  cfg.metabolicEcology=1;cfg.ecology=1;cfg.morphogenesis=1;cfg.softBody=1;cfg.grand=0;
  cfg.resourceRate=0;cfg.season=0;cfg.cohesion=0;cfg.specialization=0;cfg.predation=0;
  Sim sim(cfg);
  int id=sim.live.front();auto &g=sim.p.genomes[id];
  g.body.resize(2);g.body[1]=Node{0,uint8_t(organ),0,0,length,0.f,0.f,1.f};g.body[1].thickness=thickness;
  sim.p.allocateLife(id,int(g.body.size()),g.neurons);
  sim.p.cargoFood[id].assign(2,0.f);sim.p.cargoDetritus[id].assign(2,0.f);
  sim.p.flowFood[id].assign(2,0.f);sim.p.flowDetritus[id].assign(2,0.f);
  sim.p.x[id]=650.f;sim.p.y[id]=425.f;sim.p.energy[id]=10.f;sim.p.age[id]=160.f;
  std::fill(sim.food.begin(),sim.food.end(),20.f);
  std::fill(sim.detritus.begin(),sim.detritus.end(),20.f);
  std::fill(sim.soil.begin(),sim.soil.end(),.8f);
  // A mutation-free child inherits its organ topology exactly, with no recipe for role.
  uint64_t hash=sim.signature(g);
  sim.c.mutation=0.f;Genome child=sim.mutate(g);
  assert(sim.signature(child)==hash&&child.body.size()==g.body.size()&&child.body[1].type==organ);
  sim.evolve();
  std::ostringstream out;sim.report(out,false);
  assert(out.str().find("\"functionalUptake\"")!=std::string::npos);
  return {sim.functionalUptake[0],sim.functionalUptake[1],sim.functionalUptake[2],
          std::hypot(sim.p.vx[id],sim.p.vy[id]),sim.anchorVelocityLoss,child.body.size(),hash};
}
int main(){
  Run absorber=trial(2),digester=trial(5),root=trial(7);
  assert(absorber.food>0&&absorber.detritus==0&&absorber.root==0);
  assert(digester.detritus>0&&digester.food==0&&digester.root==0);
  assert(root.root>0&&root.food==0&&root.detritus==0);
  assert(root.speed<absorber.speed&&root.drag>0);
  assert(root.root>absorber.food); // genuinely distinct uptake given matched resources
  Run thin=trial(7,14.f,.7f),thick=trial(7,14.f,6.f);
  assert(thick.speed<thin.speed); // inherited geometry is mechanically meaningful
  std::cout<<"FUNCTIONAL CAUSAL TEST PASS\n"
    <<"matched absorber food="<<absorber.food<<"; digester detritus="<<digester.detritus
    <<"; root food="<<root.root<<"\n"
    <<"root speed="<<root.speed<<"; absorber speed="<<absorber.speed
    <<"; thin-root speed="<<thin.speed<<"; thick-root speed="<<thick.speed<<"\n";
}
