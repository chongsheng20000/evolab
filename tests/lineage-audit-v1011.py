#!/usr/bin/env python3
"""Real hereditary organ continuity, based on native birth ledger; not fitness proof."""
import argparse,csv,json,pathlib,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
CORE=ROOT/'native/omega_core'
BASE=['--initial','75','--capacity','145','--ticks','1600','--sample','400','--visual','0','--rate','0','--world-scale','2','--metabolic-ecology','1','--ecology','1','--morphogenesis','1','--soft-body','1','--tissue-coupling','1','--regionality','1','--transport','1','--resource','.65','--mutation','.13']
def main():
 p=argparse.ArgumentParser();p.add_argument('--out',type=pathlib.Path,default=ROOT/'evidence/v1011_multienv/lineages');p.add_argument('--start',type=int,default=13101);p.add_argument('--seeds',type=int,default=6);args=p.parse_args();args.out.mkdir(parents=True,exist_ok=True)
 audit=[]
 for seed in range(args.start,args.start+args.seeds):
  file=args.out/f'seed{seed}.csv'
  if not file.exists():
   result=subprocess.run([str(CORE),'--seed',str(seed),*BASE,'--lineage',str(file)],input='',capture_output=True,text=True,timeout=60)
   if result.returncode:raise RuntimeError(result.stderr[-500:])
  with file.open(newline='',encoding='utf8') as f:rows=list(csv.DictReader(f))
  known={};total=0;parent_root=0;retained=0;no_parent_root=0;new_root=0;root_grandparent=0;three_level=0;structure_changes=0;missing=0
  for row in rows:
   uid=int(row['individual_id']);parent=int(row['parent_id']);root=int(row['root_nodes']);
   if parent:
    total+=1
    if parent not in known:missing+=1
    else:
     a=known[parent]
     if int(row['generation'])!=int(a['generation'])+1:raise AssertionError(f'lineage generation mismatch: {uid}')
     structure_changes+=row['structure_hash']!=a['structure_hash']
     if int(a['root_nodes'])>0:
      parent_root+=1;retained+=root>0
     else:
      no_parent_root+=1;new_root+=root>0
     ancestor=known.get(int(a['parent_id'])) if a['parent_id'] else None
     if ancestor is not None and int(ancestor['root_nodes'])>0 and int(a['root_nodes'])>0:
      root_grandparent+=1;three_level+=root>0
   known[uid]=row
  if missing:raise AssertionError(f'missing parent records {seed}: {missing}')
  audit.append({'seed':seed,'birth_rows':total,'parent_root_transmissions':parent_root,
    'offspring_retaining_root':retained,'root_retention_rate':(retained/parent_root if parent_root else None),
    'root_absent_parent_transmissions':no_parent_root,'root_new_appearance':new_root,
    'root_new_appearance_rate':(new_root/no_parent_root if no_parent_root else None),
    'tracked_root_three_generation_paths':root_grandparent,
    'root_retained_at_third_generation':three_level,
    'offspring_different_structure_hash':structure_changes})
  print(seed,'births',total,'root inherited',retained,'/',parent_root,'novel root',new_root,'/',no_parent_root,flush=True)
 with (args.out/'lineage_audit.csv').open('w',newline='',encoding='utf8') as f:
  writer=csv.DictWriter(f,list(audit[0]));writer.writeheader();writer.writerows(audit)
 (args.out/'lineage_audit.json').write_text(json.dumps({'method':'Native lineage ledger and exact parent UID joins, founders included','seeds':[x['seed'] for x in audit],'results':audit,'limitations':['Records genetic birth presence, not subsequent survival or lifetime fitness.','Parents are all present in the current complete ledger; the browser projection is sampled and incomplete.','Root structure gain may arise by mutation and is not automatically an advantageous innovation.']},indent=2,ensure_ascii=False)+'\n',encoding='utf8')
if __name__=='__main__':main()
