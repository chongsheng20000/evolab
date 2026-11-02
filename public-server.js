'use strict';
/** Public beta transport: isolated per-visitor native simulations, no account required.
 * Designed for a single persistent Docker web-service instance. Never expose the
 * original server.js, which contains shared/global simulation controls.
 */
const http=require('node:http');
const fs=require('node:fs');
const fsp=require('node:fs/promises');
const path=require('node:path');
const crypto=require('node:crypto');
const {createController}=require('./native/controller.js');
const ROOT=__dirname;
const DATA_ROOT=path.resolve(process.env.DATA_ROOT||path.join(ROOT,'data'));
const PORT=Number(process.env.PORT||10000);
const HOST=process.env.HOST||'0.0.0.0';
const MAX_ACTIVE=Math.max(1,Math.min(4,Number(process.env.MAX_ACTIVE||1)));
const MAX_SESSIONS=Math.max(MAX_ACTIVE,Math.min(60,Number(process.env.MAX_SESSIONS||8)));
const MAX_WALL_MS=Math.max(60,Math.min(1800,Number(process.env.MAX_WALL_SECONDS||480)))*1000;
const IDLE_TTL_MS=40*60*1000;
const COOKIE='evolab_session';
const contexts=new Map();
const contentTypes={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.png':'image/png','.json':'application/json; charset=utf-8','.webmanifest':'application/manifest+json'};
const staticMap=Object.freeze({
 '/native.html':'native.html','/native.js':'native.js','/native.css':'native.css',
 '/v98-ui.js':'v98-ui.js','/v98.css':'v98.css','/gpu-living.js':'gpu-living.js','/gpu-living.css':'gpu-living.css',
 '/living-viewport-v10.js':'living-viewport-v10.js','/living-viewport-v10.css':'living-viewport-v10.css',
 '/v103-network.js':'v103-network.js','/v103-network.css':'v103-network.css',
 '/v98-preview.json':'v98-preview.json','/native-public.js':'native-public.js',
 '/icon-192.png':'icon-192.png','/icon-512.png':'icon-512.png'
});
function respond(res,code,data){const body=JSON.stringify(data);res.writeHead(code,{'Content-Type':'application/json; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff','Content-Length':Buffer.byteLength(body)});res.end(body);}
function safeHeaders(res){res.setHeader('X-Content-Type-Options','nosniff');res.setHeader('X-Frame-Options','DENY');res.setHeader('Referrer-Policy','strict-origin-when-cross-origin');res.setHeader('Cross-Origin-Resource-Policy','same-origin');}
function activeCount(){let n=0;for(const c of contexts.values())if(['running','stopping'].includes(c.native.status().phase))n++;return n;}
function discard(session){if(!session)return;if(['running','stopping'].includes(session.native.status().phase))session.native.stop();contexts.delete(session.id);if(session.cleanupTimer)clearTimeout(session.cleanupTimer);fsp.rm(session.root,{recursive:true,force:true}).catch(()=>{});}
function resolveSession(req,res){const raw=(req.headers.cookie||'').split(';').map(x=>x.trim()).find(s=>s.startsWith(COOKIE+'='));const id=raw?raw.slice(COOKIE.length+1):'';
 if(/^[0-9a-f]{64}$/.test(id)&&contexts.has(id)){const found=contexts.get(id);found.touched=Date.now();return found;}
 if(contexts.size>=MAX_SESSIONS){const eviction=[...contexts.values()].filter(s=>!['running','stopping'].includes(s.native.status().phase)).sort((a,b)=>a.touched-b.touched)[0];if(eviction)discard(eviction);}
 if(contexts.size>=MAX_SESSIONS)return null;
 const fresh=crypto.randomBytes(32).toString('hex');const root=path.join(DATA_ROOT,'sessions',fresh);fs.mkdirSync(root,{recursive:true});
 const s={id:fresh,root,native:createController({dataRoot:root}),touched:Date.now(),startedAt:0,starts:[],cleanupTimer:null};contexts.set(fresh,s);
 const secure=process.env.NODE_ENV==='production'?'; Secure':'';
 res.setHeader('Set-Cookie',`${COOKIE}=${fresh}; Path=/; HttpOnly; SameSite=Lax; Max-Age=86400${secure}`);
 return s;
}
async function readInput(req,max=16384){const chunks=[];let size=0;for await(const c of req){size+=c.length;if(size>max)throw Object.assign(new Error('请求参数过大'),{httpCode:413});chunks.push(c);}let obj;try{obj=JSON.parse(Buffer.concat(chunks).toString('utf8'));}catch{throw Object.assign(new Error('无效 JSON'),{httpCode:400});}if(!obj||typeof obj!=='object'||Array.isArray(obj))throw Object.assign(new Error('参数必须是对象'),{httpCode:400});return obj;}
function validateOrigin(req){const origin=req.headers.origin;if(!origin)return false;try{const o=new URL(origin),host=req.headers.host;return o.host===host&&(o.protocol==='https:'||o.protocol==='http:');}catch{return false;}}
function acceptedOpts(input){if(input.organAblation!==undefined&&!['none','absorber','digester','root'].includes(input.organAblation))throw Object.assign(new Error('器官失能类型无效'),{httpCode:422});
 if(input.resume===true)throw Object.assign(new Error('公测暂不支持服务端继续已完成的实验；请下载检查点在本地恢复'),{httpCode:400});
 const max={initial:150,capacity:350,ticks:12000,worldScale:2,bodyCap:160,neuronCap:64,edgeCap:256,threads:2,visual:160,founderNodes:80,founderNeurons:48,rate:5000,checkpointEvery:12000};
 const min={initial:20,capacity:20,ticks:100,worldScale:1,bodyCap:40,neuronCap:32,edgeCap:128,threads:1,visual:0,founderNodes:0,founderNeurons:0,rate:0,checkpointEvery:100};
 const defaults={seed:crypto.randomInt(1,4294967295),initial:75,capacity:200,ticks:5000,worldScale:2,bodyCap:96,neuronCap:48,edgeCap:192,threads:1,visual:90,sample:100,rate:100,checkpointEvery:2000,founderNodes:0,founderNeurons:0};
 const o={...defaults,...input};
 for(const [name,upper] of Object.entries(max)){const v=o[name];if(typeof v!=='number'||!Number.isInteger(v)||v<min[name]||v>upper)throw Object.assign(new Error(`${name} 超出公网公测资源范围`),{httpCode:422});}
 if(o.initial>o.capacity)throw Object.assign(new Error('初始个体不能超过容量'),{httpCode:422});
 if(o.founderNodes>o.bodyCap||o.founderNeurons>o.neuronCap)throw Object.assign(new Error('初始结构超过容量'),{httpCode:422});
 if(typeof o.sample!=='number'||!Number.isInteger(o.sample)||o.sample<40||o.sample>2000)throw Object.assign(new Error('sample 应在 40—2000'),{httpCode:422});
 if(o.ticks/o.sample>300)throw Object.assign(new Error('公测最多保存 300 个采样，请提高采样间隔'),{httpCode:422});
 if(o.historyLedger===1&&o.ticks>6000)throw Object.assign(new Error('事件账本启用时最大 6000 tick'),{httpCode:422});
 if(typeof o.ablate==='string'&&o.ablate!=='none')throw Object.assign(new Error('公开测试暂不允许消融实验'),{httpCode:422});
 return o;
}
async function handler(req,res){safeHeaders(res);let pathname;try{pathname=new URL(req.url,'http://localhost').pathname;}catch{return respond(res,400,{error:'无效路径'});}
 if(req.method==='GET'&&pathname==='/healthz')return respond(res,200,{ok:true,version:'v10.8-functional-ecology',active:activeCount(),maxActive:MAX_ACTIVE});
 if(req.method==='GET'&&pathname==='/api/public/info')return respond(res,200,{version:'v10.8-functional-ecology',active:activeCount(),maxActive:MAX_ACTIVE,capacity:350,maxTicks:12000,sessionPersistence:'memory; download checkpoints before restart',note:'每位访客独立 C++ 模拟；免费云实例可能休眠'});
 if(req.method==='GET'&&pathname==='/'){res.writeHead(302,{Location:'/native.html'});return res.end();}
 if(req.method==='GET'&&staticMap[pathname]){
   if(pathname==='/native.html'&&!resolveSession(req,res))return respond(res,503,{error:'访客会话已达上限'});
   const filename=staticMap[pathname];let out=await fsp.readFile(path.join(ROOT,filename));
   if(filename==='native.html')out=Buffer.from(out.toString('utf8').replace('</body>','<script src="native-public.js"></script></body>'));
   res.writeHead(200,{'Content-Type':contentTypes[path.extname(filename)]||'application/octet-stream','Cache-Control':filename==='native.html'?'no-store':'public, max-age=300','Content-Length':out.byteLength});return res.end(out);
 }
 if(!pathname.startsWith('/api/native/'))return respond(res,404,{error:'Not found'});
 let s=resolveSession(req,res);if(!s)return respond(res,503,{error:'服务器已达到临时会话上限'});
 if(req.method==='GET'&&pathname==='/api/native/status')return respond(res,200,s.native.status());
 if(req.method==='GET'&&pathname==='/api/native/frame'){
  const tick=Number(new URL(req.url,'http://localhost').searchParams.get('tick'));const frame=s.native.frame(tick);return frame?respond(res,200,frame):respond(res,404,{error:'本会话没有对应历史快照'});
 }
 if(req.method==='GET'&&pathname==='/api/native/region'){
  const u=new URL(req.url,'http://localhost');const frame=s.native.region(Number(u.searchParams.get('x')),Number(u.searchParams.get('y')));return frame?respond(res,200,frame):respond(res,404,{error:'本会话没有对应区域'});
 }
 if(req.method==='GET'&&pathname.startsWith('/api/native/download/')){
  const key=pathname.split('/').at(-1);const file=s.native.filePath(key);if(!file)return respond(res,404,{error:'本会话尚无该文件'});
  const stat=await fsp.stat(file);if(stat.size>30*1024*1024)return respond(res,413,{error:'文件超过公网公测 30MB 下载限制，请降低采样规模'});
  res.writeHead(200,{'Content-Type':key==='checkpoint'?'application/octet-stream':key==='record'?'application/x-ndjson':'text/csv','Content-Disposition':`attachment; filename="EvoLab_${key}.${key==='checkpoint'?'bin':key==='record'?'jsonl':'csv'}"`,'Content-Length':stat.size,'Cache-Control':'no-store'});return fs.createReadStream(file).pipe(res);
 }
 if(req.method!=='POST')return respond(res,405,{error:'Method not allowed'});
 if(!validateOrigin(req))return respond(res,403,{error:'此操作必须从当前网站内发起（同源检查）'});
 if(!req.headers['content-type']?.startsWith('application/json')&&pathname!=='/api/native/stop')return respond(res,415,{error:'请使用 application/json'});
 const now=Date.now();s.lastPost??=0;if(pathname!=='/api/native/stop'&&now-s.lastPost<750)return respond(res,429,{error:'操作过于频繁，请稍后再试'});s.lastPost=now;
 if(pathname==='/api/native/start'){
  if(s.native.status().phase==='running'||s.native.status().phase==='stopping')return respond(res,409,{error:'当前世界正在演化，请先停止'});
  if(activeCount()>=MAX_ACTIVE)return respond(res,429,{error:'服务器计算席位已满，请稍后再试'});
  s.starts=s.starts.filter(t=>now-t<60*60*1000);if(s.starts.length>=8)return respond(res,429,{error:'此访客一小时最多启动八次模拟'});
  const cfg=acceptedOpts(await readInput(req));const out=s.native.start(cfg);s.startedAt=Date.now();s.starts.push(now);return respond(res,202,{...out,note:'此访客独立实验；会话和存档可能随服务重启清除'});
 }
 if(pathname==='/api/native/stop')return respond(res,200,s.native.stop());
 if(pathname==='/api/native/control'){
  const c=await readInput(req,2000);if(c.action==='rate'&&(!Number.isInteger(c.rate)||c.rate<0||c.rate>5000))return respond(res,422,{error:'公测速度上限为 5000 tick/s'});
  return respond(res,200,s.native.control(c.action,c.rate));
 }
 return respond(res,404,{error:'Not found'});
}
fs.mkdirSync(DATA_ROOT,{recursive:true});
const server=http.createServer((req,res)=>{handler(req,res).catch(e=>{const code=e.httpCode||(/参数|无效|超出/.test(e.message)?400:500);if(!res.headersSent)respond(res,code,{error:code===500?'服务器请求失败':e.message});else res.end();if(code===500)console.error('request failed',e.stack)});});
const timer=setInterval(()=>{const now=Date.now();for(const s of contexts.values()){
 if(['running','stopping'].includes(s.native.status().phase)&&now-s.startedAt>MAX_WALL_MS){s.native.stop();s.startedAt=now;}
 else if(now-s.touched>IDLE_TTL_MS&&s.native.status().phase!=='running'&&s.native.status().phase!=='stopping')discard(s);
}},5000);timer.unref();
server.listen(PORT,HOST,()=>console.log(`EvoLab Ω public beta listening ${HOST}:${PORT} (max active=${MAX_ACTIVE})`));
function shutdown(){clearInterval(timer);for(const s of contexts.values())s.native.stop();server.close();setTimeout(()=>process.exit(0),1500).unref();}
process.once('SIGTERM',shutdown);process.once('SIGINT',shutdown);
module.exports={server,acceptedOpts};
