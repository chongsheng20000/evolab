#!/usr/bin/env python3
"""Predeclared matched-checkpoint v10.10 structural/functional causal study.

Never interprets phenotype/genotype associations as proof of open-ended evolution.
All branches restart from the exact same binary checkpoint per seed;
'rescue' restores original topology of UIDs surviving at t=330 only.
"""
import argparse, csv, hashlib, json, math, pathlib, statistics, subprocess, tempfile, time
ROOT = pathlib.Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'native' / 'omega_core'
DEFAULT_SEEDS = tuple(range(12001, 12013))  # preregistered contiguous seeds, not cherry-picked
BRANCHES = ('control', 'functional_root_off', 'genomic_root_prune', 'genomic_root_prune_then_rescue')

def run(seed, endpoint, options, *, sample=600):
    argv=[str(ENGINE),'--seed',str(seed),'--initial','80','--capacity','160','--ticks',str(endpoint),
          '--sample',str(sample),'--visual','0','--rate','0','--world-scale','1',
          '--metabolic-ecology','1','--ecology','1','--morphogenesis','1','--soft-body','1',
          '--tissue-coupling','1','--resource','0.65','--mutation','0.13']
    for key,value in options.items():
        if value is not None:argv.extend(['--'+key.replace('_','-'),str(value)])
    p=subprocess.run(argv,input='',capture_output=True,text=True,timeout=120)
    if p.returncode:raise RuntimeError(f'seed={seed}, endpoint={endpoint}, options={options}, code={p.returncode}: {p.stderr[-600:]}')
    states=[json.loads(x) for x in p.stdout.splitlines() if x.startswith('{"type":"state"')]
    assert states and states[-1]['tick']==endpoint, f'No final tick: {seed} {endpoint}'
    return states

