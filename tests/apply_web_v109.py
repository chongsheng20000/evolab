from pathlib import Path
root=Path(__file__).parents[1]
p=root/'native/controller.js';s=p.read_text()
assert "let ablate=input.ablate" in s
s=s.replace('let ablate=input.ablate??',"let organAblation=input.organAblation??(resume&&state.config?state.config.organAblation:'none');\n if(!['none','absorber','digester','root'].includes(organAblation))throw Error('不支持的器官失能类型');\n let ablate=input.ablate??")
s=s.replace("if(ablate!==state.config.ablate||neutral!==state.config.neutral)","if(ablate!==state.config.ablate||organAblation!==state.config.organAblation||neutral!==state.config.neutral)")
s=s.replace("args.push('--ablate',ablate,'--neutral'","args.push('--organ-ablation',organAblation,'--ablate',ablate,'--neutral'")
s=s.replace("state.config={...opts,ablate,neutral}","state.config={...opts,ablate,organAblation,neutral}")
# Actual state configuration assignment inspect and ensure present
if "state.config={...opts,ablate,organAblation,neutral}" not in s:
 print('WARN config assignment not replaced, looking for alternate')
 for pos in [i for i in range(len(s)) if s.startswith('ablate,neutral',i)][:4]:print(s[pos-120:pos+90])
