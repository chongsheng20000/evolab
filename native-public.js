'use strict';
// Public beta status overlay: never invents a simulation state.
(() => {
 const banner=document.createElement('section');banner.className='public-banner';banner.setAttribute('role','status');
 banner.innerHTML='<div><b>🌐 EvoLab Ω · 功能生态公测</b><span id="publicCapacity">连接计算服务器…</span></div><small>每位访客拥有独立的 C++ 演化世界。服务器有并发与运行时间上限；检查点请自行下载。免费托管可能休眠。</small>';
 const style=document.createElement('style');style.textContent='.public-banner{display:flex;justify-content:space-between;align-items:center;gap:13px;padding:12px 17px;margin:10px 12px;border:1px solid #39695c;border-radius:12px;background:linear-gradient(100deg,#073b37,#0b221f);color:#d8f4e9;font:12px/1.45 system-ui,sans-serif}.public-banner>div{display:flex;flex-wrap:wrap;align-items:center;gap:15px}.public-banner b{font-size:14px;color:#bcf9df}.public-banner span{color:#dbd09d}.public-banner small{font-size:11px;color:#93bcb0}@media(max-width:700px){.public-banner{display:block}.public-banner small{display:block;margin-top:8px}}';
 document.head.append(style);document.body.prepend(banner);
 async function refresh(){try{const r=await fetch('/api/public/info',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();document.querySelector('#publicCapacity').textContent=`计算席位 ${s.active}/${s.maxActive} · 单世界最多 ${s.capacity} 生命 · ${s.maxTicks.toLocaleString()} tick`;}
 catch{document.querySelector('#publicCapacity').textContent='服务器暂不可用';}}
 refresh();setInterval(refresh,16000);
})();

// Accessible mobile quick-start: submits the existing world configuration.
(() => {
 const btn=document.createElement('button');
 btn.id='mobileWorldStart';btn.type='button';btn.textContent='▶ 开始演化';
 btn.setAttribute('aria-label','开始当前参数的真实 C++ 生命模拟');
 btn.style.cssText='position:fixed;right:14px;bottom:calc(14px + env(safe-area-inset-bottom));z-index:1100;max-width:calc(100vw - 28px);border:1px solid #d5e7c3;border-radius:13px;background:#c1dfb7;color:#10281f;font:700 15px system-ui,sans-serif;padding:14px 22px;box-shadow:0 5px 28px #0009;cursor:pointer';
 const style=document.createElement('style');style.textContent='@media(min-width:961px){#mobileWorldStart{display:none}}#mobileWorldStart:disabled{opacity:.65;cursor:default}';
 document.head.appendChild(style);document.body.appendChild(btn);
 btn.addEventListener('click',()=>{document.querySelector('#config')?.requestSubmit();});
 setInterval(()=>{const source=document.querySelector('#start');if(!source)return;btn.disabled=source.disabled;btn.textContent=source.disabled?'世界演化中':'▶ 开始演化';},550);
})();

// v10.8: observation-only functional evidence. Does not alter experiments or auto-start.
(() => {
  const parent=document.querySelector('.proof');
  if(!parent)return;
  const panel=document.createElement('section');
  panel.className='functional-evidence';
  panel.innerHTML=`<div class="fe-head"><b>真实生存方式 · 活体功能观测</b><small>实际摄取流量 / 体内代数 · 无预设物种</small></div>
    <div class="fe-grid"><div><b>食物吸收附肢</b><strong id="fe0">—</strong></div><div><b>残骸分解器</b><strong id="fe1">—</strong></div><div><b>根状定点采集器</b><strong id="fe2">—</strong></div></div>
    <p id="feNote">启动世界后，这里显示实际从环境网格扣除的物质摄取和当前存活的器官携带者。单个生命可以同时拥有多种器官；这不是新物种、适应性或智能的证明。</p>`;
  parent.appendChild(panel);
  const css=document.createElement('style');
  css.textContent='.functional-evidence{border-top:1px solid #365448;margin-top:13px;padding:16px 3px 3px;color:#b7cfc0}.fe-head{display:flex;gap:8px;justify-content:space-between;flex-wrap:wrap}.fe-head b{color:#d4efdd;font-size:14px}.fe-head small{font-size:10px;color:#91aa9b}.fe-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px;margin-top:11px}.fe-grid>div{min-width:0;border:1px solid #305348;background:#0b261f;border-radius:8px;padding:11px}.fe-grid b{display:block;font-size:12px;color:#95c6b0}.fe-grid strong{display:block;font-size:11px;white-space:pre-line;line-height:1.6;margin-top:7px;color:#e1ebdc}.functional-evidence p{font-size:11px;line-height:1.55;color:#a1b4a9;margin:12px 0 0}@media(max-width:520px){.fe-grid{grid-template-columns:1fr}.fe-head small{font-size:10px}}';
  document.head.appendChild(css);
  let busy=false;
  async function refresh(){if(busy)return;busy=true;try{
    const r=await fetch('/api/native/status',{cache:'no-store'});if(!r.ok)return;
    const v=(await r.json()).last;if(!Array.isArray(v?.functionalUptake))return;
    const labels=['食物','残骸','食物'];for(let k=0;k<3;k++){
      document.getElementById('fe'+k).textContent=`活体 ${v.functionalCarriers?.[k]??0} 个 · 平均代数 ${(v.functionalMeanGeneration?.[k]??0).toFixed(1)}\n${labels[k]}摄取 ${(v.functionalUptake[k]??0).toFixed(3)} / tick`;
    }
    document.querySelector('#feNote').textContent=`T ${Math.round(v.tick).toLocaleString()} · 根状附肢摩擦减速合计 ${(v.anchorVelocityLoss??0).toFixed(3)} · 数据为当前计算步/现存活体，不代表长期进化成功。`;
  }catch{}finally{busy=false;}}
  refresh();setInterval(refresh,2600);
})();


// v10.9: A self-contained, accessible settings drawer. The simulator does not
// lock document scrolling; only an OPEN drawer scrolls itself.
(() => {
  const panel=document.querySelector('.panel'),form=document.querySelector('#config');
  if(!panel||!form)return;
  panel.id='worldSettingsDrawer';
  // Hoist the fixed drawer to document.body on narrow screens: several
  // immersive ancestors establish stacking contexts that would otherwise
  // paint the backdrop over the drawer despite a larger child z-index.
  const placeholder=document.createComment('world-settings-home');
  panel.parentNode.insertBefore(placeholder,panel);
  const open=document.createElement('button');open.id='worldSettingsOpen';open.type='button';open.textContent='⚙ 世界参数';open.setAttribute('aria-controls',panel.id);
  const close=document.createElement('button');close.id='worldSettingsClose';close.type='button';close.textContent='关闭 ×';close.setAttribute('aria-label','关闭世界参数');
  panel.prepend(close);document.body.appendChild(open);
  const overlay=document.createElement('button');overlay.id='worldSettingsBackdrop';overlay.type='button';overlay.setAttribute('aria-label','关闭参数界面');document.body.appendChild(overlay);
  const css=document.createElement('style');css.textContent=`
    html,body{min-height:100%;overflow-x:hidden;overflow-y:auto!important;overscroll-behavior-y:auto}
    #worldSettingsOpen,#worldSettingsClose,#worldSettingsBackdrop{display:none}
    .layout > .panel{height:calc(100dvh - 66px);max-height:calc(100dvh - 66px);overflow-y:auto!important;-webkit-overflow-scrolling:touch;overscroll-behavior:contain;scrollbar-gutter:stable}\n    .v109-ablation{padding:12px;border:1px solid #497c65;border-radius:11px;background:#0d3027;margin:8px 0 12px}
    .v109-ablation label{font-size:12px}.v109-ablation select{width:100%;max-width:100%;color:#e5f5e7}
    .v109-ablation small{font-size:10px;line-height:1.65;color:#c3d5c8;display:block;margin-top:8px}
    @media(max-width:960px){
      #v98Settings{display:none!important}
      #worldSettingsOpen{display:block;position:fixed;top:calc(75px + env(safe-area-inset-top));right:14px;z-index:1180;padding:12px 16px;border-radius:12px;border:1px solid #7db29e;background:#173d32;color:#d9f3e4;font:600 13px system-ui;box-shadow:0 6px 24px #0009}
      #worldSettingsClose{display:block;position:sticky;top:0;z-index:2;width:100%;padding:12px;border:1px solid #487762;border-radius:9px;background:#193e33;color:#e3f9e6;text-align:right;font:700 13px system-ui}
      #worldSettingsDrawer{position:fixed!important;z-index:1201!important;top:0!important;bottom:0!important;right:0!important;left:auto!important;width:min(92vw,480px)!important;max-width:480px!important;max-height:100dvh!important;height:100dvh!important;overflow-y:auto!important;overflow-x:hidden!important;-webkit-overflow-scrolling:touch;overscroll-behavior:contain;touch-action:pan-y;transform:translateX(106%);visibility:hidden;transition:transform .22s ease,visibility .22s;background:#0c211c!important;padding:calc(10px + env(safe-area-inset-top)) 16px calc(110px + env(safe-area-inset-bottom))!important;box-shadow:-15px 0 50px #0008;display:block!important}
      #worldSettingsDrawer.drawer-open{transform:translateX(0);visibility:visible;opacity:1!important}
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
  function adapt(){if(mq.matches){document.body.appendChild(panel);closeDrawer();}else {placeholder.parentNode.insertBefore(panel,placeholder.nextSibling);panel.inert=false;panel.classList.remove('drawer-open');overlay.classList.remove('drawer-open');}}
  if(mq.addEventListener)mq.addEventListener('change',adapt);else mq.addListener(adapt);
  adapt();
})();

// True per-life tick exposure and recorded births/deaths, read from the C++ state.
// Observational rates are NOT causal proof; same-seed organ ablation is the intervention.
(() => {
  const target=document.querySelector('.proof');
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
  // The short functional flux readout stays visible beside the real anatomy.
  const summary=document.querySelector('.functional-evidence'),sidebar=document.querySelector('.v98-right');
  if(summary&&sidebar){sidebar.append(summary);const style=document.createElement('style');
    style.textContent='.v98-right>.functional-evidence{margin:5px 9px 12px;padding:12px;border:1px solid #315f50;border-radius:8px;background:#0a2b24}.v98-right .fe-grid{grid-template-columns:1fr!important;gap:5px}.v98-right .fe-grid>div{padding:8px}.v98-right .fe-grid b{font-size:11px}.v98-right .fe-grid strong{font-size:10px}';document.head.append(style);
  }
  refresh();setInterval(refresh,3000);
})();
