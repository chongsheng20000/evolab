'use strict';
// Public beta status overlay: never invents a simulation state.
(() => {
 const banner=document.createElement('section');banner.className='public-banner';banner.setAttribute('role','status');
 banner.innerHTML='<div><b>🌐 EvoLab Ω · 独立世界公测</b><span id="publicCapacity">连接计算服务器…</span></div><small>每位访客拥有独立的 C++ 演化世界。服务器有并发与运行时间上限；检查点请自行下载。免费托管可能休眠。</small>';
 const style=document.createElement('style');style.textContent='.public-banner{display:flex;justify-content:space-between;align-items:center;gap:13px;padding:12px 17px;margin:10px 12px;border:1px solid #39695c;border-radius:12px;background:linear-gradient(100deg,#073b37,#0b221f);color:#d8f4e9;font:12px/1.45 system-ui,sans-serif}.public-banner>div{display:flex;flex-wrap:wrap;align-items:center;gap:15px}.public-banner b{font-size:14px;color:#bcf9df}.public-banner span{color:#dbd09d}.public-banner small{font-size:11px;color:#93bcb0}@media(max-width:700px){.public-banner{display:block}.public-banner small{display:block;margin-top:8px}}';
 document.head.append(style);document.body.prepend(banner);
 async function refresh(){try{const r=await fetch('/api/public/info',{cache:'no-store'});if(!r.ok)throw Error();const s=await r.json();document.querySelector('#publicCapacity').textContent=`计算席位 ${s.active}/${s.maxActive} · 单世界最多 ${s.capacity} 生命 · ${s.maxTicks.toLocaleString()} tick`;}
 catch{document.querySelector('#publicCapacity').textContent='服务器暂不可用';}}
 refresh();setInterval(refresh,16000);
})();

// Mobile quick launch
(() => {
  const b = document.createElement('button');
  b.id = 'mobileWorldStart';
  b.type = 'button';
  b.textContent = '▶ 开始演化';
  b.style.cssText = 'position:fixed;right:14px;bottom:18px;z-index:1100;background:#c1dfb7;color:#10281f;border:1px solid #d5e7c3;border-radius:13px;padding:14px 22px;font:700 15px system-ui;box-shadow:0 5px 28px #0009';
  b.onclick = () => document.querySelector('#config').requestSubmit();
  document.body.appendChild(b);
  const s = document.createElement('style');
  s.textContent = '@media(min-width:961px){#mobileWorldStart{display:none}}#mobileWorldStart:disabled{opacity:.6}';
  document.head.appendChild(s);
  setInterval(() => {
    b.disabled = document.querySelector('#start')?.disabled || false;
    b.textContent = b.disabled ? '世界演化中' : '▶ 开始演化';
  }, 550);
})();
