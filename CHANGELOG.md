# Changelog

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### Added

- 分层工程骨架：`chatroom_platform` / `chatroom_core` / `chatroom_server`
- GoogleTest 测试框架（系统安装优先，缺失时 FetchContent 兜底）
- 两阶段 Dockerfile、`compose.yaml`、`scripts/{build,dev,run}.sh`
- GitHub Actions CI：Linux/macOS 构建矩阵 + Docker 镜像构建
- 开发规范：`CONTRIBUTING.md`、`.clang-format`、`.editorconfig`、`.gitattributes`
- 文档体系：ADR、PR 记录、学习日志模板

### Changed

- 默认分支由 `master` 更名为 `main`，接入 GitHub 远端
- `AGENTS.md` 重写为双轨制（建设期 / 教学期）

### Removed

- `src/main_server.cpp`（内容存档于提交 `f1a5d6c`，M0.5 在新结构上重写）

## [0.1.0] - 待发布

首个可运行版本。

[Unreleased]: https://github.com/lin-weiyu/ChatRoom/compare/v0.1.0...HEAD
