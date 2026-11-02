'use strict';
// EvoLab Ω v9.6 Deep-Time Compute and Evolution Observatory; simulation computation remains global and exact. — snapshot interpolation is purely visual.
const $=s=>document.querySelector(s),world=$('#world'),chart=$('#graph'),ctx=world.getContext('2d'),gc=chart.getContext('2d');
const W=world.width,H=world.height;
let latest=null,previous=null,arrival=0,samples=[],running=false,hold=420,focusId=null,selectedEvent=null,relationFilter='all',seenTick=-1;
let zoom=2.2,cameraX=650,cameraY=425,worldWidth=1300,worldHeight=850,worldScale=1;
let liveFrame=null,historyFrames=[],historicTick=null,selectedRate=1,serverPhase='idle',serverPaused=false;
let frameInterval=800,lastFrameArrived=0,dragged=false,pointerStart=null;
let selectedRegion=-1;let selectedEcotype=-1;let ecologyLayer='nutrient',worldTimeStamp=0,organismMoments=[],lastMoment=null,lastRenderTick=-1;
const displayState={};const sampleMomentMax=24;
// Observer-only sample history: no code here changes real native reproduction.
// The archive covers only sampled living individuals, NOT every historical birth.
const sampledAncestry=new Map();const maxSampledAncestors=2500;
function observeSampledLineage(state){
 if(!Array.isArray(state?.lineageLinks))return;
 for(const link of state.lineageLinks){
  if(!Array.isArray(link)||link.length<9)continue;
  const uid=Number(link[0]);if(!Number.isSafeInteger(uid))continue;
  if(!sampledAncestry.has(uid)&&sampledAncestry.size>=maxSampledAncestors){
    sampledAncestry.delete(sampledAncestry.keys().next().value);
  }
  sampledAncestry.set(uid,{link:link.slice(),tick:state.tick});
 }
}

const roleNames=['资源采集','残骸利用','运动探索','环境改造','混合策略'];
const roleColors=['#94d8b8','#c6aaea','#8cc7e6','#e2b780','#8ea79c'];
function addMoment(t,kind,summary){if(organismMoments.some(m=>m.tick===t&&m.kind===kind))return;
 organismMoments.unshift({tick:t,kind,summary});if(organismMoments.length>sampleMomentMax)organismMoments.pop();
 const area=$('#evolutionMoments');if(!area)return;area.replaceChildren();for(const moment of organismMoments.slice(0,10)){
 const div=document.createElement('div');div.className='momentCard';const label=document.createElement('small');label.textContent='T '+fmt(moment.tick)+' / '+moment.kind;const desc=document.createElement('strong');desc.textContent=moment.summary;div.append(label,desc);area.append(div);}
}
function recordMoments(s,old){if(!old||s.tick<=old.tick)return;
 if(s.maxGeneration>old.maxGeneration)addMoment(s.tick,'新世代','谱系达到第 '+fmt(s.maxGeneration)+' 代');
 if(s.structuralMutations>old.structuralMutations)addMoment(s.tick,'结构改变','区间内 '+fmt(s.structuralMutations-old.structuralMutations)+' 次遗传结构突变');
 if(s.subtreeCopies>old.subtreeCopies)addMoment(s.tick,'身体拓扑','新增 '+fmt(s.subtreeCopies-old.subtreeCopies)+' 次身体分支复制');
 if(s.feedbackMutations>old.feedbackMutations)addMoment(s.tick,'神经反馈','新增 '+fmt(s.feedbackMutations-old.feedbackMutations)+' 次递归神经变异');
 if(s.livingClasses>old.livingClasses)addMoment(s.tick,'结构多样性','存活结构签名增加 '+fmt(s.livingClasses-old.livingClasses));
 if((s.bodyArchitectures||0)>(old.bodyArchitectures||0))addMoment(s.tick,'身体发育结构','可观察身体拓扑增加 '+fmt(s.bodyArchitectures-old.bodyArchitectures));
 if(s.activeBonds>old.activeBonds)addMoment(s.tick,'生命连接','活动组织连接增加 '+fmt(s.activeBonds-old.activeBonds));
 if(s.propagulesBorn>old.propagulesBorn)addMoment(s.tick,'组织传承','出现 '+fmt(s.propagulesBorn-old.propagulesBorn)+' 次联合后代事件');
 if(s.soilEdits>old.soilEdits)addMoment(s.tick,'环境反馈','生命局部环境改造 '+fmt(s.soilEdits-old.soilEdits)+' 次');
 if(s.developmentMutations>old.developmentMutations)addMoment(s.tick,'发育分化','发育时序或驱动改变 '+fmt(s.developmentMutations-old.developmentMutations)+' 次');
 if(s.nicheHarvests>old.nicheHarvests)addMoment(s.tick,'资源生态位','新生态位摄取活动 '+fmt(s.nicheHarvests-old.nicheHarvests)+' 次');
 if(s.transportEvents>old.transportEvents)addMoment(s.tick,'空间生态流','残骸和化学信号在相邻网格间真实扩散，累计步数 '+fmt(s.transportEvents));
 if(s.extendedHarvests>old.extendedHarvests)addMoment(s.tick,'空间采集','身体末端在邻近资源区完成 '+fmt(s.extendedHarvests-old.extendedHarvests)+' 次觅食');
 if(s.ecotypes&&old.ecotypes){for(let k=0;k<5;k++){let a=s.ecotypes[k]?.[1]||0,b=old.ecotypes[k]?.[1]||0;if(a>b+Math.max(8,Math.round(old.population*.08)))addMoment(s.tick,'生态位变化',roleNames[k]+' 增加 '+fmt(a-b)+' 个体');}}
 if(s.scavenges>old.scavenges)addMoment(s.tick,'新生态活动','区间内发生 '+fmt(s.scavenges-old.scavenges)+' 次残骸利用');
}
function portrait(a){const cv=$('#lifePortrait');if(!cv)return;const ctx=cv.getContext('2d'),w=cv.width,h=cv.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0a2220';ctx.fillRect(0,0,w,h);const light=ctx.createRadialGradient(w*.5,h*.48,5,w*.5,h*.48,w*.54);light.addColorStop(0,'rgba(89,161,145,.17)');light.addColorStop(1,'rgba(10,24,28,0)');ctx.fillStyle=light;ctx.fillRect(0,0,w,h);
 if(!a){ctx.fillStyle='#7ea99e';ctx.font='12px system-ui';ctx.textAlign='center';ctx.fillText('选择生命 · 查看身体发育结构',w/2,h/2);return;}
 const parts=a[11]||[],n=parts.length,age=a[8],pts=[[0,0,0]],active=[];
 for(let j=1;j<n;j++){let [parent,type,len,angle,growth,joint]=parts[j],q=pts[parent]||pts[0],theta=q[2]+angle+joint,isActive=age/210>=growth,x=q[0]+(isActive?Math.cos(theta)*len:0),y=q[1]+(isActive?Math.sin(theta)*len:0);pts.push([x,y,theta]);if(isActive)active.push([j,parent,type]);}
 let xs=pts.map(p=>p[0]),ys=pts.map(p=>p[1]),minx=Math.min(0,...xs),maxx=Math.max(0,...xs),miny=Math.min(0,...ys),maxy=Math.max(0,...ys),scale=Math.min((w-55)/Math.max(12,maxx-minx),(h-44)/Math.max(12,maxy-miny),5.3),cx=w/2-(minx+maxx)/2*scale,cy=h/2-(miny+maxy)/2*scale;
 ctx.save();ctx.translate(cx,cy);ctx.scale(scale,scale);ctx.lineCap='round';const pal=['#7cdbce','#b7d9ed','#e4d19b','#e4be85','#cabbea','#b6a8ef','#76d9a1','#e9c58e'];
 for(const [j,parent,type] of active){let q=pts[parent]||pts[0],z=pts[j];ctx.strokeStyle=pal[type%pal.length];ctx.lineWidth=2/scale+1.8;ctx.beginPath();ctx.moveTo(q[0],q[1]);ctx.lineTo(z[0],z[1]);ctx.stroke();ctx.beginPath();ctx.fillStyle=pal[type%pal.length];ctx.arc(z[0],z[1],1.5,0,Math.PI*2);ctx.fill();}
 ctx.shadowColor='#b1eedc';ctx.shadowBlur=12/scale;ctx.fillStyle=a[12]>.18?'#65cdb0':a[12]<-.18?'#d9ba88':'#9fcfbd';ctx.beginPath();ctx.ellipse(0,0,4.5,3.1,0,0,Math.PI*2);ctx.fill();ctx.restore();
 ctx.fillStyle='#96b8b0';ctx.font='10px system-ui';ctx.textAlign='left';ctx.fillText(n+' 发育节点 · '+active.length+' 已表达',11,h-11);
}
function lifeDossier(s){const el=$('#lifeDetails');if(!el)return;let a=s?.individuals?.find(i=>i[0]===focusId);portrait(a);if(!a){el.textContent=focusId?'当前采样中未发现这只生命；它可能死亡或离开抽样范围。':'点击生态场中的生命，查看其实际身体与遗传状态。';return;}
 const currentLink=(s.lineageLinks||[]).find(x=>x[0]===focusId);
 const tracked=currentLink||sampledAncestry.get(focusId)?.link;
 const parent=tracked?.[1] ? sampledAncestry.get(tracked[1])?.link : null;
 const inherited=tracked?[['父代 UID',tracked[1]||'原始个体'],['始祖 UID',tracked[2]],['遗传结构哈希',tracked[4]],['功能器官（吸收/分解/根）',tracked.slice(6,9).join(' / ')]]:[];
 if(parent){
   const diffs=[6,7,8].map(idx=>tracked[idx]-parent[idx]);
   inherited.push(['与观测父代的器官数差',diffs.map(d=>d>0?'+'+d:String(d)).join(' / ')]);
 }
 const organismTypes=[2,5,7].map(t=>(a[11]||[]).filter(n=>n[1]===t).length);
 const scale=Number(s.worldScale||1),zoneX=Math.min(scale-1,Math.floor(a[1]/1300)),zoneY=Math.min(scale-1,Math.floor(a[2]/850));
 const region=s.regionProfiles?.[zoneY*scale+zoneX];
 const populationEvidence=[['真实器官（吸收/分解/根）',organismTypes.join(' / ')],
  ['当前所在区域',`${zoneX+1} × ${zoneY+1}`],
  ['区域资源供给系数',region?.resourceFactor??'未提供'],
  ['区域活跃种群',region?.population??'未提供'],
  ['全世界同类器官携带者',(s.functionalCarriers||[]).join(' / ')||'未提供'],
  ['本次采样的全群器官摄取',(s.functionalUptake||[]).map(v=>Number(v).toFixed(3)).join(' / ')||'未提供']];
 const details=[...inherited,...populationEvidence,['世代',a[5]],['生命年龄',fmt(a[8])+' tick'],['能量',fmt(a[4])],['身体部件',a[6]],['神经元',a[7]],['遗传功能倾向',a[12]>.18?'资源采集':a[12]<-.18?'防护 / 残骸机会':'混合'],['反馈连接',a[14]||0],['资源梯度神经',a[18]||0],['遗传资源偏好',(a[15]||0).toFixed(2)],['环境改造倾向',(a[16]||0).toFixed(2)],['组织连接',fmt((s.bonds||[]).filter(b=>b[0]===focusId||b[1]===focusId).length)]];
 el.replaceChildren();const title=document.createElement('strong');title.className='dossierName';title.textContent='生命 #'+focusId;el.append(title);for(let [k,v] of details){const cell=document.createElement('div');cell.className='dossierValue';const small=document.createElement('small');small.textContent=k;const b=document.createElement('b');b.textContent=v;cell.append(small,b);el.append(cell);}
 if(tracked){const hint=document.createElement('small');hint.style.cssText='grid-column:1/-1;color:#87b9ab;line-height:1.5';hint.textContent=parent?'父代与子代均有真实采样记录；数量差仅描述继承结果。区域和全群摄取不是个人因果效应，也不证明适应优势。':'仅显示当前生命的真实遗传记录；父代可能没有进入采样，不能推断其具体变异。';el.append(hint);
   if(parent){const back=document.createElement('button');back.type='button';back.textContent='定位已观测父代 #'+tracked[1];back.style.cssText='grid-column:1/-1;background:#153c35;color:#bbebd9;border:1px solid #4d8270;border-radius:7px;padding:8px';back.onclick=()=>{const match=s.individuals?.find(x=>x[0]===tracked[1]);if(match){focusId=match[0];cameraX=match[1];cameraY=match[2];lifeDossier(s);}else{back.textContent='父代仅有历史采样，当前视野已无法定位';}};el.append(back);}
 }
}

