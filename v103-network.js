'use strict';
/* v10.3 observational layer: read-only tissue topology; no independent simulation. */
(()=>{
const host=document.querySelector('.v98-right')||document.querySelector('.stage');
if(!host)return;
const panel=document.createElement('details');panel.className='v103-network';panel.open=false;
panel.innerHTML='<summary>遗传组织调控网络 <small>GENE-ENCODED LINKS / REAL DATA</small></summary><canvas id="v103Genes" width="680" height="280" aria-label="来自实际遗传节点的父子连接与调控信号"></canvas><div id="v103GeneInfo">选择实际生命后查看</div>';
host.append(panel);
const cn=panel.querySelector('canvas'),info=panel.querySelector('#v103GeneInfo'),ctx=cn.getContext('2d');
let last='';
function draw(){
 const state=typeof latest!=='undefined'?latest:null;
 if(!state||!panel.open)return;
 const sampled=state.individuals||[],uid=typeof focusId!=='undefined'?focusId:null;
 const creature=sampled.find(a=>a[0]===uid)||null;
 const key=creature?`${state.tick}:${creature[0]}:${state.regulatoryNetwork}`:String(state.tick);
 if(key===last)return;last=key;
 const w=cn.width,h=cn.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#081b1b';ctx.fillRect(0,0,w,h);
 if(!creature){info.textContent='未选择活体，或目标不在当前可视采样中';return;}
 const parts=creature[11]||[];if(!parts.length){info.textContent='身体发育节点数据缺失';return;}
 const canUse=Number(state.regulatoryNetwork)>0;
 const graph=[];const root={x:0,y:0,theta:0};graph.push(root);
 for(let i=1;i<parts.length;i++){const p=parts[i],par=graph[p[0]]||root,theta=par.theta+(Number(p[3])||0)+(Number(p[5])||0);graph.push({x:par.x+Math.cos(theta)*Number(p[2]||0),y:par.y+Math.sin(theta)*Number(p[2]||0),theta});}
 const xs=graph.map(x=>x.x),ys=graph.map(x=>x.y),minx=Math.min(...xs),maxx=Math.max(...xs),miny=Math.min(...ys),maxy=Math.max(...ys),zoom=Math.min((w-100)/Math.max(20,maxx-minx),(h-70)/Math.max(20,maxy-miny));
 const loc=z=>({x:w/2+(z.x-(maxx+minx)/2)*zoom,y:h/2+(z.y-(maxy+miny)/2)*zoom});
 ctx.lineCap='round';for(let i=1;i<parts.length;i++){let p=parts[i],a=loc(graph[i]),b=loc(graph[p[0]]||root);ctx.strokeStyle='#67c9ae';ctx.lineWidth=2.4;ctx.beginPath();ctx.moveTo(a.x,a.y);ctx.lineTo(b.x,b.y);ctx.stroke();}
 let links=0;
 if(canUse){ctx.setLineDash([5,4]);for(let i=1;i<parts.length;i++){
  let node=parts[i];if(node.length<12)continue;
  let source=Math.max(0,Math.min(i-1,Math.floor(Number(node[node.length-2])||0))),weight=Number(node[node.length-1])||0;
  if(Math.abs(weight)<.02)continue;
  let a=loc(graph[i]),b=loc(graph[source]);ctx.strokeStyle=weight>0?'#d6a9f6':'#e4a581';ctx.lineWidth=1+Math.min(3,Math.abs(weight)*3);ctx.beginPath();ctx.moveTo(a.x,a.y);ctx.quadraticCurveTo((a.x+b.x)/2+13,(a.y+b.y)/2-18,b.x,b.y);ctx.stroke();links++;
 }ctx.setLineDash([]);}
 for(let i=0;i<parts.length;i++){const pt=loc(graph[i]),t=Number(parts[i][1])||0;ctx.fillStyle=['#8de5d1','#98cff5','#efce8e','#b0f0b0','#c8b7fa','#f4b894','#a2dca2','#95e1c2'][t%8];ctx.beginPath();ctx.arc(pt.x,pt.y,i===0?6:4,0,Math.PI*2);ctx.fill();}
 info.textContent=`UID #${creature[0]} · 遗传节点 ${parts.length} · 额外调控连接 ${links} · T ${state.tick}。实线为实际父子结构，紫/橙色虚线为遗传调控强度的符号化表示。`;
}
panel.addEventListener('toggle',()=>{last='';draw();});setInterval(draw,900);
})();
