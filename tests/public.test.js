'use strict';
const test=require('node:test');const assert=require('node:assert/strict');const fs=require('node:fs');const path=require('node:path');const os=require('node:os');const cp=require('node:child_process');
const root=path.resolve(__dirname,'..');let server=null,port=0,tmp='';
async function get(url,session='',method='GET',body=null,origin=true){const headers={};if(session)headers.cookie=session;if(origin&&method==='POST')headers.Origin=`http://127.0.0.1:${port}`;if(body){headers['Content-Type']='application/json';}const r=await fetch(`http://127.0.0.1:${port}${url}`,{method,headers,body:body?JSON.stringify(body):undefined});return {code:r.status,body:r.headers.get('content-type')?.startsWith('application/json')?await r.json():await r.text(),cookie:r.headers.get('set-cookie')||''};}
async function setup(){tmp=fs.mkdtempSync(path.join(os.tmpdir(),'evolab-public-test-'));port=18900+Math.floor(Math.random()*1000);server=cp.spawn(process.execPath,['public-server.js'],{cwd:root,env:{...process.env,PORT:String(port),MAX_ACTIVE:'1',NODE_ENV:'test',DATA_ROOT:tmp},stdio:'ignore'});for(let i=0;i<100;i++){try{const r=await get('/healthz');if(r.code===200)return;}catch{}await new Promise(r=>setTimeout(r,50));}throw Error('public server did not start');}
test('isolated public beta API and quotas',async()=>{
 await setup();try{
  let a=await get('/native.html');let b=await get('/native.html');const c1=a.cookie.split(';')[0],c2=b.cookie.split(';')[0];assert.ok(c1.startsWith('evolab_session='));assert.ok(c2.startsWith('evolab_session='));assert.notEqual(c1,c2);
  assert.equal((await get('/api/native/status',c1)).body.phase,'idle');assert.equal((await get('/api/native/status',c2)).body.phase,'idle');
  assert.equal((await get('/api/native/status',c1,'GET')).code,200);
  assert.equal((await get('/api/native/start',c1,'POST',{seed:301,initial:70,capacity:150,ticks:6000,worldScale:2,rate:1,sample:100,visual:60})).code,202);
  assert.equal((await get('/api/native/start',c2,'POST',{seed:302,initial:70,capacity:150,ticks:6000,worldScale:2,rate:1,sample:100,visual:60})).code,429,'global CPU slot quota');
  assert.equal((await get('/api/native/status',c2)).body.phase,'idle','separate status must be idle');
  assert.equal((await get('/api/native/start',c2,'POST',{seed:302,capacity:9999,initial:700,ticks:6000})).code,429,'slot rejected first');
  assert.equal((await get('/api/native/control',c1,'POST',{action:'pause'},false)).code,403,'no cross-origin mutations');
  let s;for(let i=0;i<80;i++){s=(await get('/api/native/status',c1)).body;if(s.last?.tick>0)break;await new Promise(r=>setTimeout(r,60));}
  assert.ok(s.last?.tick>0,'actual C++ native world running');assert.equal(s.last.worldScale,2);assert.ok(s.last.individuals.length>0);
  assert.equal((await get('/api/native/status',c2)).body.last,null);
  assert.equal((await get('/api/native/frame?tick='+s.last.tick,c2)).code,404,'no cross-session frame');
  assert.equal((await get('/api/native/region?x=0&y=0',c1)).code,200);
  assert.equal((await get('/api/native/region?x=0&y=0',c2)).code,404);
  const stopped=await get('/api/native/stop',c1,'POST');assert.equal(stopped.code,200);
  let phase;for(let i=0;i<80;i++){phase=(await get('/api/native/status',c1)).body.phase;if(!['running','stopping'].includes(phase))break;await new Promise(r=>setTimeout(r,80));}
  assert.equal(['running','stopping'].includes(phase),false,'stop releases slot');
  await new Promise(r=>setTimeout(r,800));
  assert.equal((await get('/api/native/start',c2,'POST',{seed:302,capacity:9999,initial:70,ticks:5000})).code,422);
  await new Promise(r=>setTimeout(r,850));
  assert.equal((await get('/api/native/start',c2,'POST',{seed:302,initial:70,capacity:140,ticks:100,sample:50,rate:500,worldScale:1})).code,202);
  let ss;for(let i=0;i<100;i++){ss=(await get('/api/native/status',c2)).body;if(['completed','error'].includes(ss.phase))break;await new Promise(r=>setTimeout(r,50));}
  assert.equal(ss.phase,'completed');assert.ok(ss.last.tick>=100);
  assert.equal((await get('/api/native/status',c1)).body.config.seed,301,'session config remains private');
  // v10.9: public organ interventions reach native C++ without bypassing quotas.
  await new Promise(r=>setTimeout(r,850));
  assert.equal((await get('/api/native/start',c2,'POST',{seed:305,initial:70,capacity:140,ticks:100,sample:50,rate:500,worldScale:1,organAblation:'root',metabolicEcology:1,ecology:1})).code,202);
  let r109;for(let i=0;i<100;i++){r109=(await get('/api/native/status',c2)).body;if(['completed','error'].includes(r109.phase))break;await new Promise(r=>setTimeout(r,50));}
  assert.equal(r109.phase,'completed');
  assert.equal(r109.last.organAblation,'root');
  assert.equal(r109.last.functionalUptake[2],0);
  assert.equal(r109.last.anchorVelocityLoss,0);
  assert.equal(r109.last.organCompetition.length,3);
  assert.equal((await get('/api/native/start',c1,'POST',{seed:301,initial:70,capacity:140,ticks:100,sample:50,organAblation:'fabricated'})).code,422);
  console.log('PUBLIC BETA TEST PASS: isolated native worlds, quota, same-origin, per-session frames, real C++');
 }finally{server?.kill('SIGTERM');await new Promise(r=>setTimeout(r,300));fs.rmSync(tmp,{recursive:true,force:true});}
});