const speedButtons=[...document.querySelectorAll('[data-rate]')];

const nutrient=document.createElement('canvas');nutrient.width=16;nutrient.height=10;const ng=nutrient.getContext('2d');
const colors={1:'#8dcac2',2:'#e69a74',3:'#a9b8f4',4:'#f5d88b',5:'#baabb0',6:'#fff0bd',7:'#7ce6ec',8:'#a9d9a2'};
const names={1:'资源转移',2:'掠夺',3:'亲缘分享',4:'形成连接',5:'连接断开',6:'联合后代',7:'伙伴防护',8:'残骸利用'};
const labels={generation:'maxGeneration',population:'population',births:'births',classes:'livingClasses',nodes:'meanNodes',bodyArchitectures:'bodyArchitectures',largeBodyCarriers:'largeBodyCarriers',neurons:'meanNeurons',mutations:'structuralMutations',predations:'predations',shares:'shares',thermal:'meanThermalMismatch',activeBonds:'activeBonds',connectedGroups:'connectedGroups',largestGroup:'largestGroup',complementaryPairs:'complementaryPairs',propagulesBorn:'propagulesBorn',propagulesMatured:'propagulesMatured',organizationLineages:'organizationLineages',maxOrganizationGeneration:'maxOrganizationGeneration',descendantOrganizationBirths:'descendantOrganizationBirths',cooperativeDefenses:'cooperativeDefenses',preventedLoss:'preventedLoss',integratedEvents:'integratedEvents',integratedHarvest:'integratedHarvest',integratedCost:'integratedCost',productiveOrganizations:'productiveOrganizations',scavenges:'scavenges',scavengedEnergy:'scavengedEnergy',subtreeCopies:'subtreeCopies',reparentedBranches:'reparentedBranches',feedbackMutations:'feedbackMutations',soilEdits:'soilEdits',soilMineralizations:'soilMineralizations',nicheHarvests:'nicheHarvests',developmentMutations:'developmentMutations',recycledMatter:'recycledMatter',nicheSensorCarriers:'nicheSensorCarriers',habitatOrganCarriers:'habitatOrganCarriers',meanNiche:'meanNiche'};
const fmt=x=>typeof x==='number'?x.toLocaleString('zh-CN',{maximumFractionDigits:2}):'—';
const text=(id,value)=>{let el=$('#'+id);if(el&&el.textContent!==String(value))el.textContent=String(value);};
const lerp=(a,b,t)=>a+(b-a)*t;
function coords(){let map=new Map(),alpha=historicTick!==null?1:Math.min(1,(performance.now()-arrival)/frameInterval);let old=new Map((previous?.individuals||[]).map(a=>[a[0],a]));
 for(const individual of latest?.individuals||[]){let a=old.get(individual[0]),x=individual[1],y=individual[2],h=individual[3];
  if(a&&Math.hypot(x-a[1],y-a[2])<230){x=lerp(a[1],x,alpha);y=lerp(a[2],y,alpha);let dh=Math.atan2(Math.sin(h-a[3]),Math.cos(h-a[3]));h=a[3]+dh*alpha;}
  let delta=0,oldParts=a?.[11];let parts=individual[11];
  // Only interpolate joint angles for the same tracked individual and matching topology.
  if(a&&oldParts&&parts&&oldParts.length===parts.length&&alpha<1){parts=parts.map((n,k)=>{let q=oldParts[k];if(!q||q[0]!==n[0]||q[1]!==n[1])return n;let diff=Math.atan2(Math.sin(n[5]-q[5]),Math.cos(n[5]-q[5]));return [...n.slice(0,5),q[5]+diff*alpha,...n.slice(6)];});}
  map.set(individual[0],{x:W/2+(x-cameraX)*zoom*W/worldWidth,y:H/2+(y-cameraY)*zoom*H/worldHeight,h,individual,parts,worldX:x,worldY:y});
 }return map;}
