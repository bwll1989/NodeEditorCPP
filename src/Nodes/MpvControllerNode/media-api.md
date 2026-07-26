# mpv media-api 接口文档

本地 REST API，由 mpv 内置脚本 `portable_config/scripts/media-api.lua` 提供。

- **Base URL**：`http://127.0.0.1:8995`
- **API 前缀**：`/api/v1`
- **验证网页**：`http://127.0.0.1:8995/`
- **默认监听**：仅本机 `127.0.0.1:8995`（可在 `portable_config/script-opts/media-api.conf` 修改）

---

## 通用约定

### 请求

- JSON 接口：`Content-Type: application/json`
- 上传接口：`multipart/form-data` 或 `application/octet-stream`

### 响应壳

成功：

```json
{
  "ok": true,
  "data": {},
  "error": null
}
```

失败：

```json
{
  "ok": false,
  "data": null,
  "error": {
    "code": "NOT_FOUND",
    "message": "media not found"
  }
}
```

### HTTP 状态码

| 状态码 | 含义 |
|--------|------|
| `200` | 成功 |
| `400` | 参数错误 |
| `404` | 资源不存在 |
| `405` | 方法不允许 |
| `409` | 状态冲突（如未播放时 seek） |
| `413` | 上传体积超限 |
| `500` | 内部错误 |

### 错误码 `error.code`

| code | 含义 |
|------|------|
| `BAD_REQUEST` | JSON / 字段非法 |
| `NOT_FOUND` | 媒体或接口不存在 |
| `PATH_DENIED` | 路径越界 |
| `SCAN_FAILED` | 媒体目录扫描失败 |
| `PLAYER_ERROR` | mpv 命令失败 |
| `INTERNAL` | 其它内部错误 |

### 媒体 ID

- 相对 `medias` 根目录的路径，POSIX 风格
- 示例：`clip.mp4`、`subdir/a.mkv`
- URL 中需编码，如 `subdir%2Fa.mkv`

### 支持的媒体扩展名

**视频**：`mp4` `mkv` `webm` `avi` `mov` `m4v` `ts` `m2ts` `wmv` `flv`

**图片**：`jpg` `jpeg` `png` `webp` `bmp` `gif` `tif` `tiff` `jxl` `avif`

列表项含 `kind` 字段：`video` 或 `image`。

---

## 接口一览

| 方法 | 路径 | 说明 |
|------|------|------|
| `GET` | `/api/v1/health` | 健康检查 |
| `GET` | `/api/v1/library` | 媒体库列表 |
| `GET` | `/api/v1/library/{id}` | 单个媒体信息 |
| `POST` | `/api/v1/library/refresh` | 重新扫描媒体库 |
| `POST` | `/api/v1/library/upload` | 上传视频到媒体库 |
| `POST` | `/api/v1/library/delete` | 删除媒体文件 |
| `DELETE` | `/api/v1/library/{id}` | 删除媒体文件（等价） |
| `GET` | `/api/v1/playback` | 当前播放状态 |
| `POST` | `/api/v1/playback/play` | 播放指定媒体 |
| `POST` | `/api/v1/playback/stop` | 停止播放 |
| `POST` | `/api/v1/playback/pause` | 暂停 / 继续 |
| `POST` | `/api/v1/playback/fullscreen` | 全屏切换 |
| `POST` | `/api/v1/playback/volume` | 设置音量 |
| `POST` | `/api/v1/playback/seek` | 跳转进度 |
| `POST` | `/api/v1/system/restart` | 重启 mpv |
| `GET` | `/api/v1/settings` | 读取持久化设置 |
| `POST` | `/api/v1/settings` | 更新持久化设置 |

---

## 1. 健康检查

### `GET /api/v1/health`

**响应示例**

