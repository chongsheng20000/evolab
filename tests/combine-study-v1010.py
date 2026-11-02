#!/usr/bin/env python3
"""Merge finite batch evidence into a single seed-complete, auditable report."""
import argparse, csv, json, pathlib, random, statistics, math
ROOT=pathlib.Path(__file__).resolve().parents[1]
SEEDS=set(range(12001,12013))
BRANCHES={'control','functional_root_off','genomic_root_prune','genomic_root_prune_then_rescue','genomic_root_prune_unrescued_330'}
INT_FIELDS={'seed','start_tick','horizon_tick','start_pop','end_pop','start_births','births_since_start','start_deaths','deaths_since_start','start_generation','max_generation','root_carriers','absorber_carriers','digester_carriers','genome_pruned_lives_on_resume','genome_pruned_nodes_on_resume','genome_rescued_lives_on_resume','genome_rescued_nodes_on_resume'}
FLOAT_FIELDS={'mean_energy','root_functional_uptake_since_branch'}

def read_csv(f):
  with f.open(newline='') as inp:return list(csv.DictReader(inp))

def write_csv(f,records):
  with f.open('w',newline='') as out:
    w=csv.DictWriter(out,fieldnames=list(records[0]));w.writeheader();w.writerows(records)

def qtile(x,p):
  z=sorted(x);i=p*(len(z)-1);j=int(i);return z[j]*(1-(i-j))+z[min(j+1,len(z)-1)]*(i-j)

def ci_bootstrap(deltas,replicates=5000):
  rng=random.Random(1010);n=len(deltas)
  means=[sum(rng.choice(deltas) for _ in range(n))/n for _ in range(replicates)]
  return [round(qtile(means,0.025),3),round(qtile(means,0.975),3)]

def merge(indir,outdir,horizon):
  outdir.mkdir(parents=True,exist_ok=True)
  folders=sorted(indir.glob(f'{horizon}_part*'))
  assert folders, 'No input batches'
  records=[];trace=[];hashes=[]
  for folder in folders:
    records+=read_csv(folder/'v1010_long_horizon_branches.csv')
    trace+=read_csv(folder/'v1010_long_horizon_trajectories.csv')
    hashes+=read_csv(folder/'v1010_checkpoint_hashes.csv')
  for r in records:
    for k in INT_FIELDS:r[k]=int(r[k])
    for k in FLOAT_FIELDS:r[k]=float(r[k])
    assert r['horizon_tick']==horizon
  keys={(r['seed'],r['branch']) for r in records}
  assert len(keys)==len(records), 'Duplicate seed/branch'
  seen={r['seed'] for r in records}
  assert seen==SEEDS, f'Missing or extra seeds: {seen^SEEDS}'
  assert keys=={(seed,branch) for seed in SEEDS for branch in BRANCHES}, 'Missing branches'
  hash_by={int(h['seed']):h['sha256'] for h in hashes};assert set(hash_by)==SEEDS
  assert all(hash_by[r['seed']]==r['checkpoint_sha256'] for r in records), 'Checkpoint mismatch'
  records.sort(key=lambda r:(r['seed'],r['branch']))
  trace.sort(key=lambda r:(int(r['seed']),r['branch'],int(r['tick'])))
  hashes.sort(key=lambda r:int(r['seed']))
  write_csv(outdir/'branches.csv',records)
  write_csv(outdir/'trajectories.csv',trace)
  write_csv(outdir/'checkpoint_hashes.csv',hashes)
  lookup={(r['seed'],r['branch']):r for r in records}
  fields=['births_since_start','deaths_since_start','end_pop','mean_energy','root_carriers','max_generation']
  summary={
    'engine':'Native C++ genome topology intervention (not a fixed evolutionary tech tree)',
    'checkpoint_tick':300,'prune_tick':300,'rescue_tick':330,'end_tick':horizon,'seeds':sorted(SEEDS),
    'design':'Within-seed branches from identical SHA256 checkpoint; 5 branch conditions; no favorable-seed exclusion; independently repeated control process must match exactly',
    'branch_count':len(records),'data_rows':len(trace),'compared_to_control':{},
    'limits':[
      'The test is a model-internal causal perturbation, not a proof of long-run adaptive innovation.',
      'Pruning removes a functional node and all its hereditary descendants: other subordinate functions may also be lost.',
      'Genome rescue only restores a reference genome to still-living source UIDs; offspring born after prune do not get artificial rescue.',
      'Rescue necessarily resets affected kinematic state and releases remaining cargo to local grid (conserves matter but introduces a rescue-specific disturbance).',
      'Simulations are capacity-limited and the habitat regime is fixed across seeds; replicate selection statistics across different environments before fitness claims.',
      'Associations between a genetic organ and birth/death rate have survivor, ancestry and environmental confounding.'
    ]}
  for branch in sorted(BRANCHES-{'control'}):
    s={}
    for field in fields:
      deltas=[lookup[(seed,branch)][field]-lookup[(seed,'control')][field] for seed in sorted(SEEDS)]
      s[field]={'mean_delta':round(statistics.fmean(deltas),3),'median_delta':round(statistics.median(deltas),3),
                'positive':sum(v>0 for v in deltas),'negative':sum(v<0 for v in deltas),'zero':sum(v==0 for v in deltas),
                'bootstrap_mean_delta_95pct_exploratory':ci_bootstrap(deltas),
                'paired_deltas':deltas}
    summary['compared_to_control'][branch]=s
  # Observational genotype carrier fertility/mortality rates, not causal estimates.
  rates={}
  for branch in sorted(BRANCHES):
    carExp=carBirth=carDeath=nonExp=nonBirth=nonDeath=0
    for seed in sorted(SEEDS):
      r=lookup[(seed,branch)];g=json.loads(r['organ_competition'])[2]
      nonExp+=g[0];carExp+=g[1];nonBirth+=g[2];carBirth+=g[3];nonDeath+=g[4];carDeath+=g[5]
    rates[branch]={
       'carrier_life_ticks':carExp,'noncarrier_life_ticks':nonExp,
       'carrier_parent_births_per_1000_life_ticks':round(1000*carBirth/carExp,4) if carExp else None,
       'noncarrier_parent_births_per_1000_life_ticks':round(1000*nonBirth/nonExp,4) if nonExp else None,
       'carrier_deaths_per_1000_life_ticks':round(1000*carDeath/carExp,4) if carExp else None,
       'noncarrier_deaths_per_1000_life_ticks':round(1000*nonDeath/nonExp,4) if nonExp else None}
  summary['observational_root_rates']=rates
  summary['recovery_counts']={str(seed):lookup[(seed,'genomic_root_prune_then_rescue')]['genome_rescued_lives_on_resume'] for seed in sorted(SEEDS)}
  (outdir/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
  print('PASS',len(SEEDS),'seeds',len(records),'branches',len(trace),'trajectory rows','at',horizon)
  for branch,s in summary['compared_to_control'].items():
    print(branch,'birth_delta=',s['births_since_start']['mean_delta'], 'birth sign (+/-/0)=',s['births_since_start']['positive'],s['births_since_start']['negative'],s['births_since_start']['zero'], 'root_carrier_delta=',s['root_carriers']['mean_delta'])
  return summary
if __name__=='__main__':
  a=argparse.ArgumentParser();a.add_argument('--batch-root',type=pathlib.Path,default=ROOT/'evidence'/'v1010_batches');a.add_argument('--out',type=pathlib.Path,default=ROOT/'evidence'/'v1010_causal_study');a.add_argument('--horizon',type=int,default=6000)
  o=a.parse_args();merge(o.batch_root,o.out,o.horizon)