function background(){let grad=ctx.createLinearGradient(0,0,W,H);grad.addColorStop(0,'#122923');grad.addColorStop(.50,'#0b1f1b');grad.addColorStop(1,'#07130f');ctx.fillStyle=grad;ctx.fillRect(0,0,W,H);
 // Render sampled resource field using engine values (not painted fake ecology).
 const heat=ecologyLayer==='soil'?latest?.soilGrid:ecologyLayer==='detritus'?latest?.detritusGrid:ecologyLayer==='ecotype'?latest?.ecotypeGrid:ecologyLayer==='habitat'?latest?.habitatGrid:ecologyLayer==='density'?latest?.densityGrid:ecologyLayer==='regional'?(()=>{const n=Number(latest?.worldScale||1),w=16*n,h=10*n,prof=latest?.regionProfiles||[];return Array.from({length:w*h},(_,i)=>prof[Math.floor(i/w/10)*n+Math.floor(i%w/16)]?.resourceFactor??1);})():ecologyLayer==='off'?null:latest?.resourceGrid;
 const gridW=16*worldScale,gridH=10*worldScale;
 if(heat?.length===gridW*gridH){if(nutrient.width!==gridW||nutrient.height!==gridH){nutrient.width=gridW;nutrient.height=gridH;}let data=ng.createImageData(gridW,gridH);for(let i=0;i<gridW*gridH;i++){let f=Math.max(0,Math.min(1,heat[i]/(ecologyLayer==='soil'?1:ecologyLayer==='habitat'?2:ecologyLayer==='detritus'?9:ecologyLayer==='density'?Math.max(1,Math.ceil((latest?.population||100)/80)):40))),j=i*4;
 if(ecologyLayer==='regional'){let v=heat[i];data.data[j]=Math.round(73+Math.max(0,v-.5)*120);data.data[j+1]=Math.round(100+Math.max(0,1.5-v)*95);data.data[j+2]=Math.round(135+Math.max(0,1.3-v)*70);data.data[j+3]=157;continue;}
 if(ecologyLayer==='ecotype'){let v=heat[i];if(v<0){data.data[j+3]=0;continue;}const rgb=[[148,216,184],[198,170,234],[140,199,230],[226,183,128],[142,167,156]][v]||[100,140,130];data.data[j]=rgb[0];data.data[j+1]=rgb[1];data.data[j+2]=rgb[2];data.data[j+3]=153;continue;}
 if(ecologyLayer==='density'){data.data[j]=Math.round(48+190*f);data.data[j+1]=Math.round(52+156*f);data.data[j+2]=Math.round(84+96*f);}else if(ecologyLayer==='habitat'){data.data[j]=Math.round(90+160*f);data.data[j+1]=Math.round(35+156*f);data.data[j+2]=Math.round(75+63*f);}else if(ecologyLayer==='soil'){data.data[j]=Math.round(46+115*f);data.data[j+1]=Math.round(52+142*f);data.data[j+2]=Math.round(36+65*f);}else if(ecologyLayer==='detritus'){data.data[j]=Math.round(70+170*f);data.data[j+1]=Math.round(66+93*f);data.data[j+2]=Math.round(95+32*f);}else{data.data[j]=Math.round(39+55*f);data.data[j+1]=Math.round(82+118*f);data.data[j+2]=Math.round(75+75*f);}data.data[j+3]=Math.round(35+145*f);}
ng.putImageData(data,0,0);ctx.save();ctx.globalAlpha=.59;ctx.imageSmoothingEnabled=true;ctx.drawImage(nutrient,W/2-cameraX*zoom*W/worldWidth,H/2-cameraY*zoom*H/worldHeight,W*zoom,H*zoom);ctx.restore();}
 // Contours are aesthetic geographic texture, not measured height contours.
 for(let j=0;j<9;j++){ctx.strokeStyle='rgba(105,156,122,'+(j%2?.065:.09)+')';ctx.lineWidth=1;ctx.beginPath();for(let x=0;x<=W;x+=8){let y=H*(j+1)/10+Math.sin(x*.013+j)*17+Math.cos(x*.029+j*2)*9;if(x)ctx.lineTo(x,y);else ctx.moveTo(x,y);}ctx.stroke();}
 for(let x=0;x<W;x+=55){ctx.strokeStyle='rgba(170,202,162,.025)';ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,H);ctx.stroke();}
}
function bondCanvas(m){
  if(!latest?.bonds)return;
  // Active bonds are physical links, not merely recent contact events.
  let count=0;
  for(const b of latest.bonds){let p=m.get(b[0]),q=m.get(b[1]);if(!p||!q)continue;
    let dx=q.x-p.x,dy=q.y-p.y,d=Math.hypot(dx,dy);if(d<5||d>W*.8)continue;
    let ux=dx/d,uy=dy/d,r0=(6+Math.min(7,p.individual[6]*.18))*zoom,r1=(6+Math.min(7,q.individual[6]*.18))*zoom;
    ctx.save();ctx.strokeStyle=(b[7]>=2)?'#7ce6ec':b[4]?'#fff0bd':'#e7d49b';ctx.globalAlpha=b[4]?.97:.75;ctx.lineWidth=b[4]?3.5:2.5;ctx.shadowBlur=b[4]?14:7;ctx.shadowColor=(b[7]>=2)?'#7ce6ec':b[4]?'#fff1c1':'#dbbd71';ctx.beginPath();ctx.moveTo(p.x+ux*r0,p.y+uy*r0);ctx.lineTo(q.x-ux*r1,q.y-uy*r1);ctx.stroke();ctx.restore();
    count++;if(count>=90)break;
  }
}
function relationCanvas(m){if(!latest)return;let rels=(latest.relationships||latest.interactions||[]).filter(e=>latest.tick-e[0]<=hold&&(!selectedEvent||e[1]===selectedEvent[1]&&e[2]===selectedEvent[2]));rels.sort((a,b)=>b[0]-a[0]);let shown=0;
 for(const e of rels){if(e[3]===4||e[3]===5||e[3]===6)continue;const [t,a,b,type,count=1]=e;if(relationFilter!=='all'&&String(type)!==relationFilter)continue;let p=m.get(a),q=m.get(b);if(!p||!q)continue;let dx=q.x-p.x,dy=q.y-p.y,d=Math.hypot(dx,dy);if(d<8||d>W*.75)continue;let age=latest.tick-t,alpha=.22+.73*Math.pow(Math.max(0,1-age/hold),.7);let vx=dx/d,vy=dy/d,st=(5+Math.min(9,p.individual[6]*.20))*zoom,en=(6+Math.min(9,q.individual[6]*.20))*zoom,ax=p.x+vx*st,ay=p.y+vy*st,bx=q.x-vx*en,by=q.y-vy*en;
  ctx.save();ctx.globalAlpha=alpha;ctx.strokeStyle=colors[type]||'#adbcae';ctx.fillStyle=ctx.strokeStyle;ctx.lineWidth=(selectedEvent&&selectedEvent[1]===a&&selectedEvent[2]===b)?3:Math.min(2.6,.9+Math.log2(count+1)*.26);if(age>42)ctx.setLineDash([5,6]);let bend=Math.min(12,d*.06)*(type%2?1:-1),cx=(ax+bx)*.5-vy*bend,cy=(ay+by)*.5+vx*bend;
  ctx.beginPath();ctx.moveTo(ax,ay);ctx.quadraticCurveTo(cx,cy,bx,by);ctx.stroke();ctx.setLineDash([]);
  let angle=Math.atan2(by-cy,bx-cx),size=Math.min(9,4.5+zoom*.7);ctx.beginPath();ctx.moveTo(bx,by);ctx.lineTo(bx-Math.cos(angle-.55)*size,by-Math.sin(angle-.55)*size);ctx.lineTo(bx-Math.cos(angle+.55)*size,by-Math.sin(angle+.55)*size);ctx.closePath();ctx.fill();
  if(age<30){let speed=performance.now()*.00028,progress=(speed+((a%17)*.071))%1.0;let px=(1-progress)*(1-progress)*ax+2*(1-progress)*progress*cx+progress*progress*bx,py=(1-progress)*(1-progress)*ay+2*(1-progress)*progress*cy+progress*progress*by;ctx.globalAlpha*=.85;ctx.beginPath();ctx.arc(px,py,Math.min(3.3,1.5+zoom*.3),0,Math.PI*2);ctx.fill();}
  ctx.restore();shown++;if(shown>=45)break;}
 text('linksCount',`${shown} 条可见关系 · ${rels.length} 条记录`);
}
function organism(m){for(let d of m.values()){let i=d.individual,[id,x,y,h,energy,gen,nodes,neur,age,vx,vy,origParts,roleBias=0]=i;let parts=d.parts||origParts;let stage=Math.min(1,gen/Math.max(8,latest.maxGeneration)),baseHue=126+stage*62,r=3+Math.min(2.4,nodes*.15),isFocus=id===focusId;ctx.save();if(focusId&&id!==focusId)ctx.globalAlpha=.61;if(selectedEcotype>=0&&i[17]!==selectedEcotype)ctx.globalAlpha*=.16;ctx.translate(d.x,d.y);ctx.scale(zoom,zoom);ctx.rotate(d.h);
  let pts=[[0,0,0]];for(let j=1;j<(parts?.length||0);j++){let [parent,type,len,angle,growth,joint=0,ctrl=0,sense=0,motor=.5]=parts[j];let q=pts[parent]||pts[0],th=q[2]+angle+joint,active=age/210>=growth,xx=q[0]+(active?Math.cos(th)*len*.58:0),yy=q[1]+(active?Math.sin(th)*len*.58:0);pts.push([xx,yy,th]);if(!active)continue;const palette=['#8addbd','#a9c9e8','#c0bfa8','#e4bc86','#c6b0de','#ad9ce4','#6bdf9b','#e9c07b'];ctx.lineCap='round';ctx.lineWidth=isFocus?3.4:1.5+Math.min(1.8,nodes/15);ctx.strokeStyle=palette[type%palette.length];ctx.beginPath();ctx.moveTo(q[0],q[1]);ctx.lineTo(xx,yy);ctx.stroke();ctx.fillStyle=palette[type%palette.length];ctx.beginPath();ctx.arc(xx,yy,(isFocus?2.6:1.6)+Math.min(1.25,Math.abs(motor)*.65),0,Math.PI*2);ctx.fill();}
  if(isFocus){ctx.shadowColor='#c7f0d1';ctx.shadowBlur=12;ctx.strokeStyle='#e2eed3';ctx.lineWidth=1.5;ctx.beginPath();ctx.arc(0,0,r+8,0,Math.PI*2);ctx.stroke();ctx.shadowBlur=0;}
  ctx.shadowColor=isFocus?"#a9e3c5":"transparent";ctx.shadowBlur=isFocus?8:0;ctx.fillStyle=roleBias>.18?'#65cdb0':roleBias<-.18?'#d9ba88':`hsl(${baseHue},${44+Math.min(30,nodes)}%,${47+stage*12}%)`;ctx.strokeStyle='rgba(215,240,220,.6)';ctx.lineWidth=.8;ctx.beginPath();ctx.ellipse(0,0,r*1.3,r*.74,0,0,Math.PI*2);ctx.fill();ctx.stroke();ctx.strokeStyle='#ccebd8';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(0,0);ctx.lineTo(r*1.7,0);ctx.stroke();ctx.restore();}
}
function drawWorld(){background();if(worldScale>1){ctx.save();ctx.strokeStyle='rgba(225,239,195,.38)';ctx.setLineDash([9,8]);ctx.lineWidth=1.3;
 for(let t=1;t<worldScale;t++){let x=W/2+(1300*t-cameraX)*zoom*W/worldWidth,y=H/2+(850*t-cameraY)*zoom*H/worldHeight;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,H);ctx.stroke();ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(W,y);ctx.stroke();}
 ctx.restore();}if(!latest?.individuals?.length){ctx.fillStyle='#9fb8a8';ctx.font='15px system-ui';ctx.textAlign='center';ctx.fillText('启动原生演化实验，以查看真实种群抽样',W/2,H/2);return;}let m=coords();relationCanvas(m);bondCanvas(m);organism(m);
 ctx.fillStyle='#c8d8ca';ctx.font='12px system-ui';ctx.textAlign='left';ctx.fillText(`T ${fmt(latest.tick)} · 抽样 ${latest.individuals.length} / ${fmt(latest.population)} 个体`,19,27);
 if(focusId){let d=m.get(focusId);if(d){ctx.strokeStyle='rgba(240,246,210,.35)';ctx.beginPath();ctx.arc(d.x,d.y,18*zoom,0,Math.PI*2);ctx.stroke();}}
}
function drawChart(){let w=chart.width,h=chart.height;gc.clearRect(0,0,w,h);gc.strokeStyle='#344f42';gc.lineWidth=1;for(let k=0;k<4;k++){let y=20+k*(h-40)/3;gc.beginPath();gc.moveTo(20,y);gc.lineTo(w-15,y);gc.stroke();}if(samples.length<2)return;let maxTick=samples[samples.length-1].tick||1;
 for(let [key,color] of [['population','#94d0b7'],['maxGeneration','#e2be81'],['meanNodes','#a3b9de'],['connectedGroups','#d5bf80'],['propagulesBorn','#f7e2b0']]){let v=Math.max(1,...samples.map(s=>s[key]||0));gc.strokeStyle=color;gc.lineWidth=2;gc.beginPath();samples.forEach((s,i)=>{let x=20+(w-40)*s.tick/maxTick,y=h-20-(h-45)*(s[key]||0)/v;i?gc.lineTo(x,y):gc.moveTo(x,y);});gc.stroke();}}
