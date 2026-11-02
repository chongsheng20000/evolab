#!/usr/bin/env python3
"""EvoLab 10.11 preregistered environmental contrast study, no seed selection.

Per seed all branches resume an identical SHA-256 checkpoint at tick 300. A
physical habitat is not a chosen evolution outcome: only world resource supply,
regional movement cost, and gate width differ. Structural root-pruning is a
specific, multiply-confounded perturbation, not a proof of root adaptive fitness.
"""
import argparse, csv, hashlib, json, pathlib, statistics, subprocess, tempfile, time
ROOT=pathlib.Path(__file__).resolve().parents[1]
CORE=ROOT/'native'/'omega_core'
MANIFEST=tuple(range(13101,13125))
SCENARIOS={
 'reference': {'resource':.65,'cost':0,'isolation':0},
 'scarce': {'resource':.16,'cost':0,'isolation':0},
 'abundant': {'resource':1.6,'cost':0,'isolation':0},
 'migration_cost': {'resource':.65,'cost':2,'isolation':0},
 'isolated_regions': {'resource':.65,'cost':0,'isolation':.92},
}
BASE=['--initial','75','--capacity','145','--sample','250','--visual','5','--rate','0',
 '--world-scale','2','--metabolic-ecology','1','--ecology','1','--morphogenesis','1',
 '--soft-body','1','--tissue-coupling','1','--regionality','1', '--transport','1',
 '--resource','.65','--mutation','.13']

def run(seed, end, flags):
 cmd=[str(CORE),'--seed',str(seed),'--ticks',str(end),*BASE]
 for name,val in flags.items():cmd.extend(['--'+name.replace('_','-'),str(val)])
 p=subprocess.run(cmd,input='',text=True,capture_output=True,timeout=110)
 if p.returncode:raise RuntimeError(f'{seed} {flags}: {p.returncode}: {p.stderr[-750:]}')
 states=[json.loads(line) for line in p.stdout.splitlines() if line.startswith('{"type":"state"')]
 if not states or states[-1]['tick']!=end:raise RuntimeError(f'{seed} {flags}: missing final state')
 return states

