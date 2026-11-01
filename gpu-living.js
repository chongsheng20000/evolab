'use strict';
/* EvoLab Ω v10.1: WebGL2 instanced continuous finite-width body surfaces. No simulation writes.
   GPU computes fragment shading; C++ remains the only source of genetics and physics. */
(() => {
const world=document.getElementById('world'),holder=world?.parentElement;if(!world||!holder)return;
const gpu=document.createElement('canvas');gpu.id='livingGpu';gpu.width=world.width;gpu.height=world.height;gpu.setAttribute('aria-label','WebGL2 真实生命身体实例化渲染层');holder.append(gpu);
let gl=null,program=null,vao=null,instances=null,corner=null,active=false,mode='auto',draws=0;
const label=document.createElement('span');label.id='gpuStatus';label.textContent='检测 GPU …';
const btn=document.createElement('button');btn.id='gpuMode';btn.type='button';btn.textContent='渲染：自动 GPU';btn.title='WebGL2 实例化渲染，失败时回退到 Canvas2D';
const bar=document.querySelector('.fieldcontrols');bar?.prepend(btn);bar?.append(label);
const vertex=`#version 300 es
precision highp float;
layout(location=0)in vec2 aCorner;
layout(location=1)in vec2 aFrom;
layout(location=2)in vec2 aTo;
layout(location=3)in float aRadius;
layout(location=4)in vec4 aTint;
uniform vec2 uSize;
out vec2 vLocal;
out vec4 vTint;
flat out float vLength;
flat out float vRadius;
void main(){vec2 delta=aTo-aFrom;float len=length(delta);vec2 t=len<0.001?vec2(1.0,0.0):delta/max(len,0.001);vec2 normal=vec2(-t.y,t.x);
float along=mix(-aRadius,len+aRadius,aCorner.x);float side=aCorner.y*aRadius;
vec2 pos=aFrom+t*along+normal*side;
gl_Position=vec4(pos/uSize*2.0-1.0,0.0,1.0);gl_Position.y=-gl_Position.y;
vLocal=vec2(along,side);vTint=aTint;vLength=len;vRadius=aRadius;}`;
const fragment=`#version 300 es
precision highp float;
in vec2 vLocal;in vec4 vTint;flat in float vLength;flat in float vRadius;
out vec4 fragColor;
void main(){float d=length(vec2(vLocal.x-clamp(vLocal.x,0.0,vLength),vLocal.y));
float opacity=1.0-smoothstep(vRadius-1.4,vRadius+0.8,d);if(opacity<0.004)discard;
float edge=clamp(d/max(1.0,vRadius),0.0,1.0);
float striation=0.92+0.08*cos(vLocal.x*0.21+vLocal.y*0.07);
float highlight=pow(max(0.0,1.0-edge),2.0)*0.27+pow(max(0.0,1.0-abs(vLocal.y/max(vRadius,0.01))),4.0)*0.34;
vec3 base=vTint.rgb*(0.34+0.44*(1.0-edge))*striation+highlight*vec3(0.68,0.93,0.88);
fragColor=vec4(base,opacity*vTint.a);}`;
function compile(type,source){const sh=gl.createShader(type);gl.shaderSource(sh,source);gl.compileShader(sh);if(!gl.getShaderParameter(sh,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(sh));return sh;}
function init(){if(active)return true;try{gl=gpu.getContext('webgl2',{alpha:true,antialias:true,preserveDrawingBuffer:true,depth:false,stencil:false,premultipliedAlpha:false,powerPreference:'high-performance'});if(!gl)throw Error('WebGL2 不可用');
program=gl.createProgram();const vs=compile(gl.VERTEX_SHADER,vertex),fs=compile(gl.FRAGMENT_SHADER,fragment);gl.attachShader(program,vs);gl.attachShader(program,fs);gl.linkProgram(program);if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(program));gl.deleteShader(vs);gl.deleteShader(fs);
vao=gl.createVertexArray();gl.bindVertexArray(vao);corner=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,corner);gl.bufferData(gl.ARRAY_BUFFER,new Float32Array([0,-1,1,-1,0,1,1,1]),gl.STATIC_DRAW);gl.enableVertexAttribArray(0);gl.vertexAttribPointer(0,2,gl.FLOAT,false,8,0);
instances=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,instances);for(const [loc,count,offset] of [[1,2,0],[2,2,8],[3,1,16],[4,4,20]]){gl.enableVertexAttribArray(loc);gl.vertexAttribPointer(loc,count,gl.FLOAT,false,36,offset);gl.vertexAttribDivisor(loc,1);}gl.bindVertexArray(null);
gl.useProgram(program);gl.uniform2f(gl.getUniformLocation(program,'uSize'),gpu.width,gpu.height);gl.viewport(0,0,gpu.width,gpu.height);gl.enable(gl.BLEND);gl.blendFunc(gl.SRC_ALPHA,gl.ONE_MINUS_SRC_ALPHA);gl.disable(gl.DEPTH_TEST);
active=true;gpu.hidden=false;label.textContent='WebGL2 · 实例化 GPU';return true;
}catch(e){console.warn('EvoLab GPU unavailable:',e.message);active=false;gpu.hidden=true;label.textContent='Canvas2D · 兼容回退';return false;}}
function refreshMode(){if(mode==='cpu'){active=false;gpu.hidden=true;label.textContent='Canvas2D · 手动';btn.textContent='渲染：Canvas';return;}if(init())btn.textContent='渲染：WebGL2';else btn.textContent='渲染：Canvas 回退';}
btn.onclick=()=>{mode=mode==='cpu'?'gpu':'cpu';refreshMode();};
gpu.addEventListener('webglcontextlost',e=>{e.preventDefault();active=false;gpu.hidden=true;label.textContent='GPU 上下文丢失 · Canvas';},{passive:false});
gpu.addEventListener('webglcontextrestored',()=>{gl=null;program=null;vao=null;active=false;if(mode!=='cpu')init();});
function draw(map){if(!active||!gl)return;const colors=[[0.44,.88,.78],[.56,.77,.94],[.91,.74,.45],[.76,.94,.80],[.75,.63,.95],[.94,.69,.46],[.53,.87,.64],[.89,.82,.54]];const rows=[];
function add(ax,ay,bx,by,r,col,alpha){rows.push(ax,ay,bx,by,r,col[0],col[1],col[2],alpha);}
let organismCount=0;
for(const d of map.values()){
 const a=d.individual,id=a[0],focused=id===focusId; if(d.x < -120 || d.x > gpu.width+120 || d.y < -120 || d.y > gpu.height+120)continue;
 const muted=selectedEcotype>=0 && a[17]!==selectedEcotype;
 const parts=d.parts||a[11]||[],age=a[8];let local=[[0,0,0]];const co=Math.cos(d.h),si=Math.sin(d.h),scale=zoom;
 const transform=(x,y)=>[d.x+scale*(x*co-y*si),d.y+scale*(x*si+y*co)];
 for(let j=1;j<parts.length;j++){
  let n=parts[j],par=local[n[0]]||local[0],theta=par[2]+n[3]+(n[5]||0),on=n.length>9?!!n[9]:age/210>=n[4],realSurface=n.length>=13,x=par[0]+(on?Math.cos(theta)*n[2]*(realSurface?1:.58):0),y=par[1]+(on?Math.sin(theta)*n[2]*(realSurface?1:.58):0);
  local.push([x,y,theta]);if(!on)continue;const start=transform(par[0],par[1]),end=transform(x,y),color=colors[n[1]%8];
  add(start[0],start[1],end[0],end[1],realSurface?Math.max(.5,scale*Number(n[12])):Math.max(1.2,scale*(focused?2.05:1.05)),color,muted?.12:(focusId&&!focused?.5:.9));
 }
 const root=transform(0,0);add(root[0],root[1],root[0],root[1],Math.max(2.5,zoom*(2.7+Math.min(2.1,parts.length*.12))),a[12]>.18?colors[0]:a[12]<-.18?colors[2]:colors[3],muted?.12:1);
 organismCount++;
}
 const data=new Float32Array(rows);
 gl.viewport(0,0,gpu.width,gpu.height);gl.clearColor(0,0,0,0);gl.clear(gl.COLOR_BUFFER_BIT);gl.useProgram(program);gl.bindVertexArray(vao);gl.bindBuffer(gl.ARRAY_BUFFER,instances);gl.bufferData(gl.ARRAY_BUFFER,data,gl.DYNAMIC_DRAW);gl.drawArraysInstanced(gl.TRIANGLE_STRIP,0,4,rows.length/9);gl.bindVertexArray(null);
 draws++;if(draws%20===0)label.textContent=`WebGL2 · ${organismCount} 生命 · ${rows.length/9} 实例`;
}
refreshMode();
// Maintain original 2D world overlays; GPU renders only visual bodies.
const legacyOrganism=organism;organism=function(map){if(!active)legacyOrganism(map);};
const originalDrawWorld=drawWorld;drawWorld=function(){originalDrawWorld();if(active&&latest?.individuals?.length)draw(coords());else if(active){gl.clearColor(0,0,0,0);gl.clear(gl.COLOR_BUFFER_BIT);}};
})();