function eventLog(){let el=$('#eventList');if(!el||!latest)return;let seen=new Set(),list=[];
 for(let e of (latest.relationships||[])){let key=e[1]+':'+e[2]+':'+e[3];if(seen.has(key))continue;seen.add(key);list.push(e);if(list.length===12)break;}
 if(!list.length){el.textContent='尚未捕获到存活样本中的关系；交互仍可能发生在抽样视野外。';return;}
 el.replaceChildren();for(const e of list){let b=document.createElement('button');b.type='button';b.className='event';b.style.setProperty('--event',colors[e[3]]||'#bbb');let age=latest.tick-e[0];b.textContent=`${names[e[3]]||'互动'} · #${e[1]} → #${e[2]} · ${e[4]||1}次 · ${age} tick前`;b.onclick=()=>{selectedEvent=e;focusId=e[1];let ca=latest.individuals?.find(i=>i[0]===e[1]),cb=latest.individuals?.find(i=>i[0]===e[2]);if(ca&&cb){cameraX=(ca[1]+cb[1])/2;cameraY=(ca[2]+cb[2])/2;}else if(ca){cameraX=ca[1];cameraY=ca[2];}text('selectedRelation',`已聚焦 ${names[e[3]]||'互动'} #${e[1]} → #${e[2]}，历史连接可能已结束。`);};el.append(b);}}
function renderEcotypes(s){const bars=$('#ecotypeBands'),cards=$('#ecotypeCards');if(!bars||!cards)return;
 const groups=s.ecotypes||[];let total=groups.reduce((a,g)=>a+(g[1]||0),0);text('ecotypeTotal',fmt(total)+' 活跃生命 · 5 个功能类');
 bars.replaceChildren();cards.replaceChildren();
 for(let k=0;k<5;k++){const a=groups[k]||[k,0,0,0,0],n=a[1]||0;
  const segment=document.createElement('div');segment.className='ecotypeSegment';segment.style.background=roleColors[k];segment.style.flex=String(Math.max(0,n));segment.title=roleNames[k]+' '+n;bars.append(segment);
  const card=document.createElement('button');card.type='button';card.className='ecotypeCard';card.classList.toggle('selected',selectedEcotype===k);card.style.setProperty('--role',roleColors[k]);card.onclick=()=>{selectedEcotype=selectedEcotype===k?-1:k;renderEcotypes(latest);};
  const title=document.createElement('strong');title.textContent=roleNames[k];const qty=document.createElement('b');qty.textContent=fmt(n);
  const note=document.createElement('small');note.textContent='占比 '+(total?(n/total*100).toFixed(1):0)+'% · 平均部件 '+Number(a[2]||0).toFixed(1);
  card.append(title,qty,note);cards.append(card);}
 drawEcotypeTimeline();
}
function drawEcotypeTimeline(){const canvas=$('#ecotypeTimeline');if(!canvas)return;let data=samples.filter(x=>Array.isArray(x.ecotypes)&&x.ecotypes.length===5);let g=canvas.getContext('2d'),w=canvas.width,h=canvas.height;g.clearRect(0,0,w,h);g.fillStyle='#0d211f';g.fillRect(0,0,w,h);g.strokeStyle='rgba(144,195,173,.13)';for(let z=1;z<4;z++){g.beginPath();g.moveTo(0,z*h/4);g.lineTo(w,z*h/4);g.stroke();}
 if(data.length<2){g.fillStyle='#7ba79b';g.font='18px system-ui';g.textAlign='center';g.fillText('至少记录两次采样后显示真实生态型变化',w/2,h/2);return;}
 let total=data.length;for(let role=0;role<5;role++){g.beginPath();for(let i=0;i<total;i++){let s=data[i],sum=s.ecotypes.reduce((a,v)=>a+(v?.[1]||0),0)||1,below=s.ecotypes.slice(0,role).reduce((a,v)=>a+(v?.[1]||0),0)/sum,top=below+(s.ecotypes[role]?.[1]||0)/sum;let x=i/(total-1)*w,y=h*(1-top);if(i===0)g.moveTo(x,y);else g.lineTo(x,y);}for(let i=total-1;i>=0;i--){let s=data[i],sum=s.ecotypes.reduce((a,v)=>a+(v?.[1]||0),0)||1,below=s.ecotypes.slice(0,role).reduce((a,v)=>a+(v?.[1]||0),0)/sum;g.lineTo(i/(total-1)*w,h*(1-below));}g.closePath();g.fillStyle=roleColors[role];g.globalAlpha=role===selectedEcotype||selectedEcotype<0?.84:.24;g.fill();g.globalAlpha=1;}
 g.fillStyle='#d7eee1';g.font='15px system-ui';g.fillText('T '+fmt(data[0].tick),12,h-9);g.textAlign='right';g.fillText('T '+fmt(data[total-1].tick),w-12,h-9);g.textAlign='left';}
