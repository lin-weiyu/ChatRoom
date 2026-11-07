# 贡献指南

这个仓库同时是**教学材料**：提交历史、PR 记录、ADR 都是给人读的。
所以除了"代码能跑"，还有"记录能让后来的人看懂"这条要求。

## 分支模型

采用 **trunk-based + 短命分支**。不用 GitFlow——对单人/小团队来说，develop、
release、hotfix 三条长命分支只会制造合并地狱。

| 前缀 | 用途 | 例子 |
|---|---|---|
| `feat/` | 新功能 | `feat/protocol-framing` |
| `fix/` | 修 bug | `fix/partial-write` |
| `chore/` | 构建、依赖、环境 | `chore/dev-env` |
| `docs/` | 只改文档 | `docs/protocol-spec` |
| `test/` | 只改测试 | `test/integration-load` |
| `exp/` | 实验，可能被丢弃 | `exp/thread-per-conn` |
| `release/` | 发布准备 | `release/v1.0` |
| `hotfix/` | 线上紧急修复 | `hotfix/broadcast-crash` |

规则：

1. 分支从 `main` 切出，寿命尽量短（一个里程碑以内）
2. 合并后立即删除分支
3. 合并用 `--no-ff`，保留 merge commit

## 提交信息

格式：`<type>(<scope>): <subject>`

```
feat(protocol): add length-prefix frame codec

FrameCodec 只做纯函数转换，不碰任何 I/O，因此可以用单元测试穷举
半包、粘包、超长帧等边界情况。

Refs: #3
```

- `type`：`feat` `fix` `refactor` `test` `docs` `chore` `perf` `build` `ci`
- `scope`：`net` `protocol` `chat` `platform` `infra` `build` `ci` `docs`
- `subject`：祈使句、小写开头、不加句号、不超过 72 字符
- **正文写"为什么"，不写"改了什么"**——改了什么 git diff 已经说了

## 提交粒度

一次提交 = 一个可解释的最小意图。宁可 5 个清晰的小提交，不要 1 个"完成全部功能"。

不允许的提交：

- `fix bug`、`update`、`wip`
- 同时包含格式化和逻辑改动（审阅时无法分辨）

## Pull Request

每个 PR 必须在 `docs/pr/PR-<编号>.md` 留下说明，内容包括：

1. 背景：为什么要做这件事
2. 方案：怎么做，否掉了哪些替代方案
3. 权衡：代价是什么
4. 测试：怎么验证的（命令贴出来）
5. 自查清单

合并前检查：

- [ ] CI 全绿
- [ ] `CHATROOM_WERROR=ON` 无警告
- [ ] 新增逻辑有测试
- [ ] 有 ADR（如果涉及架构决策）
- [ ] `PROGRESS.md` 已更新

## Review 约定

Review 优先列**严重问题**，其次改进建议，最后才是风格。风格问题交给
`clang-format`，不要人肉挑。

被打回的 PR 用 `fixup!` 提交修补，合并前 `git rebase -i --autosquash` 压平，
这样主干历史是干净的"一次提交一个意图"。

## 代码规范

- 格式：`clang-format -i`（配置见 `.clang-format`）
- 行尾：LF（`.gitattributes` 强制）
- 命名：类型 `CamelCase`，函数/变量 `snake_case`，成员变量加 `_` 后缀，
  常量 `kCamelCase`
- 注释：只解释**为什么**，不解释**是什么**——完整规范见
  [`docs/comment-style.md`](docs/comment-style.md)
- 操作系统常识（fd / errno / epoll）写在 [`docs/basics.md`](docs/basics.md)，
  不要塞进源码注释

## 分层纪律

```
chat  ->  protocol  ->  net  ->  platform
```

依赖只能向下、且只能相邻。`net` 不得包含任何 `chat` 的头文件。
这条规则由 CMake 的 target 链接关系强制执行——违反了直接链接失败。

## 文档

| 目录 | 内容 |
|---|---|
| `docs/adr/` | 架构决策记录，一次决策一个文件，**不修改已通过的 ADR**，用新 ADR 取代 |
| `docs/pr/` | 每个 PR 的说明与 review 记录 |
| `docs/journal/` | 学习日志 |
| `docs/protocol.md` | 协议规范，唯一真相 |
| `PROGRESS.md` | 当前进度 |
| `CHANGELOG.md` | 版本变更 |
