'use strict';
/* EvoLab Ω v9.9 — view-only interface composition.
   Every biological measurement comes from the C++ sampled state. Rendering has
   no write path to genomes, simulation tick, ecological resources or lineage. */
(() => {
const stage=document.querySelector('.stage'), stageBar=stage?.querySelector('.bar'), oldWorld=stage?.querySelector('.worldStage'), footer=stage?.querySelector('.stagefoot'),bar=document.querySelector('.chronobar'),layout=document.querySelector('.layout');
if(!stage||!oldWorld||!layout)return;
const make=(tag,cl,html)=>{const a=document.createElement(tag);if(cl)a.className=cl;if(html!==undefined)a.innerHTML=html;return a;};
const go=(selector)=>document.querySelector(selector);
const top=make('div','v98-top');
const left=make('aside','v98-sidebar v98-left',`<div class="v98-card-header">◉ 世界总览 <small>WORLD OVERVIEW</small></div><canvas id="v98MiniMap" width="410" height="220" aria-label="基于采样状态的区域世界微缩图"></canvas><div class="v98-map-caption"><span id="v98MapPopulation">等待生命数据</span><span id="v98MapScale">— 区域</span></div><div class="v98-card-header v98-split-head">≋ 区域活动 <small>REGION ACTIVITY</small></div><div id="v98RegionList" class="v98-region-list"><div class="v98-muted">启动世界后显示分区</div></div><div class="v98-card-header v98-split-head">↭ 生态关系 <small>ECOLOGICAL RELATIONS</small></div><div class="v98-overlay-lines"><span><i style="--c:#87eddf"></i>资源转移</span><span><i style="--c:#e6b970"></i>掠夺与能量交换</span><span><i style="--c:#c3b3f3"></i>亲缘与谱系</span><span><i style="--c:#f5d88b"></i>持续组织连接</span></div><p class="v98-muted v98-context">箭头绑定当前采样 ID；虚线表示历史事件。全图使用原生资源与种群数据。</p>`);
const middle=make('section','v98-world-column');
const right=make('aside','v98-sidebar v98-right',`<div class="v98-card-header">✧ 个体解析 <small>ORGANISM ANATOMY</small><button type="button" id="v98Expand" title="打开高分辨率结构观察器">⛶</button></div><div class="v98-mode-tabs" role="group" aria-label="结构观察模式"><button data-anatomy="body" class="is-active">结构总览</button><button data-anatomy="drive">控制通道</button><button data-anatomy="function">功能构成</button></div><canvas id="v98Anatomy" width="780" height="470" aria-label="根据实际身体结构绘制的高质感解剖视图"></canvas><p id="v98AnatomyCaption" class="v98-caption">选择生命，查看实际身体发育拓扑。</p><div class="v98-sample-metrics"><div>身体节点 <strong id="v98Parts">—</strong></div><div>表达节点 <strong id="v98Expressed">—</strong></div><div>神经元 <strong id="v98Neurons">—</strong></div><div>反馈连接 <strong id="v98Feedback">—</strong></div><div>遗传世代 <strong id="v98Gene">—</strong></div><div>内部能量 <strong id="v98Energy">—</strong></div></div><button type="button" id="v98OpenStructure" class="v98-primary-soft">⤢ 进入全屏结构观察器</button><div class="v98-card-header v98-split-head">◈ 个体关系 <small>LIVE LINKS</small></div><div id="v98LiveRelations" class="v98-small-list">选择个体后显示实际连接及最近交互</div><p class="v98-muted v98-context">材质与发光仅属可视化；原始模拟只有节点、部件类型、驱动参数和能量等数据。不会虚构内部器官。</p>`);
top.append(left,middle,right);stageBar?.after(top);middle.append(oldWorld);middle.append(footer);
// Relocate the existing functional run controls without duplicating event targets.
if(bar){stage.append(bar);bar.classList.add('v98-time-strip');}
const runtimeStrip=make('div','v98-runtime-strip',`<span><b>▤ 精确生命流水线</b><small id="v98ParallelStatus">等待内核数据</small></span><span><b>▧ 连续区域调度</b><small id="v98SchedulerStatus">等待世界启动</small></span><span class="v98-honesty">高维／宇宙与近似休眠尚未实现；当前仅为二维精确演化。</span>`);stage.append(runtimeStrip);
stage.classList.add('v98-stage');
const settings=make('button','v98-toolbar-btn', '⚙ 世界参数');settings.id='v98Settings';settings.type='button';
const info=make('button','v98-toolbar-btn','ℹ 模拟说明');info.type='button';info.id='v98Info';
const header=document.querySelector('header');header?.append(settings,info);
const stageControls=stage.querySelector('.fieldcontrols');
const previewBtn=make('button','v98-toolbar-btn','◉ 历史样本');previewBtn.type='button';previewBtn.id='v98Preview';stageControls?.prepend(previewBtn);
const magnify=make('button','v98-toolbar-btn v98-magnify-trigger','⌕ 生命剖析');magnify.type='button';magnify.id='v98AnatomyOpenHeader';stageControls?.prepend(magnify);
const btnOut=make('button','v98-icon','−'),btnIn=make('button','v98-icon','+');btnOut.id='v98ZoomOut';btnIn.id='v98ZoomIn';btnOut.setAttribute('aria-label','缩小世界');btnIn.setAttribute('aria-label','放大世界');stageControls?.prepend(btnOut,btnIn);
const viewScale=go('#viewScale');if(viewScale){for(let [value,label] of [[16,'显微近景 ×16'],[32,'显微近景 ×32'],[64,'显微近景 ×64'],[128,'结构追踪 ×128']]){let option=document.createElement('option');option.value=value;option.textContent=label;viewScale.append(option);}}
const label=stage.querySelector('.bar>span');if(label){label.innerHTML='<i class="live"></i> LIVING ECOSYSTEM <small class="v98-data-honest"> / REAL C++ STATE · VIEW ONLY</small>';}
const previewBadge=make('div','v98-preview-badge','已计算历史快照 · 非实时 · 点击开始演化可启动新世界');previewBadge.hidden=true;stage.querySelector('.worldStage')?.append(previewBadge);
const legend=make('div','v98-view-legend','<span class="v98-dot"></span> 生物与环境来自实际计算 <span class="v98-divider">│</span> 高质感材质为视图重建');stage.querySelector('.worldStage')?.append(legend);
const originalHeading=document.querySelector('.heading');
settings.onclick=()=>{layout.classList.toggle('v98-settings-visible');settings.classList.toggle('is-active');};
info.onclick=()=>{if(originalHeading)originalHeading.classList.toggle('v98-heading-open');};
const cover=make('div','v98-drawer-cover');layout.append(cover);cover.onclick=()=>layout.classList.remove('v98-settings-visible');
const close=make('button','v98-close-panel','×');close.type='button';close.title='关闭参数';close.onclick=()=>layout.classList.remove('v98-settings-visible');layout.querySelector('.panel')?.prepend(close);
// Observation-only full-screen detail; no invented neuron adjacency or organ volumes.
const dialog=make('dialog','v98-dialog',`<div class="v98-dialog-top"><div><small>REAL BODY GRAPH / HIGH-RESOLUTION VECTOR RENDERING</small><h2>生命内部结构观察器 <span id="v98DialogUid"></span></h2></div><button type="button" id="v98CloseDialog" aria-label="关闭观察器">×</button></div><div class="v98-detail-toolbar"><label>放大倍率 <input type="range" min="1" max="128" value="1" id="v98DetailZoom"/></label><strong id="v98DetailZoomText">×1 · 拓扑适配</strong><button id="v98ResetDetail" type="button">视图复位</button><button id="v98DetailBody" type="button">身体结构</button><button id="v98DetailDrive" type="button">神经驱动</button><button id="v98DetailFunction" type="button">功能类型</button></div><canvas id="v98AnatomyHuge" width="1400" height="820" aria-label="实际个体身体发育结构高清近景"></canvas><div class="v98-dialog-bottom"><span id="v98AnatomyNote">选择生命后开始观察</span><span>可放大真实节点与连接。无体积数据的部位不会被当作已模拟器官。</span></div>`);
document.body.append(dialog);
const closeDialog=()=>dialog.close();go('#v98CloseDialog').onclick=closeDialog;
const openDialog=()=>{if(!latest?.individuals?.length){go('#status').textContent='请先开始模拟并选择生命';return;}if(focusId==null)selectLargest();try{dialog.showModal();}catch{dialog.setAttribute('open','');}drawAnatomy();};
for(const id of ['v98Expand','v98OpenStructure','v98AnatomyOpenHeader'])go('#'+id).onclick=openDialog;
go('#v98DetailZoom').oninput=e=>{detailZoom=Number(e.target.value);go('#v98DetailZoomText').textContent='×'+detailZoom+' · 数字放大';drawAnatomy();};
go('#v98ResetDetail').onclick=()=>{detailZoom=1;detailPan={x:0,y:0};go('#v98DetailZoom').value='1';go('#v98DetailZoomText').textContent='×1 · 拓扑适配';drawAnatomy();};
const modes=['body','drive','function'];let anatomyMode='body',detailZoom=1,detailPan={x:0,y:0};
function setMode(mode){anatomyMode=mode;document.querySelectorAll('[data-anatomy]').forEach(b=>b.classList.toggle('is-active',b.dataset.anatomy===mode));drawAnatomy();}
document.querySelectorAll('[data-anatomy]').forEach(b=>b.onclick=()=>setMode(b.dataset.anatomy));
for(const [id,mode] of [['v98DetailBody','body'],['v98DetailDrive','drive'],['v98DetailFunction','function']])go('#'+id).onclick=()=>setMode(mode);
// Whole-organism vector microscope: pan, wheel zoom and reset operate on display only.
const detailCanvas=go('#v98AnatomyHuge');detailCanvas.style.touchAction='none';let microscopeDrag=null;
detailCanvas.addEventListener('pointerdown',e=>{microscopeDrag={x:e.clientX,y:e.clientY,px:detailPan.x,py:detailPan.y};detailCanvas.setPointerCapture(e.pointerId);});
detailCanvas.addEventListener('pointermove',e=>{if(!microscopeDrag)return;let rect=detailCanvas.getBoundingClientRect();detailPan.x=microscopeDrag.px+(e.clientX-microscopeDrag.x)*detailCanvas.width/rect.width;detailPan.y=microscopeDrag.py+(e.clientY-microscopeDrag.y)*detailCanvas.height/rect.height;drawAnatomy();});
for(const type of ['pointerup','pointercancel'])detailCanvas.addEventListener(type,()=>{microscopeDrag=null;});
detailCanvas.addEventListener('wheel',e=>{e.preventDefault();detailZoom=Math.max(1,Math.min(128,Math.round(detailZoom*(e.deltaY>0?.8:1.25))));go('#v98DetailZoom').value=String(detailZoom);go('#v98DetailZoomText').textContent='×'+detailZoom+' · 数字放大';drawAnatomy();},{passive:false});
detailCanvas.addEventListener('dblclick',()=>{detailZoom=1;detailPan={x:0,y:0};go('#v98DetailZoom').value='1';go('#v98DetailZoomText').textContent='×1 · 拓扑适配';drawAnatomy();});

function selectLargest(){const a=latest?.individuals||[];if(!a.length)return;let big=a.reduce((x,y)=>x[6]>=y[6]?x:y);focusId=big[0];cameraX=big[1];cameraY=big[2];zoom=Math.max(8,zoom);lifeDossier(latest);drawAnatomy();}
// Derive actually expressed body-node positions from the C++ snapshot body array.
function nodesOf(a){const parts=Array.isArray(a?.[11])?a[11]:[],age=Number(a?.[8])||0,positions=[{x:0,y:0,t:0,active:true}];for(let j=1;j<parts.length;j++){const p=parts[j];const parent=positions[p[0]]||positions[0],theta=parent.t+(Number(p[3])||0)+(Number(p[5])||0),active=p.length>9?!!p[9]:age/210>=Number(p[4]||0);positions.push({x:parent.x+(active?Math.cos(theta)*p[2]:0),y:parent.y+(active?Math.sin(theta)*p[2]:0),t:theta,active,parent:p[0],type:p[1],ctrl:p[6],sense:p[7],motor:p[8],len:p[2],thickness:p.length>=13?Number(p[12]):null,flexibility:p.length>=13?Number(p[11]):null});}return positions;}
const organLabels=['运动','感觉','采集','防护','信号','分解','土壤改造','根式采集'];const organColors=['#4be1d6','#76cce6','#e0bd7c','#a7ddc9','#c8a1e8','#f0b477','#81d7a4','#86dfb0'];
function shinyConnection(g,from,to,col,thickness,alpha=1){g.save();g.globalAlpha*=alpha;g.lineCap='round';const dx=to.x-from.x,dy=to.y-from.y,ang=Math.atan2(dy,dx),length=Math.hypot(dx,dy);g.translate(from.x,from.y);g.rotate(ang);let shell=g.createLinearGradient(0,-thickness,0,thickness);shell.addColorStop(0,'#d8fff2');shell.addColorStop(.16,col);shell.addColorStop(.48,'#183b37');shell.addColorStop(.72,col);shell.addColorStop(1,'#072522');g.fillStyle=shell;g.beginPath();g.moveTo(0,-thickness*.5);g.bezierCurveTo(length*.28,-thickness*.85,length*.7,-thickness*.85,length,-thickness*.33);g.lineTo(length,thickness*.33);g.bezierCurveTo(length*.7,thickness*.85,length*.28,thickness*.85,0,thickness*.5);g.closePath();g.fill();g.strokeStyle='rgba(221,255,242,.63)';g.lineWidth=Math.max(.3,thickness*.12);g.beginPath();g.moveTo(0,-thickness*.38);g.lineTo(length,-thickness*.28);g.stroke();g.restore();}
function sphere(g,x,y,r,col,active=true){const rg=g.createRadialGradient(x-r*.35,y-r*.43,0,x,y,r*1.15);rg.addColorStop(0,'rgba(241,255,246,.96)');rg.addColorStop(.2,col);rg.addColorStop(.5,'#235c56');rg.addColorStop(.77,'#103332');rg.addColorStop(1,'rgba(0,23,27,.82)');g.fillStyle=rg;g.beginPath();g.arc(x,y,r,0,Math.PI*2);g.fill();g.strokeStyle=active?'rgba(183,252,234,.7)':'rgba(118,160,149,.3)';g.lineWidth=r*.075;g.stroke();g.strokeStyle='rgba(220,255,248,.36)';g.lineWidth=Math.max(.2,r*.065);g.beginPath();g.ellipse(x-r*.16,y-r*.24,r*.66,r*.3,-.4,Math.PI*1.04,Math.PI*1.85);g.stroke();}
function drawFunctionalModule(g,p,unit){
 if(!p.active)return;const r=unit,theta=p.t,col=organColors[(p.type||0)%8],type=p.type||0;g.save();g.translate(p.x,p.y);g.rotate(theta);
 g.lineWidth=Math.max(.17,r*.1);g.strokeStyle=col;
 if(type===0){ // joint-driven motor fin, shape follows the inherited module location
  const gr=g.createLinearGradient(0,-r*3,0,r*2);gr.addColorStop(0,'rgba(180,255,245,.57)');gr.addColorStop(.55,'rgba(27,154,140,.23)');gr.addColorStop(1,'rgba(9,41,50,.1)');g.fillStyle=gr;
  g.beginPath();g.moveTo(-r*.4,0);g.bezierCurveTo(-r*1.7,-r*3,r*1.5,-r*3.5,r*2,-r*.4);g.bezierCurveTo(r*3,r*2.1,-r*.5,r*1.3,-r*.4,0);g.closePath();g.fill();g.stroke();
  g.strokeStyle='rgba(221,255,246,.55)';g.beginPath();g.moveTo(0,0);g.quadraticCurveTo(r*.4,-r*1.5,r*1.8,-r*.4);g.stroke();
 }else if(type===1){ // actual sensor-module symbol, not real eye anatomy
  g.strokeStyle='rgba(133,245,250,.87)';g.fillStyle='rgba(48,184,200,.12)';g.beginPath();g.ellipse(r*.8,0,r*2,r*.85,0,0,Math.PI*2);g.fill();g.stroke();g.beginPath();g.ellipse(r*.8,0,r*.55,r*.48,0,0,Math.PI*2);g.stroke();
 }else if(type===2||type===7){ // collector / root filaments
  g.strokeStyle='rgba(130,242,191,.82)';for(let k=-1;k<=1;k++){g.beginPath();g.moveTo(0,0);g.quadraticCurveTo(r*1.5,k*r*1.1,r*2.5,k*r*2);g.stroke();}
 }else if(type===3){ // inherited shield module illustrated as mineral-like translucent plate
  g.fillStyle='rgba(110,236,204,.18)';g.beginPath();g.moveTo(-r*1.9,-r*.6);g.bezierCurveTo(-r*1.6,-r*2.8,r*1.4,-r*3.2,r*2,-r*.5);g.quadraticCurveTo(0,-r*1.5,-r*1.9,-r*.6);g.closePath();g.fill();g.stroke();
 }else if(type===4){g.strokeStyle='rgba(199,158,248,.78)';for(let k=1;k<=2;k++){g.globalAlpha=.7/k;g.beginPath();g.arc(0,0,r*(1+k*.85),0,Math.PI*2);g.stroke();}}
 else if(type===5){g.fillStyle='rgba(241,173,108,.21)';g.beginPath();g.ellipse(0,0,r*1.75,r*1.15,0,0,Math.PI*2);g.fill();g.stroke();g.strokeStyle='rgba(255,232,193,.6)';for(let k=-1;k<=1;k++){g.beginPath();g.moveTo(k*r*.6,-r*.7);g.lineTo(k*r*.6,r*.7);g.stroke();}}
 else if(type===6){g.strokeStyle='rgba(112,214,166,.76)';g.beginPath();g.arc(0,0,r*2.1,-1.4,1.4);g.stroke();}
 g.restore();
}
function renderAnatomy(canvas,a,large=false){const g=canvas.getContext('2d');if(!g)return;const W=canvas.width,H=canvas.height;g.clearRect(0,0,W,H);
 const back=g.createRadialGradient(W*.5,H*.42,15,W*.5,H*.5,W*.75);back.addColorStop(0,'#153b37');back.addColorStop(.45,'#082421');back.addColorStop(1,'#031110');g.fillStyle=back;g.fillRect(0,0,W,H);
 g.strokeStyle='rgba(122,206,185,.07)';g.lineWidth=1;for(let x=0;x<W;x+=Math.max(24,W/24)){g.beginPath();g.moveTo(x,0);g.lineTo(x,H);g.stroke();}for(let y=0;y<H;y+=Math.max(24,H/16)){g.beginPath();g.moveTo(0,y);g.lineTo(W,y);g.stroke();}
 if(!a){g.font=(large?28:21)+'px sans-serif';g.fillStyle='#8daea8';g.textAlign='center';g.fillText('选择一只真实生命 · 观察其遗传身体',W/2,H/2);return;}
 const nodes=nodesOf(a),expressed=nodes.filter(x=>x.active),xs=expressed.map(x=>x.x),ys=expressed.map(x=>x.y);let maxX=Math.max(...xs,5),minX=Math.min(...xs,-5),maxY=Math.max(...ys,5),minY=Math.min(...ys,-5),spanX=Math.max(18,maxX-minX),spanY=Math.max(18,maxY-minY),margin=.23;
 let base=Math.min(W*(1-margin*2)/spanX,H*(1-margin*2)/spanY),r=large?detailZoom:1;const s=base*r;
 g.save();g.translate(W/2+(large?detailPan.x:0)-s*(maxX+minX)/2,H/2+(large?detailPan.y:0)-s*(maxY+minY)/2);g.scale(s,s);
 const unit=Math.max(1.15,Math.min(5,spanX*.017));
 // Geometry = inherited body graph; materials are artistic visualization only.
 for(let j=1;j<nodes.length;j++){let p=nodes[j];if(!p.active)continue;let q=nodes[p.parent]||nodes[0];const col=organColors[(p.type||0)%8];shinyConnection(g,q,p,col,p.thickness!=null?Math.max(.5,p.thickness):unit*(1+Math.abs(p.motor||0)*.16));}
 sphere(g,0,0,unit*2.3,a[12]>.18?'#6ee2bd':a[12]<-.18?'#edc37c':'#b1ece1');
 for(let j=1;j<nodes.length;j++){const p=nodes[j];if(!p.active)continue;const col=organColors[(p.type||0)%8];sphere(g,p.x,p.y,unit*(p.type===3?1.35:1.15),col);drawFunctionalModule(g,p,unit);}
 if(anatomyMode==='drive'){g.save();g.lineWidth=Math.max(.22,unit*.16);g.setLineDash([unit*.7,unit*.48]);let count=0;for(let j=1;j<nodes.length;j++){const p=nodes[j];if(!p.active)continue;const q=nodes[p.parent]||nodes[0];if(!Number.isFinite(p.ctrl))continue;g.strokeStyle='rgba(233,165,253,.84)';g.beginPath();g.moveTo(q.x,q.y);g.quadraticCurveTo((q.x+p.x)/2+unit*2,(q.y+p.y)/2-unit*1.4,p.x,p.y);g.stroke();count++;}g.restore();}
 if(anatomyMode==='function'&&large){g.save();g.textAlign='left';g.font=`${Math.max(1.5,unit*1.2)}px sans-serif`;for(let j=1;j<nodes.length;j++){const p=nodes[j];if(!p.active)continue;g.fillStyle='rgba(230,248,240,.93)';g.fillText(organLabels[(p.type||0)%8],p.x+unit*1.8,p.y-unit*.5);}g.restore();}
 g.restore();g.fillStyle='rgba(151,204,192,.8)';g.font=(large?17:22)+'px system-ui';g.textAlign='left';g.fillText('UID '+a[0]+' · '+nodes.length+' 节点 / '+expressed.length+' 表达 · 第 '+a[5]+' 代',large?25:17,large?H-25:H-19);
 if(anatomyMode==='drive'){g.fillStyle='rgba(210,166,237,.8)';g.font=(large?17:18)+'px system-ui';g.fillText('虚线：部件控制参数映射 · 并非完整神经突触图',large?25:17,large?38:32);}
}
function drawAnatomy(){const a=latest?.individuals?.find(i=>i[0]===focusId)||null;renderAnatomy(go('#v98Anatomy'),a);if(dialog.open)renderAnatomy(go('#v98AnatomyHuge'),a,true);
 for(const [id,value] of [['v98Parts',a?.[6]],['v98Expressed',a?nodesOf(a).filter(x=>x.active).length:null],['v98Neurons',a?.[7]],['v98Feedback',a?.[14]],['v98Gene',a?.[5]],['v98Energy',a?.[4]?.toFixed(2)]])go('#'+id).textContent=value??'—';
 go('#v98AnatomyCaption').textContent=a?`UID #${a[0]} · ${a[6]} 遗传身体节点 · ${a[7]} 神经元 · ${organLabels[(nodesOf(a)[1]?.type||0)%8]}等真实功能模块`:'选择生命后显示实际身体节点、关节与神经驱动。';
 go('#v98DialogUid').textContent=a?'#'+a[0]:'';
 go('#v98AnatomyNote').textContent=a?`节点数 ${a[6]} · 神经元 ${a[7]} · 反馈连接 ${a[14]||0} · 模型 tick ${latest.tick}`:'尚未选择生命';
 if(a){const actual=(latest.bonds||[]).filter(b=>b[0]===a[0]||b[1]===a[0]);let rel=(latest.relationships||[]).filter(b=>b[1]===a[0]||b[2]===a[0]).slice(0,4);const area=go('#v98LiveRelations');area.replaceChildren();if(!actual.length&&!rel.length)area.textContent='当前采样未记录到该个体的活动连接或近期事件';for(const b of actual){let row=make('div','v98-rel-row');row.textContent=`◈ 与 #${b[0]===a[0]?b[1]:b[0]} 连接 · ${b[2]} tick · 组织代数 ${b[7]||0}`;area.append(row);}for(const b of rel){let row=make('div','v98-rel-row');row.textContent=`↗ T${b[0]} · ${names[b[3]]||'互动'} · #${b[1]} → #${b[2]}`;area.append(row);}}
 else{go('#v98LiveRelations').textContent='选择生命后显示实际连接及最近交互';}
}
function drawMap(s){let cv=go('#v98MiniMap'),g=cv.getContext('2d'),W=cv.width,H=cv.height;g.clearRect(0,0,W,H);g.fillStyle='#081d1c';g.fillRect(0,0,W,H);if(!s){g.fillStyle='#759b96';g.fillText('等待真实世界状态',25,H/2);return;}
 const n=s.worldScale||1,pts=s.individuals||[],ww=s.worldWidth||1300*n,hh=s.worldHeight||850*n,pad=16,scx=(W-2*pad)/ww,scy=(H-2*pad)/hh;
 const heat=s.densityGrid||s.nutrientGrid;let gw=16*n,gh=10*n;if(heat?.length===gw*gh){const max=Math.max(1,...heat);for(let y=0;y<gh;y++)for(let x=0;x<gw;x++){let v=(heat[y*gw+x]||0)/max;g.fillStyle=`rgba(${Math.floor(31+78*v)},${Math.floor(81+125*v)},${Math.floor(80+97*v)},${.12+.5*v})`;g.fillRect(pad+x*(W-2*pad)/gw,pad+y*(H-2*pad)/gh,(W-2*pad)/gw+1,(H-2*pad)/gh+1);}}
 g.strokeStyle='rgba(170,214,200,.27)';g.setLineDash([4,5]);for(let i=1;i<n;i++){let x=pad+1300*i*scx,y=pad+850*i*scy;g.beginPath();g.moveTo(x,pad);g.lineTo(x,H-pad);g.moveTo(pad,y);g.lineTo(W-pad,y);g.stroke();}g.setLineDash([]);
 for(const p of pts){g.fillStyle=p[0]===focusId?'#fff2bd':'rgba(139,238,216,.74)';g.beginPath();g.arc(pad+p[1]*scx,pad+p[2]*scy,p[0]===focusId?4:1.7,0,Math.PI*2);g.fill();}
 let px=pad+cameraX*scx,py=pad+cameraY*scy,gx=(W-2*pad)/Math.max(1,zoom),gy=(H-2*pad)/Math.max(1,zoom);g.strokeStyle='#f3df9f';g.lineWidth=1.4;g.strokeRect(Math.max(pad,Math.min(W-pad-gx,px-gx/2)),Math.max(pad,Math.min(H-pad-gy,py-gy/2)),gx,gy);g.fillStyle='#9acabc';g.font='12px system-ui';g.fillText(`${n}×${n} REGIONS · T ${s.tick}`,pad,H-5);
 go('#v98MapPopulation').textContent=`${Number(s.population||0).toLocaleString()} 活跃生命`;go('#v98MapScale').textContent=`${n*n} 区域 · ${ww}×${hh}`;
}
function renderRegions(s){const area=go('#v98RegionList'),reg=s.regions||[];area.replaceChildren();if(!reg.length){area.textContent='区域数据尚未到达';return;}let n=s.worldScale||1;for(let idx=0;idx<reg.length;idx++){let z=reg[idx],b=make('button','v98-region-item');b.type='button';const c=Array.isArray(z)?z:{};const pop=Number(c.population??c[2]??0),gen=c.maxGeneration??c[4]??'—';const rx=idx%n,ry=Math.floor(idx/n);let title=make('strong');title.textContent=`区域 ${rx+1}-${ry+1}`;let sub=make('small');sub.textContent=`活跃 ${pop} · 最高代 ${gen}`;b.append(title,sub);b.onclick=()=>{const j=go('#regionJump');if(j){j.value=`${rx},${ry}`;j.dispatchEvent(new Event('change'));}drawMap(s);};area.append(b);}}
let isPreview=false;previewBtn.onclick=async()=>{try{const response=await fetch('/v98-preview.json',{cache:'no-store'});if(!response.ok)throw Error('无法读取真实历史样本');const s=await response.json();isPreview=true;previewBadge.hidden=false;render(s,true);selectLargest();drawAnatomy();go('#status').textContent='已加载真实历史快照 · 非实时';}catch(e){go('#status').textContent=e.message;}};
go('#config')?.addEventListener('submit',()=>{isPreview=false;previewBadge.hidden=true;},true);
const nativeRender=render;let v99InitialSelection=false;
render=function(s,seek=false){if(!isPreview)previewBadge.hidden=true;nativeRender(s,seek);if(!v99InitialSelection&&s?.individuals?.length){focusId=s.individuals.reduce((a,b)=>a[6]>=b[6]?a:b)[0];v99InitialSelection=true;lifeDossier(s);}drawMap(s);renderRegions(s);drawAnatomy();const a=s.lifeParallelActive===1?'身体只读准备并行启用': '身体并行当前未触发';go('#v98ParallelStatus').textContent=`${a} · ${s.threads||1} 线程`;go('#v98SchedulerStatus').textContent=`${s.worldScale||1}×${s.worldScale||1} 连续区域 · ${s.scheduler===1?'区域任务':'传统调度'} · 全部逐 tick 更新`;};
function adjustZoom(mult){zoom=Math.max(1,Math.min(128,zoom*mult));if(viewScale&&[1,2.2,3.5,8,16,32,64,128].includes(zoom))viewScale.value=String(zoom);}
go('#v98ZoomIn').onclick=()=>adjustZoom(1.6);go('#v98ZoomOut').onclick=()=>adjustZoom(1/1.6);
// Overwrite only the visual glyphs for high-quality vector-backed organisms.
const originalOrganism=organism;
organism=function(map){for(const d of map.values()){
 if(d.x < -80 || d.x > W+80 || d.y < -80 || d.y > H+80)continue;
 const a=d.individual,focused=a[0]===focusId,rich=(focused||zoom>=8);const pts=[[0,0,0]],parts=d.parts||a[11]||[],age=a[8];const power=zoom;
 const pal=organColors,ctx2=ctx;ctx2.save();if(focusId&&a[0]!==focusId)ctx2.globalAlpha=.44;if(selectedEcotype>=0&&a[17]!==selectedEcotype)ctx2.globalAlpha*=.2;ctx2.translate(d.x,d.y);ctx2.rotate(d.h);ctx2.scale(power,power);
 for(let k=1;k<parts.length;k++){const p=parts[k],q=pts[p[0]]||pts[0],ang=q[2]+p[3]+(p[5]||0),active=p.length>9?!!p[9]:age/210>=p[4],X=q[0]+(active?Math.cos(ang)*p[2]*.58:0),Y=q[1]+(active?Math.sin(ang)*p[2]*.58:0);pts.push([X,Y,ang]);if(!active)continue;
 let col=pal[(p[1]||0)%8],width=p.length>=13?Math.max(.5,Number(p[12])):Math.min(2.7,1+Math.abs(p[8]||0)*.3);if(rich){shinyConnection(ctx2,{x:q[0],y:q[1]},{x:X,y:Y},col,width);sphere(ctx2,X,Y,Math.min(2.15,1.25+Math.abs(p[8]||0)*.12),col);if(focused&&zoom>=8)drawFunctionalModule(ctx2,{x:X,y:Y,t:ang,active:true,type:p[1]},1.2);}else{ctx2.strokeStyle=col;ctx2.lineWidth=1.5;ctx2.beginPath();ctx2.moveTo(q[0],q[1]);ctx2.lineTo(X,Y);ctx2.stroke();ctx2.fillStyle=col;ctx2.beginPath();ctx2.arc(X,Y,1.2,0,Math.PI*2);ctx2.fill();}}
 if(rich)sphere(ctx2,0,0,Math.min(6,3+Math.sqrt(Math.max(1,parts.length))*.3),a[12]>.18?'#80eccb':a[12]<-.18?'#e6c78b':'#9ee7db');else{ctx2.fillStyle=a[12]>.18?'#80eccb':a[12]<-.18?'#e6c78b':'#9ee7db';ctx2.beginPath();ctx2.ellipse(0,0,3.1,2.2,0,0,Math.PI*2);ctx2.fill();}
 if(focused){ctx2.strokeStyle='rgba(252,236,174,.76)';ctx2.lineWidth=Math.max(.13,1.2/power);ctx2.setLineDash([3,2]);ctx2.beginPath();ctx2.arc(0,0,9,0,Math.PI*2);ctx2.stroke();}
 ctx2.restore();}}
// Keep selected canvas anatomies synchronized when selection changes outside render().
const oldDossier=lifeDossier;lifeDossier=function(s){oldDossier(s);drawAnatomy();};
const oldSet=go('#followLargest').onclick;go('#followLargest').onclick=(e)=>{oldSet(e);drawAnatomy();};
let lastDetails=0;setInterval(()=>{if(latest&&performance.now()-lastDetails>1200){lastDetails=performance.now();drawMap(latest);drawAnatomy();}},1500);
// Initial static preview without invented life data.
drawAnatomy();drawMap(null);
})();