async function inspectRegion(x,y){const panel=$('#regionDetail');if(!panel)return;panel.textContent='读取区域实际快照…';
 try{const r=await fetch(`/api/native/region?x=${x}&y=${y}`,{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);const d=await r.json(),q=d.profile||{};
  const mean=(arr)=>Array.isArray(arr)&&arr.length?arr.reduce((a,b)=>a+(Number(b)||0),0)/arr.length:0;
  panel.textContent=`REGION ${x+1}—${y+1} · T ${fmt(d.tick)} · ${fmt(q.population||0)} 活跃生命 · 采样 ${fmt(d.individuals?.length||0)} · `+
    `创始祖先谱系 ${fmt(q.founderLineages||0)} · 采样营养均值 ${mean(d.resourceGrid).toFixed(2)} · 采样残骸均值 ${mean(d.detritusGrid).toFixed(2)}。`+
    `此接口按需提取区域数据；不停止、加速或近似该区域的物理模拟。`;
 }catch(e){panel.textContent=`区域观测暂不可用：${e.message}`;}
}
function updateRegions(s){if(!s)return;const n=Number(s.worldScale||1),regions=s.regions||[], profiles=s.regionProfiles||[],matrix=s.migrationMatrix||[];
 text('worldDimensions',`${n}×${n} 区 · ${s.worldWidth||1300}×${s.worldHeight||850} 单位 · ${s.worldGridWidth||65}×${s.worldGridHeight||43} 网格`);
 text('regionalMigrants',fmt(s.regionalMigrants||0));text('crossRegionMatter',fmt(s.crossRegionMatter||0));
 const area=$('#regionCards'),jump=$('#regionJump');if(!area||!jump)return;
 const expected=n*n;if(jump.options.length!==expected+1){jump.replaceChildren();jump.add(new Option('整个世界','overview'));for(let y=0;y<n;y++)for(let x=0;x<n;x++)jump.add(new Option(`区域 ${x+1}-${y+1}`,`${x},${y}`));}
 const signature=JSON.stringify([regions,profiles,matrix,selectedRegion]);if(area.dataset.signature===signature)return;area.dataset.signature=signature;area.replaceChildren();
 for(const r of regions){let [x,y,pop,energy,maxGen,food,detritus]=r;let q=profiles[y*n+x]||{},role=roleNames[q.dominantRole]||'混合策略';
   const b=document.createElement('button');b.type='button';b.className='regionCard';b.classList.toggle('selected',selectedRegion===y*n+x);
   const tag=document.createElement('strong');tag.textContent=`REGION ${x+1} — ${y+1}`;const number=document.createElement('b');number.textContent=fmt(pop)+' 个体';const stat=document.createElement('small');stat.textContent=`最高代数 ${fmt(maxGen)} · 营养 ${fmt(food)} · 残骸 ${fmt(detritus)}`;
   const ecology=document.createElement('small');ecology.textContent=`主导功能 ${role} · 初始祖先系 ${fmt(q.founderLineages||0)}`;
   const anatomy=document.createElement('small');anatomy.className='regionalAnatomy';anatomy.textContent=`形态 ${fmt(q.bodyArchitectures||0)} 种 · 均身 ${Number(q.meanBodyNodes||0).toFixed(1)} 节 · ≥10 节 ${fmt(q.largeBodyCarriers||0)}`;
   const founder=document.createElement('small');founder.textContent=`主导创始祖先 #${q.dominantAncestor||0} · 占比 ${(100*Number(q.ancestorShare||0)).toFixed(0)}%`;
   const climate=document.createElement('span');climate.className='regionalFactor';climate.textContent=`再生 ×${Number(q.resourceFactor||1).toFixed(2)} · 温差 ${Number(q.thermalOffset||0)>0?'+':''}${Number(q.thermalOffset||0).toFixed(2)}`;
   const traffic=document.createElement('small');traffic.textContent=`迁入 ${fmt(q.immigration||0)} · 迁出 ${fmt(q.emigration||0)}`;
   b.append(tag,number,stat,ecology,anatomy,founder,climate,traffic);
   b.onclick=()=>{const z=y*n+x;selectedRegion=selectedRegion===z?-1:z;cameraX=(x+.5)*1300;cameraY=(y+.5)*850;zoom=n>1?2.4:2.2;$('#viewScale').value='2.2';jump.value=`${x},${y}`;focusId=null;selectedEvent=null;updateRegions(s);drawWorld();inspectRegion(x,y);};area.append(b);
 }
 const journal=$('#regionalJournal');if(journal){journal.replaceChildren();const events=(s.regionCrossings||[]).filter(e=>selectedRegion<0||e[3]===selectedRegion||e[4]===selectedRegion).slice(-8).reverse();
  for(const e of events){const label=document.createElement('span');label.className='regionalJournalItem';label.textContent=`T ${fmt(e[0])} · 生命 #${e[1]} · 祖先 #${e[2]} · 区域 ${e[3]%n+1}-${Math.floor(e[3]/n)+1} → ${e[4]%n+1}-${Math.floor(e[4]/n)+1} · 第 ${e[5]} 代`;journal.append(label);}
  if(!events.length){const empty=document.createElement('small');empty.textContent='当前尚无区域间迁移事件。';journal.append(empty);}
 }
 const flow=$('#regionalFlow');if(flow){flow.replaceChildren();let directed=[];const zones=n*n;
  for(let a=0;a<zones;a++)for(let b=0;b<zones;b++)if(a!==b&&(selectedRegion<0||a===selectedRegion||b===selectedRegion)){let count=Number(matrix[a*zones+b]||0);if(count>0)directed.push({a,b,count});}
  directed.sort((a,b)=>b.count-a.count);let max=directed[0]?.count||1;
  for(const e of directed.slice(0,10)){const div=document.createElement('div');div.className='regionalFlowItem';const label=document.createElement('small');label.textContent=`${e.a%n+1}-${Math.floor(e.a/n)+1} → ${e.b%n+1}-${Math.floor(e.b/n)+1}`;const track=document.createElement('span');track.className='regionalFlowTrack';const fill=document.createElement('i');fill.style.width=Math.max(3,e.count/max*100)+'%';track.append(fill);const count=document.createElement('b');count.textContent=fmt(e.count);div.append(label,track,count);flow.append(div);}
  if(!directed.length){const empty=document.createElement('small');empty.textContent='当前没有可观察的跨区迁移。';flow.append(empty);}
 }
}
function drawRegionalMorphTimeline(current){
 const canvas=$('#regionalMorphTimeline');if(!canvas)return;
 const ctx=canvas.getContext('2d'),w=canvas.width,h=canvas.height,n=Number(current.worldScale||1),zones=n*n;
 ctx.clearRect(0,0,w,h);ctx.fillStyle='#0c211f';ctx.fillRect(0,0,w,h);
 const series=samples.filter(s=>s.worldScale===n&&Array.isArray(s.regionProfiles)&&s.regionProfiles.length===zones).slice(-110);
 if(!series.some(s=>s.tick===current.tick))series.push(current);
 if(series.length<2){text('regionalMorphCaption','需要至少两个真实采样点才能比较区域结构变化');return;}
 let max=Math.max(3,...series.flatMap(s=>s.regionProfiles.map(z=>Number(z.meanBodyNodes)||0)))*1.08;
 ctx.strokeStyle='rgba(158,208,178,.14)';ctx.lineWidth=1;
 for(let j=0;j<=4;j++){let y=14+(h-35)*j/4;ctx.beginPath();ctx.moveTo(32,y);ctx.lineTo(w-14,y);ctx.stroke();}
 const palette=['#6be0c4','#e7c17d','#b1a6ef','#95d58f','#b7cdea','#f69e94','#d9e19c','#9dd9f0','#d2abd9'];
 for(let z=0;z<zones;z++){ctx.beginPath();let has=false;
  for(let i=0;i<series.length;i++){let v=Number(series[i].regionProfiles[z]?.meanBodyNodes)||0,x=32+(w-48)*i/(series.length-1),y=h-21-(h-35)*v/max;
   if(!has){ctx.moveTo(x,y);has=true;}else ctx.lineTo(x,y);}
  ctx.strokeStyle=palette[z%palette.length];ctx.lineWidth=selectedRegion<0||selectedRegion===z?2.4:.65;ctx.globalAlpha=selectedRegion<0||selectedRegion===z?1:.23;ctx.stroke();ctx.globalAlpha=1;
  let last=Number(series.at(-1).regionProfiles[z]?.meanBodyNodes)||0;
  ctx.fillStyle=palette[z%palette.length];ctx.font='12px sans-serif';ctx.fillText(String(z+1),w-29,Math.max(14,Math.min(h-22,h-21-(h-35)*last/max)));
 }
 ctx.fillStyle='#b8d9c8';ctx.font='11px sans-serif';ctx.fillText(`0–${max.toFixed(1)} 节`,8,13);
 text('regionalMorphCaption',`T ${fmt(series[0].tick)} → ${fmt(series.at(-1).tick)} · ${zones} 区 · ${series.length} 个真实采样状态 · 不预测缺失的中间数据`);
}
$('#regionJump').onchange=e=>{if(e.target.value==='overview'){cameraX=worldWidth/2;cameraY=worldHeight/2;zoom=1;$('#viewScale').value='1';return;}
 let [x,y]=e.target.value.split(',').map(Number);cameraX=(x+.5)*1300;cameraY=(y+.5)*850;zoom=2.2;$('#viewScale').value='2.2';};
function drawDeepTime(s){
 const canvas=document.getElementById('deepTimeChart');if(!canvas)return;
 const cx=canvas.getContext('2d'),w=canvas.width,h=canvas.height;
 cx.clearRect(0,0,w,h);cx.fillStyle='rgba(8,27,22,.44)';cx.fillRect(0,0,w,h);
 const data=samples.filter(q=>Number.isFinite(q.tick)&&Number.isFinite(q.meanNodes)&&Number.isFinite(q.livingClasses)&&Number.isFinite(q.maxGeneration)).slice(-160);
 for(let j=0;j<4;j++){let y=20+(h-40)*j/3;cx.strokeStyle='rgba(173,211,181,.15)';cx.beginPath();cx.moveTo(32,y);cx.lineTo(w-16,y);cx.stroke();}
 const lines=[['meanNodes','#a9ebbc'],['livingClasses','#f0bf72'],['maxGeneration','#90c6ed']];
 for(const [key,color] of lines){let lo=Math.min(...data.map(d=>d[key])),hi=Math.max(...data.map(d=>d[key]));if(!data.length||!Number.isFinite(lo))continue;
 cx.strokeStyle=color;cx.lineWidth=2.5;cx.beginPath();data.forEach((d,k)=>{let x=32+(w-48)*k/Math.max(1,data.length-1),y=h-20-(h-40)*((d[key]-lo)/Math.max(hi-lo,1e-9));if(k===0)cx.moveTo(x,y);else cx.lineTo(x,y);});cx.stroke();}
 text('deepTimeLabel',`${data.length} 帧 · T ${fmt(data[0]?.tick||0)} — ${fmt(data[data.length-1]?.tick||0)}`);
 text('deepNodes',fmt(s.meanNodes));text('deepTypes',fmt(s.livingClasses));text('deepGeneration',fmt(s.maxGeneration));
 text('computeMode',(s.compute===1?'精确优化':'旧版基线')+(s.lifeParallelActive?' · 并行身体准备已执行':s.lifeParallel===1?' · 根据负载自动串行':' · 原序列生命计算'));text('reuseCount',fmt(s.reusedOscillations||0));text('emptyEdges',fmt(s.zeroTransportEdges||0));
}
function render(s,seek=false){drawDeepTime(s);
 const v106=$('#v106Evidence');if(v106&&s.graphPool!==undefined){const n=Number(s.allocatedStateFloats||0),a=Number(s.activeStateFloats||0);v106.textContent=`紧凑状态池：${s.graphPool?'开启':'旧定长'} · 状态槽位 ${fmt(n)} / 活跃需求 ${fmt(a)} · 碰撞缓存 ${fmt(s.contactCacheSlots||0)} 槽 · 已记录交互 ${fmt(s.eventLedgerCount||0)} · ${s.historyLedger?'完整物理账本':'账本关闭'}`;}
 const v104=$('#v104Evidence');if(v104){v104.textContent=`真实模型 · 最大身体 ${fmt(s.maxBodyNodes||0)} / ${fmt(s.bodyCap||40)} 节点 · 神经容量 ${fmt(s.neuronCap||32)} · 连接上限 ${fmt(s.edgeCap||128)} · ${s.sparseBuckets===0?"稳定基线":s.sparseBuckets===1?"哈希稀疏桶":"连续稀疏候选"} · 唯一候选过滤 ${fmt((s.dedupeAvoided||0)+(s.tileDuplicatePrunes||0))} 次 · ${s.quietFluxTolerance>0?'局部低通量近似：省略 '+fmt(s.skippedQuietEdges||0)+' 边，累计省略残骸 '+fmt(s.omittedDetritusFlux||0)+' / 信号 '+fmt(s.omittedSignalFlux||0)+'（不代表长期误差上界）':'完整精确生态输运'} · 生命逐 tick 更新。`;}
let newScale=Number(s.worldScale||1);if(newScale!==worldScale){worldScale=newScale;cameraX=1300*newScale/2;cameraY=850*newScale/2;focusId=null;zoom=1;$('#viewScale').value='1';}
 worldWidth=Number(s.worldWidth||1300);worldHeight=Number(s.worldHeight||850);
 if(newScale!==1&&cameraX===650&&cameraY===425){cameraX=worldWidth/2;cameraY=worldHeight/2;}
 updateRegions(s);
 drawRegionalMorphTimeline(s);
 if(seek||latest&&s.tick<latest.tick){previous=null;}else if(latest&&s.tick!==latest.tick){previous=latest;}let oldState=latest;latest=s;observeSampledLineage(s);
 if(!seek&&historicTick===null&&s.tick!==lastRenderTick){recordMoments(s,oldState);lastRenderTick=s.tick;}
 const now=performance.now();if(!seek&&lastFrameArrived)frameInterval=Math.max(80,Math.min(1050,now-lastFrameArrived));if(!seek)lastFrameArrived=now;
 arrival=now;
 for(let [id,key] of Object.entries(labels))text(id,fmt(s[key]));text('pulseSubtree',fmt(s.subtreeCopies||0));text('pulseReparent',fmt(s.reparentedBranches||0));text('pulseFeedback',fmt(s.feedbackMutations||0));text('pulseCarriers',fmt(s.feedbackCarriers||0));text('range','T '+fmt(s.tick));text('grandHarvests',fmt(s.extendedHarvests||0));text('grandEnergy',fmt(s.extendedEnergy||0));text('transportedDetritus',fmt(s.transportedDetritus||0));text('gradientCarriers',fmt(s.gradientCarriers||0));renderEcotypes(s);text('fieldGen','G '+fmt(s.maxGeneration));text('fieldPop',fmt(s.population)+' 活跃生命');text('fieldTempo','T '+fmt(s.tick)+' · 模型计算时间');lifeDossier(s);text('frameHint',`T ${fmt(s.tick)} · ${fmt(s.individuals?.length||0)} 样本`);
 text('proofText',`第 ${fmt(s.maxGeneration)} 代 · ${fmt(s.births)} 次出生 · ${fmt(s.structuralMutations)} 次结构变异 · ${fmt(s.livingClasses)} 个存活结构签名。当前平均温度偏差 ${fmt(s.meanThermalMismatch)}。目前有 ${fmt(s.activeBonds||0)} 条活动连接和 ${fmt(s.connectedGroups||0)} 个候选联合体；联合后代事件 ${fmt(s.propagulesBorn||0)} 次，已有 ${fmt(s.propagulesMatured||0)} 对持续到 50 tick，最高组织谱系第 ${fmt(s.maxOrganizationGeneration||0)} 代，其中二代以上繁殖 ${fmt(s.descendantOrganizationBirths||0)} 次。累计协同觅食 ${fmt(s.integratedEvents||0)} 次、已解体且有后代的组织 ${fmt(s.productiveOrganizations||0)} 个；这些不是已证实的多细胞生命。身体子树复制 ${fmt(s.subtreeCopies||0)} 次、分支重连 ${fmt(s.reparentedBranches||0)} 次、递归神经变异 ${fmt(s.feedbackMutations||0)} 次，均来自可遗传的结构变化。形态新颖性不等于适应性创新或更高级物种。`);
 if(s.tick!==seenTick){seenTick=s.tick;eventLog();}drawChart();}
async function refresh(){try{let r=await fetch('/api/native/status',{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);let a=await r.json();samples=a.samples||[];drawEcotypeTimeline();serverPhase=a.phase;serverPaused=!!a.paused;
 liveFrame=a.last||null;historyFrames=a.frames||[];
 const busy=a.phase==='running';const rate=a.rate??1;
 selectedRate=rate;
 speedButtons.forEach(b=>{b.classList.toggle('active',Number(b.dataset.rate)===rate);});
 text('runtimeLabel',busy?(serverPaused?'已暂停 · 状态可单步':'正在演化 · '+(rate===0?'不限制执行速度':rate+' tick/s 上限')):'等待 / 已结束');
 text('throughput',fmt(a.actualTicksPerSecond||0)+' tick/s · 实测累计速率');
 text('runPercent',(Math.max(0,Math.min(1,a.progress||0))*100).toFixed(1)+'%');
 $('#progressFill').style.width=(Math.max(0,Math.min(1,a.progress||0))*100)+'%';
 for(let id of ['pauseEngine','stepEngine','checkpointNow'])$('#'+id).disabled=!busy;
 $('#pauseEngine').textContent=serverPaused?'继续':'暂停';
 $('#historyScrub').disabled=!historyFrames.length;
 $('#historyScrub').max=Math.max(0,historyFrames.length-1);
 if(historicTick===null){$('#historyScrub').value=Math.max(0,historyFrames.length-1);text('historyTime',historyFrames.length?'T '+fmt(historyFrames.at(-1).tick)+' · 最新':'等待采样');}
 else {let index=historyFrames.findIndex(f=>f.tick===historicTick);if(index<0){index=0;historicTick=historyFrames[0]?.tick??null;}$('#historyScrub').value=index;text('historyTime','T '+fmt(historicTick)+' · 历史');}
 $('#backLive').disabled=historicTick===null;
 text('status',({idle:'等待启动',running:'原生演化计算中',completed:'实验已完成',stopped:'已停止',stopping:'正在安全保存检查点',error:'计算出错'})[a.phase]||a.phase);
 $('#statusDot').style.background=a.phase==='running'?'#b5e1ad':'#789987';$('#start').disabled=a.phase==='running'||a.phase==='stopping';$('#resume').disabled=!a.checkpointAvailable||a.phase==='running'||a.phase==='stopping';
 if(historicTick===null&&a.last&&(!latest||a.last.tick!==latest.tick)){render(a.last);}else if(!latest){drawWorld();drawChart();}if(a.error)text('proofText',a.error);
 text('checkpointState',a.checkpointAvailable?'检查点已确认 · T '+fmt(a.lastCheckpointTick??0):'尚无已确认的检查点 · 可请求保存');
 }catch(e){text('status','连接失败 · 请从 Node.js 启动项目');text('proofText','原生 C++ 内核必须由项目服务端运行。执行 bash native/build.sh 然后 node server.js。');}}
function send(resume=false){let data=Object.fromEntries(new FormData($('#config')));for(let k of ['seed','initial','capacity','ticks','mutation','resourceRate','season','climate','predation','sharing','cohesion','specialization','cooperation','integration','scavenging','plasticity','ecology','development','morphogenesis','softBody','collisionIndex','allContacts','regulatoryNetwork','bodyCap','neuronCap','edgeCap','founderNeurons','sparseBuckets','graphPool','contactCache','historyLedger','founderNodes','contactDedupe','quietActivityLimit','quietFluxTolerance','tissueCoupling','metabolicEcology','grand','habitats','transport','regionality','worldScale','scheduler','compute','lifeParallel','threads','sample'])data[k]=Number(data[k]);data.rate=resume?selectedRate:Number($('#startRate').value);data.resume=resume;data.visual=120;data.checkpointEvery=1000;
 return fetch('/api/native/start',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)}).then(async r=>{let a=await r.json();if(!r.ok)throw Error(a.error||r.status);if(!resume){sampledAncestry.clear();samples=[];latest=null;previous=null;focusId=null;selectedEvent=null;historicTick=null;historyFrames=[];organismMoments=[];selectedEcotype=-1;lastRenderTick=-1;stopHistoryPlayback();$('#evolutionMoments').replaceChildren();lifeDossier(null);}return refresh();}).catch(e=>{text('status','实验启动失败');text('proofText',e.message);});}
