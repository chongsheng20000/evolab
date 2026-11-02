'use strict';
// Native single-user experiment driver. Loopback-only by default; do not expose public without auth.
const cp=require('node:child_process'),path=require('node:path'),fs=require('node:fs');
const bin=path.join(__dirname,process.platform==='win32'?'omega_core.exe':'omega_core');
function createController({dataRoot=path.join(__dirname,'..','experiments')}={}){
fs.mkdirSync(dataRoot,{recursive:true});
const numeric={seed:[1,4294967295],initial:[20,12000],capacity:[20,100000],ticks:[100,5000000],sample:[10,100000],visual:[0,200],mutation:[0,1],resourceRate:[0,3],season:[0,1],climate:[0,1],predation:[0,2],sharing:[0,2],cohesion:[0,2],specialization:[0,2],cooperation:[0,2],integration:[0,2],scavenging:[0,2],plasticity:[0,2],ecology:[0,2],development:[0,2],morphogenesis:[0,2],softBody:[0,2],collisionIndex:[0,1],allContacts:[0,1],bodyCap:[40,224],neuronCap:[32,80],edgeCap:[128,352],founderNeurons:[0,80],sparseBuckets:[0,2],graphPool:[0,1],contactCache:[0,1],historyLedger:[0,1],metabolicEcology:[0,2],founderNodes:[0,224],contactDedupe:[0,1],quietActivityLimit:[0,100000],quietFluxTolerance:[0,.05],regulatoryNetwork:[0,2],tissueCoupling:[0,2],grand:[0,2],habitats:[0,2],transport:[0,2],regionality:[0,2],worldScale:[1,3],scheduler:[0,1],compute:[0,1],lifeParallel:[0,1],threads:[1,12],checkpointEvery:[100,1000000],rate:[0,100000]};
const defaults={seed:261029,initial:350,capacity:1500,ticks:12000,sample:120,visual:100,mutation:.13,resourceRate:1,season:.25,climate:.5,predation:.5,sharing:.5,cohesion:.85,specialization:1,cooperation:.8,integration:0,scavenging:0,plasticity:0,ecology:0,development:0,morphogenesis:0,softBody:0,collisionIndex:0,allContacts:1,bodyCap:40,neuronCap:32,edgeCap:128,founderNeurons:0,sparseBuckets:0,graphPool:1,contactCache:1,historyLedger:1,metabolicEcology:1,founderNodes:0,contactDedupe:1,quietActivityLimit:12,quietFluxTolerance:0,regulatoryNetwork:1,tissueCoupling:0,grand:0,habitats:0,transport:0,regionality:0,worldScale:1,scheduler:1,compute:1,lifeParallel:1,threads:1,checkpointEvery:2000,rate:0};
const cliName={resourceRate:'resource',checkpointEvery:'checkpoint-every',worldScale:'world-scale',lifeParallel:'life-parallel',softBody:'soft-body',tissueCoupling:'tissue-coupling',collisionIndex:'collision-index',allContacts:'all-contacts',regulatoryNetwork:'regulatory-network',bodyCap:'body-cap',neuronCap:'neuron-cap',edgeCap:'edge-cap',founderNeurons:'founder-neurons',sparseBuckets:'sparse-buckets',graphPool:'graph-pool',contactCache:'contact-cache',historyLedger:'history-ledger',metabolicEcology:'metabolic-ecology',founderNodes:'founder-nodes',contactDedupe:'contact-dedupe',quietActivityLimit:'quiet-activity-limit',quietFluxTolerance:'quiet-flux-tolerance'};
const state={phase:'idle',engine:'native-cpp17-genomic-causality-v10.10',started:null,ended:null,last:null,hello:null,samples:[],error:null,exitCode:null,pid:null,checkpointAvailable:false,config:null,rate:0,paused:false,frames:[],progress:0,actualTicksPerSecond:0,lastCheckpointTick:null};
let child=null,linebuf='',savedFiles={};
function status(){return {...state,samples:state.samples.slice(-250),frames:state.frames.map(f=>({tick:f.tick,population:f.population,maxGeneration:f.maxGeneration}))};}
function frame(tick){if(!Number.isSafeInteger(tick)||tick<0)return null;return state.frames.find(f=>f.tick===tick)||null;}
// On-demand observation ONLY: no region ever skips native physics or genetics.
function region(x,y){
 const f=state.last;if(!f)return null;
 const n=Number(f.worldScale||1);if(!Number.isInteger(x)||!Number.isInteger(y)||x<0||y<0||x>=n||y>=n)return null;
 const z=y*n+x, mapWidth=16*n, grids={};
 for(const key of ['resourceGrid','detritusGrid','soilGrid','densityGrid','ecotypeGrid','habitatGrid']){
   if(!Array.isArray(f[key])||f[key].length!==mapWidth*10*n)continue;
   const subset=[];for(let row=0;row<10;row++)for(let col=0;col<16;col++)subset.push(f[key][(y*10+row)*mapWidth+x*16+col]);
   grids[key]=subset;
 }
 const sampled=(f.individuals||[]).filter(a=>Math.min(n-1,Math.floor(a[1]/1300))===x&&Math.min(n-1,Math.floor(a[2]/850))===y);
 const ids=new Set(sampled.map(a=>a[0]));
 return {tick:f.tick,worldScale:n,region:[x,y],profile:f.regionProfiles?.[z]||null,summary:f.regions?.[z]||null,individuals:sampled,visibleBonds:(f.bonds||[]).filter(b=>ids.has(b[0])&&ids.has(b[1])),...grids,observationOnly:true};
}

function parse(data){linebuf+=data.toString('utf8');let e;while((e=linebuf.indexOf('\n'))>=0){let line=linebuf.slice(0,e);linebuf=linebuf.slice(e+1);if(!line||line.length>1500000)continue;try{let msg=JSON.parse(line);if(msg.type==='hello')state.hello=msg;else if(msg.type==='checkpoint'){state.checkpointAvailable=true;state.lastCheckpointTick=msg.tick;}else if(msg.type==='state'){state.last=msg;
 const {individuals,interactions,relationships,bonds,resourceGrid,detritusGrid,soilGrid,...summary}=msg;
 state.samples.push(summary);if(state.samples.length>700)state.samples.shift();
 if(individuals?.length&&(!state.frames.length||state.frames[state.frames.length-1].tick!==msg.tick)){
   state.frames.push(msg);if(state.frames.length>90)state.frames.shift();
 }
 state.progress=state.config?msg.tick/state.config.ticks:0;
 const elapsed=Math.max(.05,(Date.now()-state.started)/1000);
 state.actualTicksPerSecond=Number((msg.tick-(state.startTick||0))/elapsed).toFixed(1)*1;
}else if(msg.type==='complete'){state.phase='completed';state.ended=Date.now();state.checkpointAvailable=!!savedFiles.checkpoint&&fs.existsSync(savedFiles.checkpoint);}}catch{}}if(linebuf.length>4000000)linebuf='';}
function start(input={}){
 if(child)throw Error('上一项原生计算尚未退出');
 if(!fs.existsSync(bin))throw Error('原生二进制未编译：执行 bash native/build.sh');
 const resume=input.resume===true;let opts={};
 for(let [key,[min,max]] of Object.entries(numeric)){
   let v=input[key]??(resume&&state.config?state.config[key]:defaults[key]);
   if(typeof v!=='number'||!Number.isFinite(v)||v<min||v>max||(!['mutation','resourceRate','season','climate','predation','sharing','cohesion','specialization','cooperation','integration','scavenging','plasticity','ecology','development','morphogenesis','softBody','tissueCoupling','regulatoryNetwork','metabolicEcology','grand','habitats','transport','regionality','quietFluxTolerance','metabolicEcology'].includes(key)&&!Number.isInteger(v)))throw Error(key+' 参数无效');opts[key]=v;
 }
 if(opts.founderNeurons>opts.neuronCap)throw Error('初始神经压力测试不得超过神经容量');
 if(opts.founderNodes>opts.bodyCap)throw Error('初始身体节点数不得超过遗传上限');
 if(opts.initial>opts.capacity)throw Error('初始数量不能超过容量');
 let organAblation=input.organAblation??(resume&&state.config?state.config.organAblation:'none');
 if(!['none','absorber','digester','root'].includes(organAblation))throw Error('不支持的器官失能类型');
 let ablate=input.ablate??(resume&&state.config?state.config.ablate:'none');
 if(!['none','motor','signal','climate','interaction','organization','specialization','cooperation','integration'].includes(ablate))throw Error('消融类型不支持');
 let neutral=input.neutral??(resume&&state.config?state.config.neutral:false);
 if(typeof neutral!=='boolean')throw Error('neutral 必须为布尔值');
 if(resume){
  if(!savedFiles.checkpoint||!fs.existsSync(savedFiles.checkpoint)||!state.config)throw Error('暂无可续算检查点');
  for(const k of ['seed','initial','capacity','mutation','resourceRate','season','climate','predation','sharing','cohesion','specialization','cooperation','integration','scavenging','plasticity','ecology','development','morphogenesis','softBody','tissueCoupling','regulatoryNetwork','metabolicEcology','grand','habitats','transport','regionality','tissueCoupling','worldScale','scheduler','bodyCap','neuronCap','edgeCap','founderNeurons','sparseBuckets','graphPool','contactCache','historyLedger','founderNodes','contactDedupe','quietActivityLimit','quietFluxTolerance'])if(opts[k]!==state.config[k])throw Error('续算时不能修改世界或遗传参数；请新建实验');
  if(ablate!==state.config.ablate||organAblation!==state.config.organAblation||neutral!==state.config.neutral)throw Error('续算时消融条件必须保持一致');
  if(opts.ticks<=Number(state.last?.tick??0))throw Error('续算终点必须晚于现有 tick');
 }else{
  const dir=path.join(dataRoot,'native-'+new Date().toISOString().replace(/[^0-9T]/g,'').slice(0,15)+'-'+process.pid+'-'+Math.floor(Math.random()*100000));
  fs.mkdirSync(dir,{recursive:true});savedFiles={csv:path.join(dir,'metrics.csv'),lineage:path.join(dir,'lineage.csv'),record:path.join(dir,'states.jsonl'),organization:path.join(dir,'organization_lineage.csv'),fitness:path.join(dir,'organization_fitness.csv'),regionalLedger:path.join(dir,'regional_ledger.csv'),eventLedger:path.join(dir,'interaction_ledger.csv'),checkpoint:path.join(dir,'checkpoint.bin')};
 }
 Object.assign(state,{phase:'running',started:Date.now(),ended:null,last:resume?state.last:null,hello:null,samples:resume?state.samples:[],frames:resume?state.frames:[],rate:opts.rate,paused:false,progress:0,startTick:resume?(state.last?.tick||0):0,actualTicksPerSecond:0,lastCheckpointTick:resume?state.lastCheckpointTick:null,error:null,exitCode:null,pid:null,checkpointAvailable:resume,config:{...opts,ablate,organAblation,neutral}});
 linebuf='';let args=[];for(let [k,v] of Object.entries(opts))args.push('--'+(cliName[k]||k),String(v));
 if(opts.historyLedger===1)args.push('--regional-ledger',savedFiles.regionalLedger,'--event-ledger',savedFiles.eventLedger);
 args.push('--organ-ablation',organAblation,'--ablate',ablate,'--neutral',neutral?'1':'0','--csv',savedFiles.csv,'--lineage',savedFiles.lineage,'--organization-log',savedFiles.organization,'--organization-fitness-log',savedFiles.fitness,'--record',savedFiles.record,'--checkpoint',savedFiles.checkpoint);
 if(resume)args.push('--resume',savedFiles.checkpoint);
 child=cp.spawn(bin,args,{cwd:__dirname,stdio:['pipe','pipe','pipe'],windowsHide:true});state.pid=child.pid;
 let err='';child.stdout.on('data',parse);child.stderr.on('data',b=>{err+=(b.toString('utf8')).slice(0,8000);});
 child.on('error',e=>{state.phase='error';state.error=e.message;state.ended=Date.now();child=null;});
 child.on('exit',(code)=>{state.exitCode=code;state.ended=Date.now();if(state.phase==='running'||state.phase==='stopping')state.phase=code===0?'completed':'error';if(state.phase==='error')state.error=err||('exit '+code);state.checkpointAvailable=!!savedFiles.checkpoint&&fs.existsSync(savedFiles.checkpoint);child=null;});
 return {accepted:true,phase:'running',pid:state.pid,resume,rate:opts.rate};
}
function filePath(kind){if(!['csv','lineage','organization','fitness','record','checkpoint','regionalLedger','eventLedger'].includes(kind))return null;const v=savedFiles[kind];return v&&fs.existsSync(v)?v:null;}
function stop(){if(child&&state.phase==='running'){state.phase='stopping';child.kill('SIGTERM');return {stopping:true,checkpoint:'will-save-on-shutdown'};}return {stopping:false};}
function control(action,rate){
 if(!child||!child.stdin.writable||state.phase!=='running')throw Error('当前没有可控制的原生实验');
 if(action==='rate'){
   if(!Number.isInteger(rate)||rate<0||rate>100000)throw Error('执行速度超出允许范围');
   child.stdin.write(`RATE ${rate}\n`);state.rate=rate;state.config.rate=rate;
 }else if(action==='pause') {child.stdin.write('PAUSE\n');state.paused=true;}
 else if(action==='resume') {child.stdin.write('RESUME\n');state.paused=false;}
 else if(action==='step') {child.stdin.write('STEP\n');state.paused=true;}
 else if(action==='checkpoint')child.stdin.write('CHECKPOINT\n');
 else throw Error('不支持的控制命令');
 return {accepted:true,action,rate:state.rate,paused:state.paused};
}
return {start,stop,status,frame,region,filePath,control};
}
module.exports={createController};
