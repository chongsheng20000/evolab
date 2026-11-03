# EvoLab Ω v10.12 — Living Observatory / 移动与观察修复

**性质**：v10.11 的实际工程维护升级，不是预演画面、也不是预设生命演化成果。

## 本轮解决的实测问题

1. **时间尺度**：旧版本启动默认 `100 tick/s`、浏览器每约 850 ms 请求一次最新状态。C++ 实际模拟在两次观察之间可能完成数十次移动、出生、死亡，无法据此重建真实轨迹。v10.12 默认 `10 tick/s`、约 320 ms 状态轮询；当前 C++ 内核在低速（≤10 tick/s）时会额外输出密集的真实状态（在 10 tick/s 时每 2 tick 一帧）。仍支持在时间轴切换 1、10、100、1,000 tick/s 和更高速率。
2. **采样退出和真正死亡的区别**：旧版本公网只显示最多 120 个生命；本次将 Render 的画面抽样数量提升至 C++ 已支持的 200。默认容量也是 200，因此默认运行时可观察全部当时活体。容量超过 200 时页面标出 `已采样数/真实活体总数`。退出抽样 **不得**被界面断言为死亡。
3. **移动平滑**：同一真实 UID、短间隔（不超过 12 tick）、在模型允许的短距离变化，才做平滑过渡和关节角度插值。高速深时样本间隔过长时直接显示真实最新状态，不伪造从 A 到 B 的路径。状态会保留真实 UID 的视图位置，避免每次采样重置插值时间出现倒退。浏览器视觉缓动不参与 C++ 模拟。
4. **暂停检查真实生命**：在世界场地选择一个真实活体时，默认给 C++ 服务器发送 `pause`，让同一个世界停住，便于放大身体、追踪父代和检查能量；有可见 `继续真实演化` 控件。可关闭“点击生命即暂停”。暂停占用现有免费实例单个计算席位，不能无限占用共享云资源。
5. **历史体貌保留**：选中的生命退出最新抽样时，解剖面板会保留它最后一次真实采样的身体，但标明 `历史采样 · 当前存活未知`；不会虚报仍存活、死亡或迁移结果，不虚构已失去的生态互动。
6. **地图黑带与画面扭曲**：修正 v98 视图固定 320–480px 高度、Canvas intrinsic 1100×590 以及旧 11:8 CSS 比例不匹配所产生的 letterboxing。现在 backing pixels 为 1100×720、CSS 55:36，与 WebGL2 生命实例层一致；资源纹理完整覆盖观察场地。未增加伪造生态背景。
7. **参数按钮偏移**：移动 `世界参数` 至公测说明卡，取消与顶部卡片重叠的 `position:fixed; top:75px`；手机参数抽屉仍能独立滚动到最底部；模拟运行时隐藏遮住解剖面板的固定“开始演化”按钮。
8. **清晰部署更新**：JS、CSS、HTML 由公网服务器发送 `Cache-Control: no-store`，防止更新后混用旧版 JS 和新版服务器的资源上限。

## 无法仅凭当前截图断定的事项

- **某只生命真实存活了多少 tick**：屏幕上“不见了”可能是自然死亡、离开可视范围或样本退出。只有真实出生、死亡及 UID/谱系日志才能区分。
- **高速模式下每帧连续轨迹**：如果没有中间的原生状态，渲染器无法凭空知道精确轨迹。加速仅改变计算吞吐，不意味着手机能够展示每一个物理步。
- **新生态位/开放式演化是否自然出现**：已有 C++ 资源、遗传、神经、生态位与谱系机制没有被修改。更多世代的创新不能由渲染器许诺。
- **云端长寿命模拟**：免费 Render 限制计算、运行时间和临时存储，暂停也会占计算席位；没有实现永久持久世界。

## 已经完成的本地验证

- `bash native/build.sh`：原生 C++17 编译通过。
- `node --test tests/public.test.js`：实际 C++ 公网 API、多访客隔离、起止与资源上限通过。
- `npm run test:functional`, `npm run test:competition`, `npm run test:genomic`, `npm run test:multienv`：保留早期真实资源、遗传、结构删除／回补和多环境模型的因果逻辑。
- `python3 tests/test-observer-v1012.py`：在 393×852 Android 风格视口中，确认设置按钮位于信息卡内，抽屉可完整滚动（4,697px），Canvas 像素与 CSS 比例相等，真实 C++ 历史资源快照从顶部到下部全部绘制，没有 JS 异常；点击真实 UID 会向原生控制 API 发送暂停命令；历史快照不会被称为仍然存活。

测试截图：`evidence/v1012_mobile_world_verified.png`。该图使用已有的**真实 C++ 快照**，不是新跑出来的开放式演化结果。移动 WebGL2 显示还需要在实际 Android GPU 上验收，本地 Chromium 用 Canvas fallback 作了兼容验证。

## 部署至已存在的 GitHub/Render

```bash
cd ~/evolab
unzip -o ~/storage/downloads/EvoLab_Omega_v10_12_Living_Observatory_Update.zip -d .
git add -A
git commit -m "EvoLab Omega v10.12 stable living observatory"
git -c http.version=HTTP/1.1 push origin main
```

仓库：`https://github.com/chongsheng20000/evolab`。Render 服务：`srv-dbjn58942hec73dhe9b0`，工作区：`tea-dbjk13k9v7es73ab69kg`。**ZIP 成功创建不等于已发布公网**；须检查 Render 部署状态为 `Live` 且提交 SHA 与 GitHub 新提交一致，才能确认在线更新。历史上自动部署没有及时触发。

检查 `https://evolab-omega-public.onrender.com/healthz` 和 `https://evolab-omega-public.onrender.com/native.html`，首次打开可先在 Chrome 无痕模式验证。建议体验：10 tick/s 观察 → 点击生命自动暂停、打开身体结构 → 继续演化 → 100 tick/s 观察跨代变化 → 降回 1–10 tick/s 进行细致观察。不要用 1000 tick/s 下的稀疏采样判断单只生命的连续运动。
