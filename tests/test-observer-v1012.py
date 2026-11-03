#!/usr/bin/env python3
"""Offline Chromium UI checks against a TRUE previously computed C++ state frame.
No fake environment data; UI fetch is stubbed only to isolate browser rendering.
Requires Python playwright and local Chromium. No remote website accessed.
"""
from pathlib import Path
import json,re
from playwright.sync_api import sync_playwright
ROOT=Path(__file__).resolve().parents[1]
html=(ROOT/'native.html').read_text()
html=re.sub(r'<script\b[^>]*src="[^"]*"[^>]*></script>','',html)
html=re.sub(r'<link\b[^>]*rel="stylesheet"[^>]*>','',html)
snapshot=json.loads((ROOT/'v98-preview.json').read_text())
assert snapshot['individuals'] and snapshot['resourceGrid'], 'real native snapshot required'
with sync_playwright() as p:
 browser=p.chromium.launch(headless=True,executable_path='/usr/bin/chromium',args=['--no-sandbox','--disable-web-security'])
 page=browser.new_page(viewport={'width':393,'height':852},is_mobile=True,has_touch=True,device_scale_factor=1)
 errors=[];page.on('pageerror',lambda exc:errors.append(str(exc)))
 page.set_content(html,wait_until='domcontentloaded')
 for filename in ['native.css','v98.css','gpu-living.css','living-viewport-v10.css','v103-network.css']:
  page.add_style_tag(content=(ROOT/filename).read_text())
 page.evaluate('window.fetch=async()=>({ok:true,json:async()=>({phase:"idle",paused:false,rate:10,last:null,samples:[],frames:[],active:0,maxActive:1,capacity:200,maxTicks:12000}),status:200})')
 for filename in ['native.js','v98-ui.js','gpu-living.js','living-viewport-v10.js','v103-network.js','native-public.js']:
  page.add_script_tag(content=(ROOT/filename).read_text())
 page.wait_for_timeout(180)
 dims=page.locator('#world').evaluate('(e)=>({intrinsic:[e.width,e.height],rect:e.getBoundingClientRect().toJSON()})')
 assert dims['intrinsic']==[1100,720],dims
 ratio=dims['rect']['width']/dims['rect']['height'];assert abs(ratio-1100/720)<.012,ratio
 buttons=page.locator('#worldSettingsOpen').evaluate('(e)=>({rect:e.getBoundingClientRect().toJSON(),banner:e.closest(".public-banner")!==null})')
 assert buttons['banner'] and buttons['rect']['x']>0 and buttons['rect']['y']<200,buttons
 assert page.locator('#startRate').input_value()=='10'
 assert page.locator('#observerPauseOnSelect').is_visible()
 page.locator('#worldSettingsOpen').click();page.wait_for_timeout(300)
 drawer=page.locator('#worldSettingsDrawer')
 assert drawer.is_visible()
 travel=drawer.evaluate('(e)=>{e.scrollTop=e.scrollHeight;return [e.scrollTop,e.scrollHeight-e.clientHeight]}')
 assert travel[0]>4000 and travel[0]>=travel[1]-2,travel
 page.locator('#worldSettingsClose').click();page.wait_for_timeout(300)
 page.evaluate('(s)=>render(s,true)',snapshot)
 page.wait_for_timeout(100)
 field=page.locator('#world').evaluate('''e=>{let g=e.getContext('2d');return [0.07,0.25,0.5,0.75,0.94].map(r=>[...g.getImageData(e.width/2,e.height*r,1,1).data])}''')
 assert all(px[3]==255 for px in field)
 assert all(px[1]>px[0] for px in field),field
 # Clicking an actually observed UID dispatches native pause via the API.
 page.evaluate('window.observerControlCalls=[];window.fetch=async(u,opt)=>{if(opt?.method==="POST")observerControlCalls.push([u,JSON.parse(opt.body||"{}")]);return {ok:true,json:async()=>({phase:"running",paused:false,rate:10,last:null,samples:[],frames:[],accepted:true}),status:200}}')
 page.evaluate('serverPhase="running";serverPaused=false;window.evolabObserverAutoPause=true')
 positions=page.evaluate('''()=>[...coords().values()].filter(d=>d.x>35&&d.x<W-35&&d.y>35&&d.y<H-35).slice(0,2).map(d=>({x:d.x,y:d.y,uid:d.individual[0]}))''')
 if positions:
  target=positions[0];rect=page.locator('#world').bounding_box()
  page.locator('#world').click(position={'x':target['x']*rect['width']/1100,'y':target['y']*rect['height']/720},force=True)
  page.wait_for_timeout(100)
  assert any(x[0]=='/api/native/control' and x[1].get('action')=='pause' for x in page.evaluate('observerControlCalls')), 'select must pause the actual C++ engine'
 # Force observer-only state for following retained-history check.
 page.evaluate('serverPhase="idle";serverPaused=false;')
 # Selection history remains inspectable but is labeled unknown rather than presumed dead.
 uid=snapshot['individuals'][0][0]
 page.evaluate('(uid)=>{focusId=uid;lifeDossier(latest)}',uid)
 omitted={**snapshot,'tick':snapshot['tick']+25,'individuals':[a for a in snapshot['individuals'] if a[0]!=uid]}
 page.evaluate('(s)=>render(s,false)',omitted)
 page.wait_for_timeout(100)
 assert '未采样' in page.locator('#v98AnatomyCaption').inner_text()
 assert '不能判断' in page.locator('#lifeDetails').inner_text()
 assert len(errors)==0,errors
 print(f'PASS v10.12: 393×852 mobile, settings aligned, {travel[0]}px drawer scroll, 5/5 fully painted world samples, exact UID observation history, no JS errors')
 browser.close()