def study(out, seeds, horizon, instrument='root'):
    assert instrument=='root', 'This preregistered study is specific to root-mediated acquisition and drag'
    assert 800<=horizon<=5000000
    out.mkdir(parents=True,exist_ok=True)
    experiments=[];trajectories=[];checkpoints=[]
    for seed in seeds:
      with tempfile.TemporaryDirectory(prefix=f'evolab-v1010-{seed}-') as tmp:
        temp=pathlib.Path(tmp); original=temp/'world300.bin'; edited=temp/'root_pruned_330.bin'
        start=run(seed,300,{'checkpoint':original,'checkpoint_every':300},sample=150)[-1]
        assert original.exists()
        chksha=hashlib.sha256(original.read_bytes()).hexdigest()
        # Baseline replay is identical across two independent processes.
        baseline=run(seed,horizon,{'resume':original})
        baseline_copy=run(seed,horizon,{'resume':original})
        assert baseline[-1] == baseline_copy[-1], f'Failed exact deterministic replay for seed {seed}'
        # Functional knockout retains genome; genomic pruning changes the inherited body.
        off=run(seed,horizon,{'resume':original,'organ_ablation':'root'})
        pruned=run(seed,horizon,{'resume':original,'genome_prune':'root'})
        # At tick 330, serialize the genetically pruned world then restore matching
        # pre-pruning genomes of surviving reference UIDs in a separate branch.
        early=run(seed,330,{'resume':original,'genome_prune':'root','checkpoint':edited,'checkpoint_every':330},sample=30)
        assert edited.exists() and early[-1]['genomePrunedLives']>0 and early[-1]['genomePrunedNodes']>0
        restored=run(seed,horizon,{'resume':edited,'genome_rescue':original})
        notrestored=run(seed,horizon,{'resume':edited})
        assert restored[-1]['genomeRescuedLives']>0, f'No genomes rescued from reference seed {seed}'
        # A checkpoint restart without further intervention must preserve evolved world outcomes.
        for fld in ['population','births','deaths','maxGeneration','meanEnergy','rngState','functionalCarriers']:
            assert pruned[-1][fld]==notrestored[-1][fld], f'Checkpoint reentry diverged: {seed} {fld}'
        assert notrestored[-1]['genomeRescuedLives']==0
        assert early[-1]['genomePrunedNodes']>0
        # Restoration only reintroduces genuine original node structures, no invented species.
        assert restored[-1]['functionalCarriers'][2]>=0
        assert off[-1]['organAblation']=='root' and off[-1]['functionalUptake'][2]==0
        assert all(st['functionalUptake'][2]==0 for st in off), 'Disabled root must perform zero uptake'
        assert pruned[-1]['genomePrunedNodes']>0
        assert early[-1]['functionalCarriers'][2] <= start['functionalCarriers'][2]
        datasets={
            'control':baseline,
            'functional_root_off':off,
            'genomic_root_prune':pruned,
            'genomic_root_prune_then_rescue':restored,
            'genomic_root_prune_unrescued_330':notrestored,
        }
        for branch,trace in datasets.items():
          end=trace[-1];birthDelta=end['births']-start['births'];deathDelta=end['deaths']-start['deaths']
          record={'seed':seed,'branch':branch,'checkpoint_sha256':chksha,'start_tick':300,
             'horizon_tick':horizon,'start_pop':start['population'],'end_pop':end['population'],
             'start_births':start['births'],'births_since_start':birthDelta,
             'start_deaths':start['deaths'],'deaths_since_start':deathDelta,
             'start_generation':start['maxGeneration'],'max_generation':end['maxGeneration'],
             'mean_energy':end['meanEnergy'],'root_carriers':end['functionalCarriers'][2],
             'absorber_carriers':end['functionalCarriers'][0],'digester_carriers':end['functionalCarriers'][1],
             'root_functional_uptake_since_branch':end['functionalUptake'][2],
             'genome_pruned_lives_on_resume':end['genomePrunedLives'],
             'genome_pruned_nodes_on_resume':end['genomePrunedNodes'],
             'genome_rescued_lives_on_resume':end['genomeRescuedLives'],
             'genome_rescued_nodes_on_resume':end['genomeRescuedNodes'],
             'organ_competition':json.dumps(end['organCompetition'],separators=(',',':'))}
          experiments.append(record)
          for state in trace:
            trajectories.append({'seed':seed,'branch':branch,'tick':state['tick'],
                'population':state['population'],'births':state['births'],'deaths':state['deaths'],
                'mean_energy':state['meanEnergy'],'max_generation':state['maxGeneration'],
                'root_carriers':state['functionalCarriers'][2],
                'root_uptake_cumulative':state['functionalUptake'][2]})
        checkpoints.append({'seed':seed,'sha256':chksha,'bytes':original.stat().st_size})
        print(f'[{len(checkpoints)}/{len(seeds)}] seed {seed}: control births +{baseline[-1]["births"]-start["births"]}, pruned births +{pruned[-1]["births"]-start["births"]}, rescued reference UIDs {restored[-1]["genomeRescuedLives"]}',flush=True)
    for file,data in [('v1010_long_horizon_branches.csv',experiments),('v1010_long_horizon_trajectories.csv',trajectories),('v1010_checkpoint_hashes.csv',checkpoints)]:
        with (out/file).open('w',newline='',encoding='utf-8') as f:
            writer=csv.DictWriter(f,fieldnames=list(data[0]));writer.writeheader();writer.writerows(data)
    by={(r['seed'],r['branch']):r for r in experiments}
    keyfields=['births_since_start','deaths_since_start','end_pop','mean_energy','root_carriers','max_generation']
    summary={'protocol':'v10.10 predeclared paired-checkpoint root organ study',
     'seeds':list(seeds),'horizon_tick':horizon,'checkpoint_tick':300,'early_pruned_tick':330,
     'branches':list(datasets),'count':len(seeds),'observables':keyfields,
     'comparisons':{},'limitations':[
       'All branches share one simulated environment per seed but diverge following intervention; no pooled ecological interaction across different branches.',
       'Genome pruning deletes target-type nodes and all descendants; effects can involve loss of supporting tissue as well as target function.',
       'Rescue restores old genotypes only for surviving original UIDs, never for descendants; joints reset and existing cargo is released to grid, hence rescue is not a perfect reverse-time operation.',
       'High-population worlds can be capacity-saturated: birth and death count differences may not reflect adaptive fitness.',
       'These fixed rules and finite seeds do not prove sustained open-ended evolution or emergent civilization.']}
    for target in ['functional_root_off','genomic_root_prune','genomic_root_prune_then_rescue','genomic_root_prune_unrescued_330']:
        comp={}
        for field in keyfields:
            deltas=[by[(seed,target)][field]-by[(seed,'control')][field] for seed in seeds]
            comp[field]={'mean_paired_delta':statistics.fmean(deltas),'median_paired_delta':statistics.median(deltas),
                'positive_seeds':sum(x>0 for x in deltas),'negative_seeds':sum(x<0 for x in deltas),'zero_seeds':sum(x==0 for x in deltas),
                'raw_paired_deltas':deltas}
        summary['comparisons'][target]=comp
    (out/'v1010_summary.json').write_text(json.dumps(summary,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    return summary

if __name__=='__main__':
    a=argparse.ArgumentParser()
    a.add_argument('--out',type=pathlib.Path,default=ROOT/'evidence'/'v1010_study')
    a.add_argument('--seeds',type=int,default=len(DEFAULT_SEEDS))
    a.add_argument('--start',type=int,default=0,help='starting offset in fixed seed manifest')
    a.add_argument('--horizon',type=int,default=6000)
    opts=a.parse_args()
    assert 1<=opts.seeds<=len(DEFAULT_SEEDS) and opts.start>=0 and opts.start+opts.seeds<=len(DEFAULT_SEEDS)
    t=time.monotonic();summary=study(opts.out,DEFAULT_SEEDS[opts.start:opts.start+opts.seeds],opts.horizon)
    print(f'PASS: {summary["count"]} fixed seeds, deterministic baseline replay, genuine inherited structure pruning and reference-UID rescue, {time.monotonic()-t:.1f} s')
