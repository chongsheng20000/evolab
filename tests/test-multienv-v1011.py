#!/usr/bin/env python3
"""Actual native engine regression for ecological controls and observer genealogy."""
import json,pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
CORE=ROOT/'native'/'omega_core'
BASE=['--seed','13101','--initial','75','--capacity','145','--ticks','380','--sample','190','--visual','15','--rate','0','--world-scale','2','--metabolic-ecology','1','--ecology','1','--morphogenesis','1','--soft-body','1','--tissue-coupling','1','--regionality','1','--transport','1','--resource','.65','--mutation','.13']
def run(flags):
 a=[str(CORE),*BASE];
 if "resume" in flags:a.extend(["--ticks","700"])
 for key,v in flags.items():a.extend(['--'+key.replace('_','-'),str(v)])
 p=subprocess.run(a,input='',capture_output=True,text=True,timeout=25)
 if p.returncode:raise RuntimeError(p.stderr[-700:])
 return [json.loads(x) for x in p.stdout.splitlines() if x.startswith('{"type":"state"')][-1]
with tempfile.TemporaryDirectory() as d:
 ch=pathlib.Path(d)/'ref.bin';original=run({'checkpoint':ch,'checkpoint_every':380});assert ch.exists()
 base=run({'resume':ch})
 altered=run({'resume':ch,'experiment_resource':.16,'experiment_migration_cost':2,'experiment_isolation':.95})
 assert altered['resourceRate']==.16 and altered['blockedCrossings']>0
 assert altered['regionalMigrants']<base['regionalMigrants']
 assert altered['experimentalMigrationCost']==2
 assert base['blockedCrossings']==0
 assert isinstance(altered['lineageLinks'],list) and len(altered['lineageLinks'])==len(altered['individuals'])
 for obs,link in zip(altered['individuals'],altered['lineageLinks']):
  assert obs[0]==link[0] and obs[5]==link[3] and obs[6]==link[5]
  assert link[6]==sum(n[1]==2 for n in obs[11])
  assert link[7]==sum(n[1]==5 for n in obs[11])
  assert link[8]==sum(n[1]==7 for n in obs[11])
 assert altered==run({'resume':ch,'experiment_resource':.16,'experiment_migration_cost':2,'experiment_isolation':.95})
 print('PASS: same-source checkpoint replay; real corridor barriers, resource change, native genealogy links; deterministic branch')
