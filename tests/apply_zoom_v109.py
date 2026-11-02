from pathlib import Path
p=Path(__file__).parents[1]/'native.js';s=p.read_text();start=s.index("world.style.touchAction='none';");end=s.index('let lastDraw=0;',start)
new=r'''// v10.9: multiscale observation transforms the current C++ coordinates only.
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

'''
s=s[:start]+new+s[end:];p.write_text(s)
print('updated',p)
