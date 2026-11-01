# EvoLab Ω v10.7 — Public Beta on Render

本目录是针对公网部署裁剪后的项目：真实 C++17 模拟器通过 Node.js 后端启动，每位访客独立实验，前端保留原生演化与观察功能。

## Render 部署

1. 将本文件夹**内的全部文件**上传至 GitHub 一个新仓库的仓库根目录（请不要上传大型单人旧工程包、历史个人实验或 API 密钥）。
2. 在 Render Dashboard 选择 **New → Blueprint**，选中该 GitHub 仓库；它会读取仓库根目录的 `render.yaml`，创建 Docker Web Service。也可以用 **New → Web Service**，选择仓库及 Docker 运行时。
3. 部署完成后打开 Render 给出的 `https://<服务名>.onrender.com/native.html`。`/healthz` 返回 `{ok:true}` 表示服务已启动。
4. 默认仅 1 个同时运行的模拟（免费套餐内存有限），访客各自独立 cookie 会话，不能互读对方的历史；可以同时浏览网页，计算席位满会显示繁忙提示。
5. 如果要支持多位访客同时进行大规模演化，升级到 CPU/RAM 更高的付费实例，调整 `MAX_ACTIVE`，并再做内存及并发压测。

## 公测资源限制

- 单个模拟初始 20—150 生命，容量上限 350，最多 12,000 tick，区域至多 2×2，最多 2 个环境线程。
- 免费部署默认 1 个运行席位，8 个访客会话，单次运行最多 8 分钟，每小时每访客最多 8 次启动。
- 会话使用随机 256-bit HttpOnly + SameSite=Lax cookie；写入请求要求同源 `Origin`，外界无法读取其他会话状态或下载其他会话检查点。
- 每个访客独立 C++ 子进程、运行状态、存档目录与资源限额；不提供公开的上传代码能力。
- 公共版本不允许服务端 resume 已完成的模拟；用户可下载检查点 BIN，在本地完整工程续算。
- **免费 Render 自动休眠、重启、重新部署会丢失本地数据及模拟进程**，不能用于真正 24/7 长期演化或永久保存；后续应接对象存储或付费持久磁盘。
- 浏览器首页主入口为 `/native.html`，请注意免费实例有冷启动延迟。

## 本地测试

需要 Node.js 20+、C++17 编译器：

```bash
bash native/build.sh
PORT=10000 HOST=127.0.0.1 NODE_ENV=test node public-server.js
# 在另一终端测试
curl http://127.0.0.1:10000/healthz
node --test tests/public.test.js
```

Docker（需要本地有 Docker）：

```bash
docker build -t evolab-public-beta .
docker run --rm -p 10000:10000 -e MAX_ACTIVE=1 evolab-public-beta
```

**尚未正式上线。** 仅有此源码包和 Render 配置不代表 Render 已成功创建公网服务。必须实际获得 `onrender.com` 地址并验证该地址，才能视为部署成功。