```json
{
  "ok": true,
  "data": {
    "service": "mpv-media-api",
    "version": "1",
    "idle": true
  },
  "error": null
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `service` | string | 服务名 |
| `version` | string | API 版本 |
| `idle` | boolean | mpv 是否处于空闲 |

---

## 2. 媒体库

### `GET /api/v1/library`

列出媒体库。

**Query**

| 参数 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `q` | string | 否 | 按文件名 / id 子串过滤（不区分大小写） |

**响应示例**

```json
{
  "ok": true,
  "data": {
    "root": "D:/.../medias",
    "scanned_at": 1721640000,
    "count": 1,
    "items": [
      {
        "id": "demo.mp4",
        "name": "demo.mp4",
        "path": "D:/.../medias/demo.mp4",
        "size": 12345678,
        "mtime": 1721600000
      }
    ]
  },
  "error": null
}
```

### `GET /api/v1/library/{id}`

获取单个媒体。`id` 需 URL 编码。

**错误**

- `404 NOT_FOUND`：媒体不存在

### `POST /api/v1/library/refresh`

重新扫描媒体目录，无 body。

**响应**：与列表接口相同结构。

### `POST /api/v1/library/delete`

删除媒体库中的文件（同时从磁盘删除）。

**Body**

```json
{
  "id": "demo.mp4"
}
```

也可使用：`DELETE /api/v1/library/{id}`

若正在播放该文件，会先停止再删除。

**响应示例**

```json
{
  "ok": true,
  "data": {
    "deleted": true,
    "id": "demo.mp4",
    "library": {
      "root": "C:/Users/.../mpv-media-console/medias",
      "scanned_at": 1721640000,
      "count": 1,
      "items": []
    }
  },
  "error": null
}
```

### `POST /api/v1/library/upload`

上传视频到媒体库根目录。

#### 方式 A：multipart（推荐，网页使用）

- `Content-Type: multipart/form-data`
- 表单字段名：`file`
- 文件名从 `Content-Disposition` 读取

```bash
curl -F "file=@demo.mp4" http://127.0.0.1:8995/api/v1/library/upload
```

#### 方式 B：原始二进制

- `Content-Type: application/octet-stream`（可省略）
- 通过 Query `filename=` 或请求头 `X-Filename` 指定文件名

```bash
curl -X POST "http://127.0.0.1:8995/api/v1/library/upload?filename=demo.mp4" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @demo.mp4
```

**约束**

- 仅允许白名单扩展名
- 文件名会清洗；重名自动变为 `name_1.ext`
- 默认最大 `512MB`（`max_upload_mb` 可配）
- 只写入媒体库根目录，禁止目录穿越

**响应示例**

```json
{
  "ok": true,
  "data": {
    "uploaded": true,
    "id": "demo.mp4",
    "name": "demo.mp4",
    "path": "D:/.../medias/demo.mp4",
    "size": 12345678,
    "library": {
      "root": "D:/.../medias",
      "scanned_at": 1721640000,
      "count": 2,
      "items": []
    }
  },
  "error": null
}
```

---

## 3. 播放控制

### 播放状态对象

多数播放相关接口返回如下结构（字段可能随状态增减）：

```json
{
  "state": "playing",
  "media": {
    "id": "demo.mp4",
    "name": "demo.mp4",
    "path": "D:/.../medias/demo.mp4"
  },
  "time_pos": 12.3,
  "duration": 120.0,
  "pause": false,
  "fullscreen": true,
  "volume": 80,
  "mute": false
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `state` | string | `idle` / `playing` / `paused` |
| `media` | object\|null | 当前媒体；空闲时可能为 `null` |
| `time_pos` | number\|null | 当前进度（秒） |
| `duration` | number\|null | 总时长（秒） |
| `pause` | boolean | 是否暂停 |
| `fullscreen` | boolean | 是否全屏 |
| `volume` | number | 音量 `0–100` |
| `mute` | boolean | 是否静音 |

### `GET /api/v1/playback`

查询当前播放状态。

### `POST /api/v1/playback/play`

播放指定媒体（`loadfile ... replace`）。

**Body**

```json
{
  "id": "demo.mp4"
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `id` | string | 是 | 媒体库中的 id |

**错误**

- `400 BAD_REQUEST`：id 非法
- `404 NOT_FOUND`：媒体不在库中

### `POST /api/v1/playback/stop`

停止播放，回到 idle。无 body。

### `POST /api/v1/playback/pause`

暂停或继续。

**Body**（可为空对象 `{}`）

```json
{
  "paused": true
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `paused` | boolean | 否 | 省略则切换；`true` 暂停，`false` 继续 |

### `POST /api/v1/playback/fullscreen`

全屏切换。

**Body**（可为空对象 `{}`）

```json
{
  "fullscreen": true
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `fullscreen` | boolean | 否 | 省略则切换 |

### `POST /api/v1/playback/volume`

设置音量。

**Body**

```json
{
  "volume": 42
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `volume` | number | 是 | `0–100` |

### `POST /api/v1/playback/seek`

跳转播放进度。`time_pos` 与 `percent` 二选一。

**按百分比**

```json
{
  "percent": 50
}
```

**按绝对时间（秒）**

```json
{
  "time_pos": 12.5
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `percent` | number | 二选一 | `0–100` |
| `time_pos` | number | 二选一 | 秒，`>= 0`；超过时长会钳制到结尾 |

**错误**

- `409 BAD_REQUEST`：当前没有在播内容
- `400 BAD_REQUEST`：未提供有效字段

---

## 4. 系统

### `POST /api/v1/system/restart`

重启 mpv 进程（先拉起新实例，再退出当前实例）。无 body。

**响应示例**

```json
{
  "ok": true,
  "data": {
    "restarting": true,
    "exe": "D:/.../mpv.exe"
  },
  "error": null
}
```

调用后服务会短暂不可用，客户端应等待数秒后重连。

### `GET /api/v1/settings`

读取持久化设置。

```json
{
  "ok": true,
  "data": {
    "stop_position": "first"
  },
  "error": null
}
```

| 字段 | 说明 |
|------|------|
| `stop_position` | `first`：第一帧；`last`：最后一帧；`black`：黑帧（卸载媒体） |
| `loop` | `off`：不循环；`file`：单文件循环（开启后播完不会走停止画面逻辑） |

### `POST /api/v1/settings`

更新设置并写入 `portable_config/script-opts/media-api-state.conf`（重启后仍生效）。

```json
{
  "stop_position": "last"
}
```

---

## 5. 静态页面

| 方法 | 路径 | 说明 |
|------|------|------|
| `GET` | `/` | 验证网页 |
| `GET` | `/index.html` | 同上 |
| `GET` | `/app.js` | 前端脚本 |
| `GET` | `/style.css` | 样式 |

静态资源目录：`portable_config/script-opts/media-api-web/`

---

## 配置

文件：`portable_config/script-opts/media-api.conf`

```ini
port=8995
bind=0.0.0.0
media_dir=~/Videos
web_dir=~~/script-opts/media-api-web
max_upload_mb=512
stop_position=first
```

| 项 | 说明 |
|----|------|
| `port` | HTTP 端口 |
| `bind` | 监听地址（本机可用 `127.0.0.1`；局域网访问可用 `0.0.0.0`） |
| `media_dir` | 媒体库目录。默认 `~/Videos`（用户「视频」文件夹）。支持 `~/...`、`~~/...`、绝对路径 |
| `web_dir` | 静态网页目录 |
| `max_upload_mb` | 上传大小上限（MB） |
| `stop_position` | 停止/播完画面：`first` / `last` / `black`（网页可改，持久化到 `media-api-state.conf`） |
| `loop` | 循环：`off` / `file`（网页可改，同样持久化） |

目录不存在时会自动创建。配合 `portable_config/mpv.conf` 中的 `idle=yes`，停止播放后进程保持运行，可继续接收 API 请求。

---

## 调用示例（Windows）

```bash
# 健康检查
curl http://127.0.0.1:8995/api/v1/health

# 媒体列表
curl http://127.0.0.1:8995/api/v1/library

# 播放
curl -X POST http://127.0.0.1:8995/api/v1/playback/play ^
  -H "Content-Type: application/json" ^
  -d "{\"id\":\"demo.mp4\"}"

# 暂停切换
curl -X POST http://127.0.0.1:8995/api/v1/playback/pause ^
  -H "Content-Type: application/json" ^
  -d "{}"

# 跳到 30%
curl -X POST http://127.0.0.1:8995/api/v1/playback/seek ^
  -H "Content-Type: application/json" ^
  -d "{\"percent\":30}"

# 音量
curl -X POST http://127.0.0.1:8995/api/v1/playback/volume ^
  -H "Content-Type: application/json" ^
  -d "{\"volume\":50}"

# 停止
curl -X POST http://127.0.0.1:8995/api/v1/playback/stop

# 上传
curl -F "file=@demo.mp4" http://127.0.0.1:8995/api/v1/library/upload

# 重启
curl -X POST http://127.0.0.1:8995/api/v1/system/restart
```
