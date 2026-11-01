'use strict';
/* EvoLab Ω v10.1 — World/Individual viewport with inherited continuous surface; read-only, derived exclusively from C++ state.
   2D topology is not an imagined 3D anatomy. WebGL2 shading is an observation material. */
(() => {
  const world=document.getElementById('world'),host=world?.parentElement;
  if(!host)return;
  const $=id=>document.getElementById(id);
  const btn=(text,id,cl='')=>{const b=document.createElement('button');b.type='button';b.id=id;b.className='v10-control '+cl;b.textContent=text;return b;};
  const stage=document.querySelector('.stage'),bar=document.querySelector('.fieldcontrols');
  const open=btn('进入个体结构','v10Enter','v10-enter');
  const mode=btn('世界俯视','v10World','v10-world-on');
  bar?.prepend(open,mode);
  const deck=document.createElement('section');deck.id='v10Structure';deck.className='v10-structure';deck.hidden=true;
  deck.setAttribute('aria-label','由真实原生身体拓扑生成的个体观察视图');
  deck.innerHTML=`<div class="v10-head"><div class="v10-headline"><span>INDIVIDUAL SPACE / ACTUAL GENOME & DEVELOPMENT</span><strong id="v10Specimen">选择生命</strong><small id="v10Source">原生生命节点数据 · 展示层非物理器官体积</small></div><button id="v10Return" type="button">← 返回世界</button></div>
  <div class="v10-view"><canvas id="v10SpecimenGpu" width="1280" height="740" aria-label="WebGL2 个体身体拓扑"></canvas><canvas id="v10SpecimenFallback" width="1280" height="740" aria-label="Canvas2D 个体身体拓扑"></canvas><div class="v10-guides"><span>2D 连续体表 / INHERITED CAPSULE SURFACE</span><span id="v10GpuStatus">检测渲染能力…</span></div><div class="v10-aim" aria-hidden="true"></div></div>
  <div class="v10-bottom"><div class="v10-modes"><button id="v10Explore" type="button">发现复杂活跃形态</button><button id="v10Shell" class="is-active" type="button">身体结构</button><button id="v10Function" type="button">功能组成</button><button id="v10Control" type="button">控制映射</button><button id="v107Cargo" type="button">内部物质流</button></div><div class="v10-zoom-control"><button id="v10ZoomOut" type="button" aria-label="缩小">−</button><input type="range" id="v10Zoom" min="1" max="128" value="1" aria-label="结构视图数字放大"><button id="v10ZoomIn" type="button" aria-label="放大">+</button><button id="v10Fit" type="button">适应整体</button><strong id="v10ZoomText">×1</strong></div><div class="v10-metrics"><span>UID <b id="v10Uid">—</b></span><span>世代 <b id="v10Gen">—</b></span><span>已表达部件 <b id="v10Developed">—</b></span><span>能量 <b id="v10Energy">—</b></span><span>遗传组织 <b id="v101Tissue">—</b></span><span>组织信号 <b id="v102TissueNetwork">—</b></span><span>采样时刻 <b id="v10Tick">—</b></span></div><p id="v10Meaning">只绘制已编码的身体父子结构与表达状态；颜色体现真实模块类型。不存在的内部器官、神经连接和高维结构不会提前生成。</p></div>`;
  host.append(deck);
  const gCanvas=$('v10SpecimenGpu'),cCanvas=$('v10SpecimenFallback');
  const modeNames=['shell','function','control'],cols=[[.30,.88,.82],[.40,.72,.99],[.90,.69,.40],[.60,.92,.75],[.77,.55,.94],[.98,.64,.40],[.52,.89,.63],[.72,.91,.60]];
  let displayed=false,detail='shell',zoomLevel=1,pan={x:0,y:0},lastUid=null,viewState=null;
  let gl=null,prog=null,vao=null,corner=null,instances=null,uSize=null,glReady=false,loss=false,gpuUnavailable=false,drawCount=0,modelCache=null;
  const metrics=(id,val)=>{const e=$(id);if(e)e.textContent=String(val)};
  const vertex=`#version 300 es
precision highp float;
layout(location=0) in vec2 aCorner;
layout(location=1) in vec2 aStart;
layout(location=2) in vec2 aEnd;
layout(location=3) in float aWidth;
layout(location=4) in vec4 aColor;
layout(location=5) in float aRole;
uniform vec2 uViewport;
out vec2 vLocal;out vec4 vColor;flat out float vLength;flat out float vWidth;flat out float vRole;
void main(){vec2 direction=aEnd-aStart;float len=length(direction);vec2 tangent=len<.0001?vec2(1.,0.):direction/max(len,.0001);vec2 norm=vec2(-tangent.y,tangent.x);
float along=mix(-aWidth,len+aWidth,aCorner.x);float side=aCorner.y*aWidth;
vec2 p=aStart+tangent*along+norm*side;
gl_Position=vec4(2.*p/uViewport-1.,0.,1.);gl_Position.y=-gl_Position.y;
vLocal=vec2(along,side);vColor=aColor;vLength=len;vWidth=aWidth;vRole=aRole;}`;
  const fragment=`#version 300 es
precision highp float;
in vec2 vLocal;in vec4 vColor;flat in float vLength;flat in float vWidth;flat in float vRole;
out vec4 fragColor;
void main(){float radius=max(.01,vWidth);float distanceEdge=length(vec2(vLocal.x-clamp(vLocal.x,0.,vLength),vLocal.y));float aa=max(1.,fwidth(distanceEdge)*1.4);
float mask=1.-smoothstep(radius-aa,radius+aa,distanceEdge);if(mask<.005)discard;
float v=clamp(vLocal.y/radius,-1.,1.);float bulge=sqrt(max(0.,1.-v*v));
float highlight=pow(max(0.,bulge*.84-v*.37),8.)*.46;float edge=max(0.,1.-abs(v));
float membrane=pow(edge,1.4)*.28;float ridge=.035*sin(vLocal.x*.095+v*5.0);
vec3 base=vColor.rgb*(.24+.47*bulge+membrane+ridge)+vec3(.73,1.,.89)*highlight;
float gold=(vRole>3.5&&vRole<4.5)?0.12:0.;base+=vec3(gold,gold*.55,gold*.05);
fragColor=vec4(base,mask*vColor.a);}`;
  function compile(type,src){const shader=gl.createShader(type);gl.shaderSource(shader,src);gl.compileShader(shader);if(!gl.getShaderParameter(shader,gl.COMPILE_STATUS))throw new Error(gl.getShaderInfoLog(shader)||'GLSL 编译失败');return shader;}
  function startGpu(){if(glReady&&!loss)return true;if(gpuUnavailable)return false;
    try{
      gl=gCanvas.getContext('webgl2',{alpha:true,premultipliedAlpha:false,antialias:true,depth:false,preserveDrawingBuffer:false});
      if(!gl)throw new Error('WebGL2 不可用');
      prog=gl.createProgram();const vs=compile(gl.VERTEX_SHADER,vertex),fs=compile(gl.FRAGMENT_SHADER,fragment);
      gl.attachShader(prog,vs);gl.attachShader(prog,fs);gl.linkProgram(prog);if(!gl.getProgramParameter(prog,gl.LINK_STATUS))throw new Error(gl.getProgramInfoLog(prog)||'GLSL 链接失败');
      gl.deleteShader(vs);gl.deleteShader(fs);vao=gl.createVertexArray();gl.bindVertexArray(vao);
      corner=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,corner);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array([0,-1,1,-1,0,1,1,1]),gl.STATIC_DRAW);
      gl.enableVertexAttribArray(0);gl.vertexAttribPointer(0,2,gl.FLOAT,false,8,0);
      instances=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,instances);
      // [x1,y1,x2,y2,r,rgba,role] = ten 32-bit floats / instance.
      for(const [slot,n,offset] of [[1,2,0],[2,2,8],[3,1,16],[4,4,20],[5,1,36]]){gl.enableVertexAttribArray(slot);gl.vertexAttribPointer(slot,n,gl.FLOAT,false,40,offset);gl.vertexAttribDivisor(slot,1);}
      gl.bindVertexArray(null);gl.useProgram(prog);uSize=gl.getUniformLocation(prog,'uViewport');gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);
      glReady=true;loss=false;gCanvas.hidden=false;cCanvas.hidden=true;metrics('v10GpuStatus','WebGL2 · 真实节点实例化绘制');return true;
    }catch(e){glReady=false;gpuUnavailable=true;gCanvas.hidden=true;cCanvas.hidden=false;metrics('v10GpuStatus','Canvas2D · 安全兼容模式');console.warn('[EvoLab v10] GPU inspector fallback:',e.message);return false;}
  }
  gCanvas.addEventListener('webglcontextlost',e=>{e.preventDefault();loss=true;glReady=false;gCanvas.hidden=true;cCanvas.hidden=false;metrics('v10GpuStatus','GPU 上下文丢失 · Canvas 回退');drawSpecimen();},{passive:false});
  gCanvas.addEventListener('webglcontextrestored',()=>{glReady=false;loss=false;gpuUnavailable=false;startGpu();drawSpecimen();});
  function topology(a){if(!Array.isArray(a?.[11]))return [];
    const parent=a[11],lifeAge=Number(a[8])||0,pts=[{x:0,y:0,theta:0,active:true,parent:-1,type:0}],L=parent.length;
    // Recompute 2D body coordinates from the snapshot's actual parent, length and joint angle.
    for(let k=1;k<L;k++){const n=parent[k],pid=Number.isInteger(n[0])&&n[0]>=0&&n[0]<k?n[0]:0,p=pts[pid];const theta=p.theta+(Number(n[3])||0)+(Number(n[5])||0);
      const on=p.active&&(n.length>9?!!n[9]:(lifeAge/210>=(Number(n[4])||0)));
      const len=Number(n[2])||0,realSurface=n.length>=13;
      pts.push({x:p.x+(on?Math.cos(theta)*len*(realSurface?1:.58):0),y:p.y+(on?Math.sin(theta)*len*(realSurface?1:.58):0),theta,active:on,parent:pid,type:Math.max(0,Math.min(7,Math.trunc(n[1]||0))),motor:Number(n[8])||0,control:Number(n[6])||0,sense:Number(n[7])||0,thickness:realSurface?Number(n[10]):null,flexibility:realSurface?Number(n[11]):null,effective:realSurface?Number(n[12]):null,conductance:n.length>=16?Number(n[13]):null,stiffness:n.length>=16?Number(n[14]):null,tissueSignal:n.length>=16?Number(n[15]):null});
    }return pts;
  }
  function current(){return (typeof latest!=='undefined'&&latest?.individuals?.find(a=>a[0]===focusId))||null;}
  function findExpressed(){const list=latest?.individuals||[];if(!list.length)return null;const count=a=>{let v=0;for(const p of a[11]||[])if(p.length>9?p[9]:(a[8]/210>=p[4]))v++;return v;};const best=list.reduce((a,b)=>count(a)>count(b)?a:count(a)===count(b)&&a[6]>=b[6]?a:b);focusId=best[0];cameraX=best[1];cameraY=best[2];if(typeof lifeDossier==='function')lifeDossier(latest);return best;}
  function build(a,w,h){const n=topology(a);if(n.length===0)return {n,shapes:[]};
    const active=n.filter(p=>p.active),xs=active.map(p=>p.x),ys=active.map(p=>p.y);
    const xmin=Math.min(...xs,-4),xmax=Math.max(...xs,4),ymin=Math.min(...ys,-4),ymax=Math.max(...ys,4),spanX=Math.max(15,xmax-xmin),spanY=Math.max(15,ymax-ymin);
    const fit=Math.min(.63*w/spanX,.63*h/spanY),s=fit*zoomLevel,cx=w/2-s*(xmin+xmax)/2+pan.x,cy=h/2-s*(ymin+ymax)/2+pan.y;
    const point=p=>[cx+p.x*s,cy+p.y*s];const rows=[],baseRadius=Math.max(1.5,Math.min(5.8,spanX*.028))*s;
    const add=(x,y,xx,yy,r,c,alpha,role)=>rows.push(x,y,xx,yy,r,c[0],c[1],c[2],alpha,role);
    for(let j=1;j<n.length;j++){
      const p=n[j];if(!p.active)continue;const q=n[p.parent],a1=point(q),a2=point(p),c=cols[p.type];
      const opacity=detail==='function'?.92:detail==='control'?.74:.85;
      // Exact finite cross-section from the C++ deformed-body snapshot when present.
      const width=Number.isFinite(p.effective)?Math.max(.5,p.effective*s):baseRadius*.47;
      add(...a1,...a2,width,c,opacity,p.type);
      add(...a2,...a2,Math.max(2,width*.92),detail==='control'&&p.motor!==0?cols[4]:c,.88,p.type);
      if(detail==='control'&&p.motor!==0){let intensity=Number.isFinite(p.tissueSignal)?Math.max(.16,Math.min(.96,Math.abs(p.tissueSignal)*2.7)):.96;add(...a1,...a2,Math.max(1,baseRadius*.18),cols[4],intensity,4);}
      if(detail==='cargo'&&Array.isArray(a[23])&&Array.isArray(a[24])){
        const food=Number(a[23][j]||0),det=Number(a[24][j]||0);
        // Directed child → parent: only actual substrate transferred on the last simulated tick.
        if(food>1e-6)add(...a2,...a1,Math.max(1.6,Math.min(width*.47,1.9+food*baseRadius*4)),[.31,.98,.67],Math.min(.98,.32+food*6),2);
        if(det>1e-6)add(...a2,...a1,Math.max(1.3,Math.min(width*.32,1.3+det*baseRadius*3)),[1,.68,.33],Math.min(.98,.30+det*6),5);
      }
      if(detail==='function'&&p.type===1){add(...a2,...a2,Math.max(3,baseRadius*1.25),cols[1],.38,p.type);}
    }
    const root=point(n[0]);add(...root,...root,n.some(x=>x.effective!=null)?Math.max(3,3.5*s):Math.max(6,baseRadius*2.3),[.65,.93,.79],.96,0);
    return {n,shapes:rows,active:active.length,span:Math.max(spanX,spanY)};
  }
  function canvasDraw(rows,w,h){const c=cCanvas.getContext('2d');if(!c)return;
    c.clearRect(0,0,w,h);for(let k=0;k<rows.length;k+=10){const [x,y,xx,yy,r,red,green,blue,a]=rows.slice(k,k+9);c.save();c.globalAlpha=a;const dx=xx-x,dy=yy-y,l=Math.hypot(dx,dy),ang=Math.atan2(dy,dx);c.translate(x,y);c.rotate(ang);const color=(f=1)=>`rgba(${Math.min(255,red*255*f)|0},${Math.min(255,green*255*f)|0},${Math.min(255,blue*255*f)|0},1)`;
      c.beginPath();c.moveTo(0,-r);c.arc(l,-0,r,-Math.PI/2,Math.PI/2);c.lineTo(0,r);c.arc(0,0,r,Math.PI/2,3*Math.PI/2);c.closePath();const gradient=c.createLinearGradient(0,-r,0,r);gradient.addColorStop(0,color(1.1));gradient.addColorStop(.3,color(.95));gradient.addColorStop(.52,color(.4));gradient.addColorStop(.8,color(.68));gradient.addColorStop(1,color(.16));c.fillStyle=gradient;c.fill();c.lineWidth=Math.max(.7,r*.12);c.strokeStyle='rgba(207,248,225,.47)';c.stroke();c.restore();}
  }
  function resize(){const box=gCanvas.getBoundingClientRect(),dpr=Math.min(2,window.devicePixelRatio||1);const width=Math.max(200,Math.min(2800,Math.round(box.width*dpr))),height=Math.max(160,Math.min(1700,Math.round(box.height*dpr)));
    if(gCanvas.width!==width||gCanvas.height!==height){gCanvas.width=cCanvas.width=width;gCanvas.height=cCanvas.height=height;}
    return {w:width,h:height};
  }
  function drawSpecimen(){if(!displayed)return;const {w,h}=resize(),spec=current();if(!spec){metrics('v10Specimen','当前生命不在采样中');metrics('v10Source','可能已经死亡或离开采样范围；选择其他生命。');if(glReady)gl.clear(gl.COLOR_BUFFER_BIT);else cCanvas.getContext('2d')?.clearRect(0,0,w,h);return;}
    const data=build(spec,w,h);modelCache=data;
    metrics('v10Specimen',`生命 #${spec[0]} · 第 ${spec[5]} 代`);metrics('v10Uid',spec[0]);metrics('v10Gen',spec[5]);metrics('v10Energy',Number(spec[4]).toFixed(2));metrics('v10Developed',`${data.active}/${data.n.length}`);metrics('v10Tick',latest.tick);
    const actualSurface=spec[11]?.some(n=>n.length>=13);
    metrics('v101Tissue',actualSurface?'连续表面已计算':'旧离散节点');
    const networkNodes=data.n.filter(n=>Number.isFinite(n.tissueSignal));metrics('v102TissueNetwork',networkNodes.length?`${networkNodes.length} 通道 · 平均 ${((networkNodes.reduce((q,n)=>q+Math.abs(n.tissueSignal),0))/networkNodes.length).toFixed(2)}`:'未启用');
    if(detail==='cargo'&&Array.isArray(spec[23])){const f=spec[23].reduce((x,y)=>x+Number(y||0),0),d=spec[24].reduce((x,y)=>x+Number(y||0),0);metrics('v10Source',`原生组织真实单步输运 · 食物 ${f.toFixed(3)} · 残骸 ${d.toFixed(3)} · 绿色和琥珀色来自真实子节点→父节点通量。`);}else metrics('v10Source',actualSurface?`原生有限宽度体表：${data.active} 活跃段 · 组织厚度、柔顺性与有效形变宽度来自 C++ · 材质为观察表达`:`C++ 身体拓扑：${data.n.length} 节点，实际表达 ${data.active} · 当前体态直接取自采样 · 材质为观察表达`);
    if(startGpu()){
      gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);gl.viewport(0,0,w,h);gl.useProgram(prog);gl.uniform2f(uSize,w,h);gl.clearColor(0,0,0,0);gl.clear(gl.COLOR_BUFFER_BIT);
      gl.bindVertexArray(vao);gl.bindBuffer(gl.ARRAY_BUFFER,instances);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(data.shapes),gl.DYNAMIC_DRAW);gl.drawArraysInstanced(gl.TRIANGLE_STRIP,0,4,data.shapes.length/10);gl.bindVertexArray(null);
      drawCount++;if(drawCount%25===0)metrics('v10GpuStatus',`WebGL2 · ${data.shapes.length/10} 实际身体几何实例`);
    }else canvasDraw(data.shapes,w,h);
  }
  function setDetail(next){detail=next;for(const [id,d] of [['v10Shell','shell'],['v10Function','function'],['v10Control','control'],['v107Cargo','cargo']])$(id).classList.toggle('is-active',d===next);
    metrics('v10Meaning',detail==='cargo'?'只绘制 C++ 本 tick 实际输运的食物与残骸流。绿色为食物，琥珀色为残骸；不产生预制内部器官。':detail==='control'?'紫色标识真实部件控制索引和驱动参数映射，不代表未导出的完整神经突触网络。':detail==='function'?'根据遗传部件功能类型着色，图案不是新增器官或虚构生命阶段。':'只显示真实已表达身体节点、关节和连接；表面光泽属于视觉材料，不是实体器官体积。');drawSpecimen();}
  for(const [id,d] of [['v10Shell','shell'],['v10Function','function'],['v10Control','control'],['v107Cargo','cargo']])$(id).onclick=()=>setDetail(d);
  function worldMode(){displayed=false;deck.hidden=true;stage.classList.remove('v10-inspecting');mode.classList.add('v10-world-on');open.classList.remove('is-active');}
  function enter(){const a=current();if(!a){if(!findExpressed()){metrics('v10Meaning','请先运行或载入真实生命快照。');return;}}
    if(!displayed){pan={x:0,y:0};zoomLevel=1;metrics('v10ZoomText','×1');$('v10Zoom').value='1';}
    displayed=true;deck.hidden=false;stage.classList.add('v10-inspecting');mode.classList.remove('v10-world-on');open.classList.add('is-active');drawSpecimen();
  }
  $('v10Explore').onclick=()=>{if(findExpressed()){pan={x:0,y:0};zoomLevel=1;$('v10Zoom').value='1';metrics('v10ZoomText','×1');if(!displayed)enter();else drawSpecimen();}};
  open.onclick=enter;mode.onclick=worldMode;$('v10Return').onclick=worldMode;$('v10Fit').onclick=()=>{zoomLevel=1;pan={x:0,y:0};$('v10Zoom').value=1;metrics('v10ZoomText','×1');drawSpecimen();};
  $('v10Zoom').oninput=e=>{zoomLevel=Math.max(1,Math.min(128,Number(e.target.value)||1));metrics('v10ZoomText','×'+zoomLevel);drawSpecimen();};
  const changeZoom=mult=>{zoomLevel=Math.max(1,Math.min(128,zoomLevel*mult));$('v10Zoom').value=String(Math.round(zoomLevel));metrics('v10ZoomText','×'+Number(zoomLevel.toFixed(1)));drawSpecimen();};
  $('v10ZoomIn').onclick=()=>changeZoom(1.5);$('v10ZoomOut').onclick=()=>changeZoom(1/1.5);
  let pointer=null;for(const canvas of [gCanvas,cCanvas]){
    canvas.addEventListener('pointerdown',e=>{pointer={x:e.clientX,y:e.clientY,panX:pan.x,panY:pan.y};canvas.setPointerCapture?.(e.pointerId);});
    canvas.addEventListener('pointermove',e=>{if(!pointer)return;const rect=canvas.getBoundingClientRect();pan.x=pointer.panX+(e.clientX-pointer.x)*canvas.width/rect.width;pan.y=pointer.panY+(e.clientY-pointer.y)*canvas.height/rect.height;drawSpecimen();});
    canvas.addEventListener('pointerup',()=>pointer=null);canvas.addEventListener('pointercancel',()=>pointer=null);
    canvas.addEventListener('wheel',e=>{e.preventDefault();changeZoom(e.deltaY<0?1.25:.8);},{passive:false});
  }
  window.addEventListener('keydown',e=>{if(e.key==='Escape'&&displayed)worldMode();});window.addEventListener('resize',()=>{if(displayed)drawSpecimen();});
  // Capture the real hit candidate BEFORE native.js recenters the world camera.
  // Empty canvas clicks must not switch modes merely because an older focusId exists.
  let clickedUid=null;
  world.addEventListener('click',e=>{clickedUid=null;if(!latest)return;
    const r=world.getBoundingClientRect(),x=(e.clientX-r.left)/r.width*world.width,y=(e.clientY-r.top)/r.height*world.height;
    let best=24;for(const [uid,d] of coords()){const delta=Math.hypot(d.x-x,d.y-y);if(delta<best){best=delta;clickedUid=uid;}}
  },true);
  world.addEventListener('click',()=>{const id=clickedUid;clickedUid=null;requestAnimationFrame(()=>{if(id!=null&&focusId===id)enter();});});
  const previous=render;
  render=function(s,seek=false){previous(s,seek);if(displayed)drawSpecimen();};
  window.EvoLabV10={enter,world:worldMode,render:drawSpecimen,topology,get mode(){return displayed?'structure':'world';},get stats(){return {instanceCount:modelCache?.shapes.length/10||0,bodyNodes:modelCache?.n.length||0,gpuActive:glReady};}};
})();