p.write_text(s)
p=root/'public-server.js';s=p.read_text()
needle="if(input.resume===true)throw Object.assign"
assert needle in s
s=s.replace(needle,"if(input.organAblation!==undefined&&!['none','absorber','digester','root'].includes(input.organAblation))throw Object.assign(new Error('器官失能类型无效'),{httpCode:422});\n "+needle)
p.write_text(s)
p=root/'native.html';s=p.read_text()
needle='<input type="hidden" name="rate" id="startRate" value="100">'
assert needle in s
s=s.replace(needle,needle+'''<div class="v109-ablation"><label>器官功能干预（需要重新启动世界）<select name="organAblation" id="organAblation"><option value="none" selected>对照：全部器官正常</option><option value="absorber">食物吸收器失能</option><option value="digester">残骸分解器失能</option><option value="root">根状器官失能（同时消除锚定阻力）</option></select></label><small>相同随机种子、相同世界参数重复启动，用于配对干预。失能仅影响器官的功能表达，基因仍可遗传；不是物种升级，也不是自动证明适应性。</small></div>''')
s=s.replace('V10.8 / FUNCTIONAL ECOLOGY','V10.9 / EVOLUTION EVIDENCE')
p.write_text(s)
# readout in v10.9 comp evidence via status endpoint; do not duplicate visual states.
p=root/'native-public.js';s=p.read_text()
s+='''

// v10.9: A self-contained, accessible settings drawer. The simulator does not
// lock document scrolling; only an OPEN drawer scrolls itself.
(() => {
  const panel=document.querySelector('.panel'),form=document.querySelector('#config');
  if(!panel||!form)return;
  panel.id='worldSettingsDrawer';
  const open=document.createElement('button');open.id='worldSettingsOpen';open.type='button';open.textContent='⚙ 世界参数';open.setAttribute('aria-controls',panel.id);
  const close=document.createElement('button');close.id='worldSettingsClose';close.type='button';close.textContent='关闭 ×';close.setAttribute('aria-label','关闭世界参数');
  panel.prepend(close);document.body.appendChild(open);
  const overlay=document.createElement('button');overlay.id='worldSettingsBackdrop';overlay.type='button';overlay.setAttribute('aria-label','关闭参数界面');document.body.appendChild(overlay);
  const css=document.createElement('style');css.textContent=`
    html,body{min-height:100%;overflow-x:hidden;overflow-y:auto!important;overscroll-behavior-y:auto}
    #worldSettingsOpen,#worldSettingsClose,#worldSettingsBackdrop{display:none}
    .v109-ablation{padding:12px;border:1px solid #497c65;border-radius:11px;background:#0d3027;margin:8px 0 12px}
    .v109-ablation label{font-size:12px}.v109-ablation select{width:100%;max-width:100%;color:#e5f5e7}
    .v109-ablation small{font-size:10px;line-height:1.65;color:#c3d5c8;display:block;margin-top:8px}
    @media(max-width:960px){
      #worldSettingsOpen{display:block;position:fixed;top:calc(75px + env(safe-area-inset-top));right:14px;z-index:1180;padding:12px 16px;border-radius:12px;border:1px solid #7db29e;background:#173d32;color:#d9f3e4;font:600 13px system-ui;box-shadow:0 6px 24px #0009}
      #worldSettingsClose{display:block;position:sticky;top:0;z-index:2;width:100%;padding:12px;border:1px solid #487762;border-radius:9px;background:#193e33;color:#e3f9e6;text-align:right;font:700 13px system-ui}
      #worldSettingsDrawer{position:fixed!important;z-index:1201!important;top:0!important;bottom:0!important;right:0!important;left:auto!important;width:min(92vw,480px)!important;max-width:480px!important;max-height:100dvh!important;height:100dvh!important;overflow-y:auto!important;overflow-x:hidden!important;-webkit-overflow-scrolling:touch;overscroll-behavior:contain;touch-action:pan-y;transform:translateX(106%);visibility:hidden;transition:transform .22s ease,visibility .22s;background:#0c211c!important;padding:calc(10px + env(safe-area-inset-top)) 16px calc(110px + env(safe-area-inset-bottom))!important;box-shadow:-15px 0 50px #0008;display:block!important}
      #worldSettingsDrawer.drawer-open{transform:translateX(0);visibility:visible}
      #worldSettingsDrawer form{display:grid!important;grid-template-columns:minmax(0,1fr)!important;gap:13px!important}
      #worldSettingsDrawer form>*{grid-column:1 / -1!important;min-width:0!important}
      #worldSettingsDrawer input,#worldSettingsDrawer select{max-width:100%!important;min-width:0!important}
      #worldSettingsDrawer .twocol{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
      #worldSettingsBackdrop.drawer-open{display:block;position:fixed;inset:0;background:rgba(0,0,0,.5);border:0;z-index:1200}
      #mobileWorldStart{z-index:1181!important}
    }
  `;
  document.head.append(css);
  const mq=window.matchMedia('(max-width:960px)');
  function closeDrawer(){panel.classList.remove('drawer-open');overlay.classList.remove('drawer-open');open.setAttribute('aria-expanded','false');if(mq.matches)panel.inert=true;}
  function openDrawer(){panel.inert=false;panel.classList.add('drawer-open');overlay.classList.add('drawer-open');open.setAttribute('aria-expanded','true');panel.scrollTop=0;close.focus();}
  open.addEventListener('click',openDrawer);close.addEventListener('click',()=>{closeDrawer();open.focus();});overlay.addEventListener('click',closeDrawer);
  form.addEventListener('submit',closeDrawer);
  document.addEventListener('keydown',e=>{if(e.key==='Escape'&&panel.classList.contains('drawer-open')){closeDrawer();open.focus();}});
  function adapt(){if(mq.matches)closeDrawer();else {panel.inert=false;panel.classList.remove('drawer-open');overlay.classList.remove('drawer-open');}}
  if(mq.addEventListener)mq.addEventListener('change',adapt);else mq.addListener(adapt);
  adapt();
})();

// True per-life tick exposure and recorded births/deaths, read from the C++ state.
// Observational rates are NOT causal proof; same-seed organ ablation is the intervention.
(() => {
  const target=document.querySelector('.functional-evidence')||document.querySelector('.proof');
  if(!target)return;
  const box=document.createElement('section');box.className='organ-competition';
  box.innerHTML=`<h3>结构竞争与自然选择 · 实际事件账本</h3><div id="competitionData">尚无实验数据</div><p>每千生命·tick 的繁殖及死亡事件率，包含所有活体，非抽样。携带者与非携带者可能在年龄、亲缘和栖息地上不同；相关性不是因果性。进行同种子、同参数的器官失能对照才能检验贡献。</p>`;
  target.appendChild(box);
  const sty=document.createElement('style');sty.textContent='.organ-competition{border-top:1px solid #366352;padding:15px 0;margin-top:14px;color:#d1e3d6}.organ-competition h3{font-size:14px;font-weight:650;margin:3px 0 12px}.organ-competition table{font-size:11px;border-collapse:collapse;width:100%;font-variant-numeric:tabular-nums}.organ-competition td,.organ-competition th{padding:8px 5px;border-bottom:1px solid #36554b;text-align:right}.organ-competition th:first-child,.organ-competition td:first-child{text-align:left}.organ-competition th{color:#add7c1}.organ-competition p{line-height:1.7;font-size:10px;color:#a7bcae;margin-top:10px}';document.head.append(sty);
  let fetching=false;
  const per1000=(n,den)=>den?(n/den*1000).toFixed(2):'—';
  const labels=['食物吸收','残骸分解','根状采集'];
  async function refresh(){if(fetching)return;fetching=true;try{
    const r=await fetch('/api/native/status',{cache:'no-store'});if(!r.ok)return;
    const last=(await r.json()).last;if(!Array.isArray(last?.organCompetition))return;
    const table=document.createElement('table');
    const head=document.createElement('thead');head.innerHTML='<tr><th>结构组</th><th>后代/千生命·tick</th><th>死亡/千生命·tick</th><th>出生继承数</th></tr>';table.append(head);
    const body=document.createElement('tbody');
    last.organCompetition.forEach((v,k)=>{
      for(let x=1;x>=0;x--){const tr=document.createElement('tr');
        const cols=[labels[k]+(x?' 携带':' 未携带'),per1000(v[x+2],v[x]),per1000(v[x+4],v[x]),String(v[x+6])];
        cols.forEach(y=>{const td=document.createElement('td');td.textContent=y;tr.append(td)});body.append(tr);
      }
    });table.append(body);
    const destination=document.querySelector('#competitionData');destination.replaceChildren(table);
    const ablation=last.organAblation||'none';box.querySelector('h3').textContent='结构竞争与自然选择 · T '+Number(last.tick).toLocaleString()+(ablation==='none'?' · 正常对照':' · 失能：'+ablation);
  }catch{}finally{fetching=false;}}
  refresh();setInterval(refresh,3000);
})();
'''
p.write_text(s)
# mark type and state metadata
p=root/'native/controller.js';s=p.read_text().replace('native-cpp17-functional-ecology-v10.8','native-cpp17-competition-v10.9');p.write_text(s)
