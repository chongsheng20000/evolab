#!/usr/bin/env python3
"""Deterministic, matched starting world / organ functional ablation tests.
A paired branch from a checkpoint retains genotype and all saved world state.
No claim is made that a single short run establishes adaptive fitness.
"""
import subprocess, json, tempfile, pathlib, statistics, csv
ROOT=pathlib.Path(__file__).resolve().parents[1]
BIN=ROOT/'native'/'omega_core'
def run(seed=49391,ticks=600,ablate='none',resume='',checkpoint='',rate=0):
 args=[str(BIN),'--seed',str(seed),'--initial','80','--capacity','160',
   '--ticks',str(ticks),'--sample','150','--visual','40','--rate',str(rate),
   '--world-scale','1','--metabolic-ecology','1','--ecology','1',
   '--morphogenesis','1','--soft-body','1','--tissue-coupling','1',
   '--resource','0.65','--organ-ablation',ablate]
 if resume:args+=['--resume',str(resume)]
 if checkpoint:args+=['--checkpoint',str(checkpoint),'--checkpoint-every','300']
 p=subprocess.run(args,input='',capture_output=True,text=True,timeout=75)
 if p.returncode:raise AssertionError(f'engine failed ({p.returncode}): {p.stderr[-800:]}')
 states=[json.loads(x) for x in p.stdout.splitlines() if x.startswith('{"type":"state"')]
 assert states, 'no native states'
 assert all(len(s['organCompetition'])==3 and all(len(c)==8 for c in s['organCompetition']) for s in states)
 assert all(len(s['functionalUptake'])==3 for s in states)
 assert states[-1]['organAblation']==ablate
 assert all(all(v>=0 for v in c) for s in states for c in s['organCompetition'])
 assert all(all(c[0]+c[1]>=c[2]+c[3] for c in s['organCompetition']) for s in states)
 return states

if __name__=='__main__':
 with tempfile.TemporaryDirectory(prefix='evolab-v109-') as temp:
  cp=pathlib.Path(temp)/'same_world.bin'
  seed=49391
  initial=run(seed,ticks=300,checkpoint=cp)
  assert cp.exists() and cp.stat().st_size>1000
  reference=run(seed,ticks=750,resume=cp)
  replay=run(seed,ticks=750,resume=cp)
  # Deterministic native replay with identical checkpoint and same intervention.
  assert reference[-1]==replay[-1]
  rootOff=run(seed,ticks=750,resume=cp,ablate='root')
  foodOff=run(seed,ticks=750,resume=cp,ablate='absorber')
  detOff=run(seed,ticks=750,resume=cp,ablate='digester')
  assert reference[-1]['organLedgerSinceTick']==300
  for branch in [rootOff,foodOff,detOff]:assert branch[-1]['organLedgerSinceTick']==300
  assert rootOff[-1]['anchorVelocityLoss']==0
  assert rootOff[-1]['functionalUptake'][2]==0
  assert foodOff[-1]['functionalUptake'][0]==0
  assert detOff[-1]['functionalUptake'][1]==0
  for k,br in enumerate([foodOff,detOff,rootOff]):
   assert any(st['functionalUptake'][k]>0 for st in reference), 'the control must show native flux'
   assert all(st['functionalUptake'][k]==0 for st in br), 'ablation must zero native flux'
  # Outcome can change; we don't require a beneficial effect from every organ in every world.
  assert any(branch[-1]['population']!=reference[-1]['population'] or branch[-1]['births']!=reference[-1]['births'] or branch[-1]['deaths']!=reference[-1]['deaths'] for branch in [rootOff,foodOff,detOff]), 'no emergent survival difference in paired branches'
  for name,states in [('control',reference),('root-disabled',rootOff),('food-disabled',foodOff),('detritus-disabled',detOff)]:
   s=states[-1]
   print(f'{name:19s} tick={s["tick"]} alive={s["population"]} cumulative_births={s["births"]} deaths={s["deaths"]} root_uptake={s["functionalUptake"][2]:.3f} offspring_records={sum(x[6]+x[7] for x in s["organCompetition"])}')
  # Additional independent seeds test that structures are inherited and competing in multiple worlds.
  for seed in [11301,11302,11303]:
   s=run(seed,ticks=850)[-1]
   assert s['maxGeneration']>=2
   assert sum(x[7] for x in s['organCompetition'])>0
   assert all(sum(c[0:2])>0 for c in s['organCompetition'])
   print('seed',seed,'generation',s['maxGeneration'],'alive',s['population'],'carriers',s['functionalCarriers'])
 print('PASS v10.9: same-checkpoint replay, organ ablation, resource flux, birth/death ledger, lineage evidence')