$('#config').addEventListener('submit',e=>{e.preventDefault();send(false);});$('#resume').onclick=()=>send(true);
$('#stop').onclick=async()=>{await fetch('/api/native/stop',{method:'POST'});await refresh();};
$('#csv').onclick=()=>{if(!samples.length)return;let keys=['tick','population','maxGeneration','livingClasses','heritageClasses','meanNodes','meanNeurons','meanEnergy','meanThermalMismatch','births','deaths','exchanges','signals','predations','shares','activeBonds','connectedGroups','largestGroup','bondsFormed','bondsBroken','structuralMutations','complementaryPairs','propagulesBorn','propagulesMatured','organizationLineages','maxOrganizationGeneration','descendantOrganizationBirths','cooperativeDefenses','preventedLoss','integratedEvents','integratedHarvest','integratedCost','productiveOrganizations','scavenges','scavengedEnergy','subtreeCopies','reparentedBranches','feedbackMutations','soilEdits','soilMineralizations','nicheHarvests','developmentMutations','recycledMatter','ecologicalHarvest','nicheSensorCarriers','habitatOrganCarriers','meanNiche','metabolicEcology','cargoFoodUptake','cargoDetritusUptake','cargoFoodProcessed','cargoDetritusProcessed','metabolicHeat','habitats','transport','transportEvents','transportedDetritus','regionalMigrants','crossRegionMatter','regionality','worldScale','scheduler','compute','reusedOscillations','zeroTransportEdges','regionJobs','bodyArchitectures','largeBodyCarriers'];let csv=[keys.join(','),...samples.map(s=>keys.map(k=>s[k]??'').join(','))].join('\n');let a=document.createElement('a'),u=URL.createObjectURL(new Blob([csv],{type:'text/csv;charset=utf-8'}));a.href=u;a.download='EvoLab_v10_7_living_systems.csv';a.click();setTimeout(()=>URL.revokeObjectURL(u),3000);};
for(let [name,id] of [['mutation','mutationNumber'],['resourceRate','resourceNumber'],['season','seasonNumber'],['climate','climateNumber'],['predation','predationNumber'],['sharing','sharingNumber'],['cohesion','cohesionNumber'],['specialization','specializationNumber'],['cooperation','cooperationNumber'],['integration','integrationNumber'],['scavenging','scavengingNumber'],['plasticity','plasticityNumber'],['ecology','ecologyNumber'],['development','developmentNumber'],['morphogenesis','morphogenesisNumber'],['softBody','softBodyNumber'],['tissueCoupling','tissueCouplingNumber'],['metabolicEcology','metabolicEcologyNumber'],['regulatoryNetwork','regulatoryNetworkNumber'],['grand','grandNumber'],['habitats','habitatsNumber'],['transport','transportNumber'],['regionality','regionalityNumber']])$('#config [name='+name+']').oninput=e=>text(id,Number(e.target.value).toFixed(2));
$('#relationHold').oninput=e=>{hold=Number(e.target.value);text('holdNumber',hold+' tick');};
$('#relationFilter').onchange=e=>{relationFilter=e.target.value;selectedEvent=null;};
$('#viewScale').onchange=e=>{const z=Number(e.target.value);if(Number.isFinite(z)&&z>=1)zoom=z;};$('#ecologyLayer').onchange=e=>{ecologyLayer=e.target.value;};$('#immersiveMode').onclick=e=>{const enabled=document.querySelector('.layout').classList.toggle('immersive');e.currentTarget.textContent=enabled?'显示参数':'专注观察';e.currentTarget.setAttribute('aria-pressed',String(enabled));};$('#unfocusLife').onclick=()=>{focusId=null;selectedEvent=null;lifeDossier(latest);};
$('#followInteractions').onclick=()=>{if(!latest?.individuals?.length){text('selectedRelation','尚无可以定位的生命。');return;}
 const individuals=new Map(latest.individuals.map(a=>[a[0],a]));let center=null;
 for(const b of (latest.bonds||[])){const a=individuals.get(b[0]),c=individuals.get(b[1]);if(a&&c){center=[(a[1]+c[1])/2,(a[2]+c[2])/2];break;}}
 if(!center)for(const e of (latest.relationships||[])){const a=individuals.get(e[1]),c=individuals.get(e[2]);if(a&&c){center=[(a[1]+c[1])/2,(a[2]+c[2])/2];break;}}
 if(!center){const a=latest.individuals[0];center=[a[1],a[2]];}
 cameraX=center[0];cameraY=center[1];focusId=null;selectedEvent=null;zoom=3.5;$('#viewScale').value='3.5';text('selectedRelation','已移动至当前可观察的互动热点，关系位置由存活个体实时决定。');drawWorld();};
