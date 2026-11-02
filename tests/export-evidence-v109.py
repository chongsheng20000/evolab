#!/usr/bin/env python3
"""Export real matched checkpoint experiment outcomes to CSV."""
import csv, hashlib, importlib.util, pathlib, tempfile
root=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('v109test',str(root/'tests'/'test-competition-v109.py'))
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
with tempfile.TemporaryDirectory() as folder:
 checkpoint=pathlib.Path(folder)/'starting_world.bin'
 mod.run(seed=49391,ticks=300,checkpoint=checkpoint)
 digest=hashlib.sha256(checkpoint.read_bytes()).hexdigest()
 data=[]
 for treatment in ['none','absorber','digester','root']:
  state=mod.run(seed=49391,ticks=750,resume=checkpoint,ablate=treatment)[-1]
  data.append({'seed':49391,'starting_checkpoint_sha256':digest,'checkpoint_tick':300,'final_tick':state['tick'],
   'intervention':treatment,'alive':state['population'],'births_cumulative':state['births'],
   'deaths_cumulative':state['deaths'],'max_generation':state['maxGeneration'],
   'food_flux_tick':state['functionalUptake'][0], 'detritus_flux_tick':state['functionalUptake'][1],
   'root_flux_tick':state['functionalUptake'][2], 'root_velocity_loss_tick':state['anchorVelocityLoss'],
   'ledger_start_tick':state['organLedgerSinceTick'],
   'absorber_carrier_parent_births':state['organCompetition'][0][3],
   'absorber_carrier_deaths':state['organCompetition'][0][5],
   'absorber_carrier_exposure':state['organCompetition'][0][1],
   'digester_carrier_parent_births':state['organCompetition'][1][3],
   'digester_carrier_deaths':state['organCompetition'][1][5],
   'digester_carrier_exposure':state['organCompetition'][1][1],
   'root_carrier_parent_births':state['organCompetition'][2][3],
   'root_carrier_deaths':state['organCompetition'][2][5],
   'root_carrier_exposure':state['organCompetition'][2][1]})
 out=root/'evidence'/'v109_checkpoint_ablation.csv';out.parent.mkdir(exist_ok=True)
 with out.open('w',newline='') as f:
  writer=csv.DictWriter(f,fieldnames=list(data[0]));writer.writeheader();writer.writerows(data)
 print('wrote',out,'4 branches, same checkpoint hash',digest[:18])
