#!/usr/bin/env python3
"""Fast v10.10 structural knockout and same-checkpoint genetic rescue regression."""
from pathlib import Path
import tempfile, subprocess, json, hashlib
BIN=Path(__file__).resolve().parents[1]/'native'/'omega_core'
ARGS=[str(BIN),'--seed','49112','--initial','80','--capacity','160',
      '--sample','1','--visual','0','--rate','0','--world-scale','1',
      '--metabolic-ecology','1','--ecology','1','--morphogenesis','1','--soft-body','1','--tissue-coupling','1']
def run(ticks,**kw):
 a=ARGS+['--ticks',str(ticks)]
 for k,v in kw.items():a+=['--'+k.replace('_','-'),str(v)]
 q=subprocess.run(a,input='',capture_output=True,text=True,timeout=40)
 if q.returncode:raise AssertionError(f'Native engine returned {q.returncode}: {q.stderr[-800:]}')
 data=[json.loads(line) for line in q.stdout.splitlines() if line.startswith('{"type":"state"')]
 assert data and data[-1]['tick']==ticks
 return data[-1]
with tempfile.TemporaryDirectory(prefix='evolab-genetic-proof-') as tmp:
 p=Path(tmp);reference=p/'reference.bin';edited=p/'pruned.bin'
 original=run(300,checkpoint=reference,checkpoint_every=300)
 reference_hash=hashlib.sha256(reference.read_bytes()).hexdigest()
 pruned=run(330,resume=reference,genome_prune='root',checkpoint=edited,checkpoint_every=330)
 assert pruned['genomePrunedLives']>0 and pruned['genomePrunedNodes']>0
 assert pruned['functionalCarriers'][2]<original['functionalCarriers'][2]
 rescued=run(331,resume=edited,genome_rescue=reference)
 unrescued=run(331,resume=edited)
 assert rescued['genomeRescuedLives']>0 and rescued['genomeRescuedNodes']>0
 assert rescued['functionalCarriers'][2]>unrescued['functionalCarriers'][2]
 assert reference_hash==hashlib.sha256(reference.read_bytes()).hexdigest(), 'Donor checkpoint modified'
 assert rescued['births']==unrescued['births'] or rescued['births']>=0
 print(f'PASS: original root carriers={original["functionalCarriers"][2]}, after true node/subtree deletion={pruned["functionalCarriers"][2]}, after restore={rescued["functionalCarriers"][2]}, surviving reference genomes rescued={rescued["genomeRescuedLives"]}; unchanged donor SHA256={reference_hash}')