def main(opts):
 start=time.monotonic();out=opts.out;out.mkdir(parents=True,exist_ok=True)
 seeds=MANIFEST[opts.start:opts.start+opts.seeds]
 assert len(seeds)==opts.seeds and opts.horizon>350
 records=[];traces=[];hashes=[]
 for seed in seeds:
  with tempfile.TemporaryDirectory(prefix=f'evolab-env-{seed}-') as tmp:
   path=pathlib.Path(tmp)/'seed300.bin'
   first=run(seed,300,{'checkpoint':str(path),'checkpoint_every':300})[-1]
   assert path.exists()
   digest=hashlib.sha256(path.read_bytes()).hexdigest()
   hashes.append({'seed':seed,'checkpoint_sha256':digest,'checkpoint_size_bytes':path.stat().st_size})
   for scenario,conditions in SCENARIOS.items():
    variants=['control','prune_root']+(['function_root_off'] if scenario=='reference' else [])
    for variant in variants:
     flags={'resume':str(path),'experiment_resource':conditions['resource'],
       'experiment_migration_cost':conditions['cost'],'experiment_isolation':conditions['isolation']}
     if variant=='prune_root':flags['genome_prune']='root'
     if variant=='function_root_off':flags['organ_ablation']='root'
     states=run(seed,opts.horizon,flags)
     end=states[-1]
     if variant=='prune_root' and end['genomePrunedNodes']<=0:raise AssertionError(f'no root to prune {seed}')
     if end['resourceRate']!=round(conditions['resource'],4):raise AssertionError('resource override did not apply')
     if end['experimentalIsolation']!=conditions['isolation']:raise AssertionError('isolation override did not apply')
     if variant=='function_root_off' and end['functionalUptake'][2]!=0:raise AssertionError('root functional knockout failed')
     # Determinism: repeat reference control using independently executed process.
     if scenario=='reference' and variant=='control':
      replay=run(seed,opts.horizon,flags)
      if end!=replay[-1]:raise AssertionError(f'nondeterministic replay: {seed}')
     rec={'seed':seed,'scenario':scenario,'branch':variant,'checkpoint_sha256':digest,
        'start_tick':300,'horizon_tick':opts.horizon,'resource_replenishment':conditions['resource'],
        'migration_energy_cost':conditions['cost'],'isolation_fraction':conditions['isolation'],
        'births_since_checkpoint':end['births']-first['births'],
        'deaths_since_checkpoint':end['deaths']-first['deaths'],
        'population':end['population'],'max_generation':end['maxGeneration'],
        'body_architectures':end['bodyArchitectures'],'living_structure_classes':end['livingClasses'],
        'mean_energy':end['meanEnergy'],'regional_migration_events':end['regionalMigrants']-first['regionalMigrants'],
        'blocked_crossings':end['blockedCrossings'],'migration_energy_paid':end['migrationEnergyPaid'],
        'absorber_carriers':end['functionalCarriers'][0], 'digester_carriers':end['functionalCarriers'][1],
        'root_carriers':end['functionalCarriers'][2],
        'root_uptake':end['functionalUptake'][2],
        'root_pruned_genomes':end['genomePrunedLives'],
        'root_pruned_nodes':end['genomePrunedNodes'],
        'organ_competition':json.dumps(end['organCompetition'],separators=(',',':'))}
     records.append(rec)
     for state in states:
      traces.append({'seed':seed,'scenario':scenario,'branch':variant,'tick':state['tick'],
        'births':state['births'],'deaths':state['deaths'],'population':state['population'],
        'generation':state['maxGeneration'],'root_carriers':state['functionalCarriers'][2],
        'root_uptake':state['functionalUptake'][2],
        'regional_migration_events':state['regionalMigrants'],
        'blocked_crossings':state['blockedCrossings'],'mean_energy':state['meanEnergy']})
   print(f'{seed} complete: {len(SCENARIOS)} regimes, checkpoint {digest[:13]}, {time.monotonic()-start:.1f}s',flush=True)
 def csvout(filename,items):
  with (out/filename).open('w',newline='',encoding='utf8') as f:
   w=csv.DictWriter(f,fieldnames=list(items[0]));w.writeheader();w.writerows(items)
 csvout('branches.csv',records);csvout('trajectories.csv',traces);csvout('checkpoint_hashes.csv',hashes)
 keyed={(r['seed'],r['scenario'],r['branch']):r for r in records}
 summaries=[]
 for scenario in SCENARIOS:
  for field in ('births_since_checkpoint','deaths_since_checkpoint','max_generation','root_carriers','root_uptake','regional_migration_events','blocked_crossings'):
   deltas=[keyed[(seed,scenario,'prune_root')][field]-keyed[(seed,scenario,'control')][field] for seed in seeds]
   summaries.append({'scenario':scenario,'metric':field,'n_seed_pairs':len(seeds),
      'mean_pruned_minus_control':round(statistics.fmean(deltas),6),
      'median_pruned_minus_control':round(statistics.median(deltas),6),
      'positive':sum(x>0 for x in deltas),'negative':sum(x<0 for x in deltas),'equal':sum(x==0 for x in deltas),
      'paired_deltas':deltas})
 report={'model_version':'10.11','status':'exploratory within-model evidence, not open-ended evolution proof',
   'fixed_seeds':list(seeds),'predeclared_seed_universe':list(MANIFEST),'checkpoint_tick':300,
   'horizon_tick':opts.horizon,'world_scale':2,'population_capacity':145,'conditions':SCENARIOS,
   'branch_count':len(records),'trace_count':len(traces),'experiment_duration_seconds':round(time.monotonic()-start,2),
   'comparisons':summaries,'methodological_limits':[
      'Intervention branches share an initial checkpoint within each seed, not a single interacting world after branching.',
      'Structural pruning removes root nodes and their dependent descendants, and changes several physical functions.',
      'Root benefit cannot be read from birth count alone, especially near capacity.',
      'Cross-environment resource and barrier settings are researcher interventions, not evolved traits.',
      'Barriers are imposed at 1300/850 unit cell boundaries with corridor width determined by isolation.',
      'Migration cost is metabolic loss at an actual allowed regional crossing; blocked events have no such cost.',
      'Only sampled live organisms appear in the GUI genealogy; full births require native lineage CSV.',
      'Model mechanisms, finite seeds and horizon cannot demonstrate sustained adaptive innovation or new species.']}
 csvout('paired_summary.csv',[{k:json.dumps(v) if isinstance(v,list) else v for k,v in x.items()} for x in summaries])
 (out/'summary.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
 print('PASS',len(records),'branches,',len(traces),'trace records,',len(seeds),'seeds;',round(time.monotonic()-start,1),'sec',flush=True)

if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--start',type=int,default=0);parser.add_argument('--seeds',type=int,default=6);parser.add_argument('--horizon',type=int,default=2200);parser.add_argument('--out',type=pathlib.Path,default=ROOT/'evidence'/'v1011_multienv')
 args=parser.parse_args();assert args.seeds>0 and 0<=args.start<len(MANIFEST) and args.start+args.seeds<=len(MANIFEST)
 main(args)