$('#followLargest').onclick=()=>{
 const all=latest?.individuals||[];
 if(!all.length){text('selectedRelation','尚未采样任何生命。');return;}
 const organism=all.reduce((leader,item)=>item[6]>leader[6]?item:leader,all[0]);
 focusId=organism[0];selectedEvent=null;cameraX=organism[1];cameraY=organism[2];zoom=8;
 $('#viewScale').value='8';lifeDossier(latest);
 text('selectedRelation',`正在观察现存最大身体样本 #${organism[0]}，${organism[6]} 个遗传身体节点，第 ${organism[5]} 代。它不是预设物种。`);
 drawWorld();
};
$('#followInheritance').onclick=()=>{if(!latest?.bonds?.length){text('selectedRelation','目前无活动连接，无法追踪。');return;}
 const present=new Map((latest.individuals||[]).map(i=>[i[0],i]));
 let b=latest.bonds.find(z=>z[4]&&present.has(z[0])&&present.has(z[1]))||latest.bonds.find(z=>present.has(z[0])&&present.has(z[1]));
 if(!b){text('selectedRelation','活动连接尚未进入本次可见抽样，尝试增加抽样规模。');return;}
 let a=present.get(b[0]),c=present.get(b[1]);focusId=b[0];selectedEvent=null;zoom=3.5;$('#viewScale').value='3.5';cameraX=(a[1]+c[1])/2;cameraY=(a[2]+c[2])/2;
 text('selectedRelation',b[4]?`追踪组织 #${b[5]||'?'} · 第 ${b[7]||0} 代 · #${b[0]} 和 #${b[1]} · 连接年龄 ${b[2]} tick`:`正在追踪连接 #${b[0]} 和 #${b[1]} · 连接年龄 ${b[2]} tick`);
};
$('#overview').onclick=()=>{cameraX=worldWidth/2;cameraY=worldHeight/2;zoom=1;$('#viewScale').value='1';focusId=null;selectedEvent=null;};
$('#clearSelection').onclick=()=>{focusId=null;selectedEvent=null;lifeDossier(latest);text('selectedRelation','点击事件定位交互双方；虚线为历史关系，非持续接触。');};
world.addEventListener('click',e=>{if(dragged){dragged=false;return;}if(!latest)return;let b=world.getBoundingClientRect(),px=(e.clientX-b.left)/b.width*W,py=(e.clientY-b.top)/b.height*H,m=coords(),best=24,id=null;for(let [uid,d] of m){let dis=Math.hypot(d.x-px,d.y-py);if(dis<best){best=dis;id=uid;}}if(id){focusId=id;selectedEvent=null;let selected=m.get(id);if(selected){cameraX=selected.worldX;cameraY=selected.worldY;}lifeDossier(latest);text('selectedRelation',`已选中生命 #${id} · 查看其身体与最近关系`);}});
async function sendControl(action,rate){try{let r=await fetch('/api/native/control',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({action,rate})});let j=await r.json();if(!r.ok)throw Error(j.error||'控制失败');await refresh();}catch(e){text('status',e.message);}}
for(const b of speedButtons){b.onclick=()=>{const r=Number(b.dataset.rate);selectedRate=r;$('#startRate').value=String(r);speedButtons.forEach(x=>x.classList.toggle('active',x===b));if(serverPhase==='running')sendControl('rate',r);}};
$('#pauseEngine').onclick=()=>sendControl(serverPaused?'resume':'pause');
$('#stepEngine').onclick=()=>sendControl('step');
$('#checkpointNow').onclick=()=>sendControl('checkpoint');
let historyRequest=0;
$('#historyScrub').oninput=async e=>{stopHistoryPlayback();const f=historyFrames[Number(e.target.value)];if(!f)return;const nonce=++historyRequest;historicTick=f.tick;previous=null;
 try{const res=await fetch('/api/native/frame?tick='+encodeURIComponent(f.tick),{cache:'no-store'});if(!res.ok)throw Error('快照已过期');const full=await res.json();if(nonce!==historyRequest)return;render(full,true);$('#backLive').disabled=false;text('historyTime','T '+fmt(f.tick)+' · 历史');}
 catch(err){text('historyTime','该历史快照已被覆盖');}
};
let historyPlayback=null,playbackBusy=false;
function stopHistoryPlayback(){if(historyPlayback){clearInterval(historyPlayback);historyPlayback=null;}$('#playHistory').textContent='▶ 播放记录';}
$('#playHistory').onclick=()=>{
 if(historyPlayback){stopHistoryPlayback();return;}
 if(historyFrames.length<2){text('historyTime','至少需要两帧真实采样才能回放');return;}
 let k=Math.max(0,historyFrames.findIndex(f=>f.tick===historicTick));
 $('#playHistory').textContent='■ 停止播放';
 historyPlayback=setInterval(async()=>{
  if(playbackBusy)return;
  if(k>=historyFrames.length){stopHistoryPlayback();return;}
  const f=historyFrames[k++];playbackBusy=true;
  try{const r=await fetch('/api/native/frame?tick='+encodeURIComponent(f.tick),{cache:'no-store'});if(!r.ok)throw Error('历史帧已过期');let full=await r.json();historicTick=f.tick;previous=null;render(full,true);$('#historyScrub').value=Math.max(0,historyFrames.findIndex(z=>z.tick===f.tick));text('historyTime','T '+fmt(f.tick)+' · 历史播放');$('#backLive').disabled=false;}
  catch(e){stopHistoryPlayback();text('historyTime',e.message);}
  finally{playbackBusy=false;}
 },420);
};
$('#backLive').onclick=()=>{stopHistoryPlayback();historicTick=null;previous=null;if(liveFrame)render(liveFrame,true);$('#backLive').disabled=true;text('historyTime','T '+fmt(liveFrame?.tick||0)+' · 最新');};
// Camera gestures affect observation only; the ecological state never changes.
// v10.9: multiscale observation transforms the current C++ coordinates only.
// Single finger pans; two fingers pinch without altering any life/world state.
world.style.touchAction='none';
const pointers=new Map();let pinchStart=null;
function cameraClamp(){cameraX=Math.max(0,Math.min(worldWidth,cameraX));cameraY=Math.max(0,Math.min(worldHeight,cameraY));}
function canvasXY(x,y){const b=world.getBoundingClientRect();return {x:(x-b.left)/b.width*W,y:(y-b.top)/b.height*H};}
function changeZoom(factor,px=W/2,py=H/2){
 const old=zoom;zoom=Math.max(1,Math.min(256,zoom*factor));
 cameraX+=(px-W/2)*worldWidth/W*(1/old-1/zoom);
 cameraY+=(py-H/2)*worldHeight/H*(1/old-1/zoom);cameraClamp();
 const select=$('#viewScale');const opt=[...select.options].find(x=>Math.abs(Number(x.value)-zoom)<.002);
 if(opt)select.value=opt.value;else{
  let custom=select.querySelector('option[value="custom"]');if(!custom){custom=document.createElement('option');custom.value='custom';select.add(custom);}
  custom.textContent='自由缩放 ×'+zoom.toFixed(1);select.value='custom';
 }
}
function distance(a,b){return Math.hypot(a.x-b.x,a.y-b.y);}
world.addEventListener('pointerdown',e=>{
 pointers.set(e.pointerId,{x:e.clientX,y:e.clientY});
 world.setPointerCapture(e.pointerId);dragged=false;
 if(pointers.size===1){pointerStart={x:e.clientX,y:e.clientY,cx:cameraX,cy:cameraY};pinchStart=null;}
 else if(pointers.size===2){const [a,b]=[...pointers.values()];pinchStart={distance:Math.max(1,distance(a,b))};pointerStart=null;dragged=true;}
});
world.addEventListener('pointermove',e=>{
 if(!pointers.has(e.pointerId))return;
 pointers.set(e.pointerId,{x:e.clientX,y:e.clientY});
 if(pointers.size===2){const [a,b]=[...pointers.values()];const now=distance(a,b);
  if(pinchStart){const m=canvasXY((a.x+b.x)*.5,(a.y+b.y)*.5);changeZoom(now/pinchStart.distance,m.x,m.y);pinchStart.distance=now;}
  dragged=true;return;
 }
 if(!pointerStart)return;const dx=e.clientX-pointerStart.x,dy=e.clientY-pointerStart.y;
 if(Math.hypot(dx,dy)>7)dragged=true;
 if(dragged){const b=world.getBoundingClientRect();cameraX=pointerStart.cx-dx/b.width*worldWidth/zoom;cameraY=pointerStart.cy-dy/b.height*worldHeight/zoom;cameraClamp();focusId=null;selectedEvent=null;}
});
function finishPointer(e){pointers.delete(e.pointerId);pinchStart=null;pointerStart=null;
 if(pointers.size===1){const a=[...pointers.values()][0];pointerStart={x:a.x,y:a.y,cx:cameraX,cy:cameraY};}}
world.addEventListener('pointerup',finishPointer);world.addEventListener('pointercancel',finishPointer);
world.addEventListener('wheel',e=>{e.preventDefault();const point=canvasXY(e.clientX,e.clientY);changeZoom(e.deltaY>0?.90:1.10,point.x,point.y);},{passive:false});

let lastDraw=0;function animate(t){if(t-lastDraw>25){
 if(focusId&&historicTick===null&&latest?.individuals){const a=latest.individuals.find(i=>i[0]===focusId),b=selectedEvent?latest.individuals.find(i=>i[0]===selectedEvent[2]):null;
 if(a){const tx=b?(a[1]+b[1])*.5:a[1],ty=b?(a[2]+b[2])*.5:a[2];cameraX=lerp(cameraX,tx,.10);cameraY=lerp(cameraY,ty,.10);}}
 drawWorld();lastDraw=t;}requestAnimationFrame(animate);}requestAnimationFrame(animate);
setInterval(refresh,850);refresh();
