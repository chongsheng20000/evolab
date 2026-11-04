#!/usr/bin/env python3
"""GPU/Canvas UID pose consistency and camera boundary regression; real C++ stored data."""
from pathlib import Path
import json, re
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
state=json.loads((root/'v98-preview.json').read_text())
assert state.get('individuals') and state.get('resourceGrid')
html=(root/'native.html').read_text()
html=re.sub(r'<script\b[^>]*src="[^"]*"[^>]*></script>','',html)
html=re.sub(r'<link\b[^>]*rel="stylesheet"[^>]*>','',html)
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--use-gl=angle','--use-angle=swiftshader'])
 page=browser.new_page(viewport={'width':393,'height':852},is_mobile=True,has_touch=True,device_scale_factor=1)
 errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
 page.set_content(html,wait_until='domcontentloaded')
 for f in ['native.css','v98.css','gpu-living.css','living-viewport-v10.css','v103-network.css']:
  page.add_style_tag(content=(root/f).read_text())
 page.evaluate('window.fetch=async()=>({ok:true,status:200,json:async()=>({phase:"idle",paused:false,rate:10,last:null,samples:[],frames:[]})})')
 for f in ['native.js','v98-ui.js','gpu-living.js','living-viewport-v10.js','v103-network.js','native-public.js']:
  page.add_script_tag(content=(root/f).read_text())
 page.evaluate('(s)=>render(s,true)',state)
 assert page.evaluate('focusId===null'), 'world must not automatically track a random large-bodied organism'
 page.evaluate('''() => {window.poseCallCount=0;const old=coords;coords=function(){window.poseCallCount++;return old();};}''')
 result=page.evaluate('''() => {
  focusId=null;cameraX=0;cameraY=0;zoom=2.2;cameraClamp();
  const cx=cameraX,cy=cameraY;
  window.poseCallCount=0;
  drawWorld();
  return {calls:window.poseCallCount,cameraX:cx,cameraY:cy,worldWidth,worldHeight,
    height:world.height,width:world.width,cameraZoom:zoom,
    sameMap:lastRenderedCoords.size>0};
 }''')
 assert abs(result['cameraX']-result['worldWidth']/4.4)<.05,result
 assert abs(result['cameraY']-result['worldHeight']/4.4)<.05,result
 assert result['calls']==1, result
 assert result['sameMap'], result
 page.evaluate('cameraX=worldWidth;cameraY=worldHeight;cameraClamp();drawWorld()')
 result2=page.evaluate('''() => ({x:cameraX,y:cameraY,w:worldWidth,h:worldHeight,zoom:zoom})''')
 assert abs(result2['x']-(result2['w']-result2['w']/4.4))<.05,result2
 assert abs(result2['y']-(result2['h']-result2['h']/4.4))<.05,result2
 page.wait_for_timeout(100)
 assert not errors,errors
 page.screenshot(path='/mnt/data/evolab_v1012_camera_edge_fixed.png')
 print('PASS one UID pose evaluation per rendered frame; field-bounded camera at all edges, no JS exceptions; real C++ snapshot')
 browser.close()
