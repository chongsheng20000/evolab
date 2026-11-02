from pathlib import Path
p=Path(__file__).parents[1]/'native'/'omega_core.cpp'
s=p.read_text()
def rep(old,new,n=1):
 global s
 count=s.count(old)
 if count!=n:raise SystemExit(f'Pattern mismatch {count} vs {n}: {old[:160]!r}')
 s=s.replace(old,new)
# Variation between inherited capability and perturbation: no hard coded evolutionary endpoint.
rep('string ablate="none",csv=""','string organAblation="none",ablate="none",csv=""')
rep('if(s=="--metabolic-ecology")p.metabolicEcology=stof(v);','if(s=="--organ-ablation")p.organAblation=v;else if(s=="--metabolic-ecology")p.metabolicEcology=stof(v);')
rep('if(p.rate<0||p.rate>100000||', 'if(!(p.organAblation=="none"||p.organAblation=="absorber"||p.organAblation=="digester"||p.organAblation=="root")||p.rate<0||p.rate>100000||')
rep('array<double,3> functionalUptake{};double anchorVelocityLoss=0;', '''array<double,3> functionalUptake{};double anchorVelocityLoss=0;
  // Observer-only event ledger for expressed inherited organ carriers and noncarriers.
  // Arrays indexed by organ (absorber/digester/root) and presence (0=no,1=yes).
  array<array<uint64_t,2>,3> organExposure{},organOffspring{},organDeaths{},organInheritedBirths{};
  array<array<uint64_t,2>,3> organParentBirths{};
  static int organIndex(uint8_t t){return t==2?0:t==5?1:t==7?2:-1;}
  bool carrier(int i,int k,bool mature=false)const{auto const&g=p.genomes[i];
    for(size_t b=1;b<g.body.size();b++)if(organIndex(g.body[b].type)==k&&(!mature||expressed(i,b)))return true;
    return false;}
  bool organDisabled(int type)const{return (type==2&&c.organAblation=="absorber")||(type==5&&c.organAblation=="digester")||(type==7&&c.organAblation=="root");}
  void recordBirth(int id,bool founder){if(founder)return;for(int k=0;k<3;k++)organInheritedBirths[k][carrier(id,k)?1:0]++;}
  void recordReproduction(int id){for(int k=0;k<3;k++)organParentBirths[k][carrier(id,k)?1:0]++;}
  void recordDeath(int id){for(int k=0;k<3;k++)organDeaths[k][carrier(id,k)?1:0]++;}
  void recordExposure(){for(int id:live)for(int k=0;k<3;k++)organExposure[k][carrier(id,k)?1:0]++;}
''')
rep('live.push_back(i);maxGen=max(maxGen,gen);', 'live.push_back(i);recordBirth(i,parentUID==0);maxGen=max(maxGen,gen);')
rep('void evolve(){tick++;functionalUptake={0.,0.,0.};', 'void evolve(){recordExposure();tick++;functionalUptake={0.,0.,0.};')
rep('if(c.metabolicEcology>0.f){\n     float rootContact=0.f;', 'if(c.metabolicEcology>0.f&&c.organAblation!="root"){\n     float rootContact=0.f;')
rep('if(c.ecology<=0.f||!(node.type==2||node.type==5||node.type==7))continue;', 'if(c.ecology<=0.f||organDisabled(node.type)||!(node.type==2||node.type==5||node.type==7))continue;')
rep('if(node.type!=2&&node.type!=5&&node.type!=7)continue;', 'if(node.type!=2&&node.type!=5&&node.type!=7)continue;\n       if(organDisabled(node.type))continue;')
rep('p.offspring[i]++;births++;}}', 'p.offspring[i]++;recordReproduction(i);births++;}}')
rep('p.alive[i]=0;deaths++;', 'recordDeath(i);p.alive[i]=0;deaths++;')
rep('p.offspring[i]++;p.offspring[j]++;births+=2;', 'p.offspring[i]++;p.offspring[j]++;recordReproduction(i);recordReproduction(j);births+=2;')
rep('out<<"],\\"anchorVelocityLoss\\":"<<anchorVelocityLoss;', '''out<<"],\\"anchorVelocityLoss\\":"<<anchorVelocityLoss;
    out<<",\\"organAblation\\":\\""<<c.organAblation<<"\\"";
    out<<",\\"organCompetition\\":[";
    for(int k=0;k<3;k++){
      if(k)out<<',';out<<'[';
      // [exposureNo,exposureYes,offspringParentNo,offspringParentYes,deathNo,deathYes,
      //  newbornGenotypeNo,newbornGenotypeYes] : lifetime event counts since start, not taxonomy.
      for(int present=0;present<2;present++){if(present)out<<',';out<<organExposure[k][present];}
      for(int present=0;present<2;present++)out<<','<<organParentBirths[k][present];
      for(int present=0;present<2;present++)out<<','<<organDeaths[k][present];
      for(int present=0;present<2;present++)out<<','<<organInheritedBirths[k][present];
      out<<']';
    }out<<']';''')
# restore keeps original physical world settings; intervention is explicitly nonserialized.
rep('if(!cfg.resume.empty())try{sim.restore(cfg.resume);}catch(exception const&e)', 'if(!cfg.resume.empty())try{sim.restore(cfg.resume);sim.c.organAblation=cfg.organAblation;}catch(exception const&e)')
rep('<<",\\"ablate\\":\\""<<cfg.ablate<<"\\",\\"neutral\\":"', '<<",\\"ablate\\":\\""<<cfg.ablate<<"\\",\\"organAblation\\":\\""<<cfg.organAblation<<"\\",\\"neutral\\":"')
p.write_text(s)
print('patched',p)
