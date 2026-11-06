# Git 基础操作练习报告

## 一、练习目标与完成情况

本次练习围绕一次完整的 Git 工作流程展开：先在工作区编写笔记，再通过暂存区组织修改并提交到本地仓库，随后在独立分支补充内容，最后把修改合并回主分支，并将已有历史推送到自己的 GitHub 仓库。

| 作业要求 | 本次完成情况 |
| --- | --- |
| 查看仓库状态 | 使用 `git status` 检查分支、修改和暂存情况 |
| 编写学习笔记 | 在 `notes/git-practice.md` 记录 Git 概念、常用命令和排错经历 |
| 至少三次有效提交 | 已有添加笔记、修改忽略规则、补充分支笔记等独立提交 |
| 创建或修改忽略规则 | 在仓库根目录添加 `.gitignore` |
| 创建、切换和合并分支 | 创建 `git-practice`，在该分支提交后合并回 `master` |
| 查看历史并保存记录 | 使用 `git log` 查看历史，并保存操作截图 |
| 连接自己的远程仓库 | 添加名为 `github` 的连接，并推送 `master` 和 `git-practice` |

## 二、目录结构

```text
仓库根目录/
├── .gitignore
├── README.md                     # 仓库首页与文档导航
└── git-practice/
    ├── README.md                 # 本练习报告
    ├── notes/
    │   └── git-practice.md        # Git 学习笔记
    └── screenshots/              # 操作截图
```

`.gitignore` 按题目要求放在仓库根目录；本报告、学习笔记和截图放在 `git-practice/` 下。该目录是普通文件夹，不是另一个 Git 仓库，不需要在里面执行 `git init`。

## 三、实际操作过程

以下按练习顺序记录主要命令，作为操作回顾。已有提交和分支不需要再次创建。

### 1. 查看初始状态

```bash
git status
git branch
git remote -v
```

操作前先确认当前仓库、分支和远程地址。本仓库的主分支名称是 `master`，因此后续切回主分支时使用 `master`。

### 2. 新建笔记并完成第一次提交

在 `git-practice/notes/git-practice.md` 中记录 Git 的作用，工作区、暂存区与本地仓库的区别，以及 `status`、`add`、`commit` 的基本用途。

```bash
git add git-practice/notes/git-practice.md
git status
git commit -m "docs: 添加 Git 练习笔记"
```

先保存文件，再暂存，最后提交。`git add` 暂存的是执行当时的文件内容；如果之后又修改文件，需要再次执行 `git add` 才能把新修改包含进提交。

### 3. 添加忽略规则并单独提交

仓库根目录的 `.gitignore` 内容为：

```gitignore
node_modules/
dist/
.env
*.log
```

| 规则 | 用途 |
| --- | --- |
| `node_modules/` | 忽略依赖目录 |
| `dist/` | 忽略构建输出目录 |
| `.env` | 忽略环境配置文件 |
| `*.log` | 忽略以 `.log` 结尾的文件 |

```bash
git add .gitignore
git commit -m "chore: 添加 Git 忽略规则"
```

把忽略规则作为独立提交，便于从历史中区分“文档修改”和“仓库配置修改”。忽略规则主要影响尚未跟踪的文件，不能自动从历史中删除已经提交的内容。

### 4. 创建并切换练习分支

```bash
git switch -c git-practice
git branch
```

`git switch -c` 同时完成创建和切换。`git branch` 输出中，带 `*` 的分支是当前分支。

随后在练习分支补充“什么是分支”“为什么使用分支”“如何创建分支”和“如何合并分支”等内容，并完成提交：

```bash
git add git-practice/notes/git-practice.md
git commit -m "docs: 补充 Git 分支学习笔记"
```

### 5. 切回主分支并合并

```bash
git switch master
git merge git-practice
```

合并方向是把 `git-practice` 的提交纳入当前所在的 `master`。练习分支中已提交的笔记修改因此进入主分支。

本次已有历史呈线性结构，没有单独的合并提交，这与快进合并（Fast-forward）的结果一致。没有生成额外的 merge commit，不代表合并失败。

可以使用以下命令查看历史和已经合入主分支的分支：

```bash
git log --oneline --graph --decorate --all
git branch --merged master
```

### 6. 整理总结与操作截图

在完成核心操作后，继续整理练习总结并保存截图。此次文档完善将完整报告放到题目要求的 `git-practice/README.md`，根目录 README 用作导航。

截图文件：

- [操作记录截图一](screenshots/12c8dd94-c25d-4e77-af00-046584dd8111.png)
- [操作记录截图二](screenshots/ba47ccd3-4362-4dc3-86e7-e4c1c1561154.png)

### 7. 将已有仓库推送到 GitHub

本地已经有 Git 仓库和多次提交，因此在 GitHub 创建空的远程仓库后，直接添加连接即可，不需要重新初始化本地仓库。

本次使用的 GitHub SSH 地址为：

```text
git@github.com:deng2281/git-practice.git
```

主要命令记录如下：

```bash
git remote add github git@github.com:deng2281/git-practice.git
ssh -T git@github.com
git push -u github master
git push github git-practice
```

其中 `github` 是远程连接的名字；原有的 `origin` 仍指向教学平台仓库。推送时明确写出连接名，可以避免把内容发送到错误的目标。

## 四、已有提交记录

下面列出本次文档完善前已存在的六次作业提交，按从早到晚的顺序排列。后续新增记录以 `git log` 为准。

| 提交编号 | Commit message | 修改目的 |
| --- | --- | --- |
| `9144831` | `docs: 添加 Git 练习笔记` | 建立基础学习笔记 |
| `c5e7098` | `chore: 添加 Git 忽略规则` | 配置不需要跟踪的文件类型 |
| `b2243a4` | `docs: 补充 Git 分支学习笔记` | 在练习分支补充分支知识 |
| `f314a84` | `docs: 记录 Git 练习过程和总结` | 记录练习过程 |
| `9e1b368` | `docs: 将练习总结整理到独立 README` | 将总结从笔记中分离 |
| `bacad80` | `docs: 添加 Git 练习操作截图` | 保存操作记录 |

在上述历史中，`git-practice` 分支停留在 `b2243a4`，主分支继续包含总结和截图提交，这是练习结束后的正常状态。主分支已有的六次作业提交包含了练习分支的提交。

## 五、遇到的问题与处理过程

### 1. HTTPS 克隆提示双重认证

最初使用教学平台的 HTTPS 地址克隆时，服务端提示开启双重认证的账号不能使用普通密码进行 Git 认证。随后改用 SSH 地址，成功克隆仓库。

这使我认识到：网页登录密码、个人访问令牌和 SSH 密钥是不同的认证方式，不能混为一谈。

### 2. 教学平台推送被拒绝

教学平台 SSH 推送提示 `You do not have sufficient authorization for this action`；使用分配的账号通过 HTTPS 推送时，也遇到过 HTTP 403。

仓库路径经过核对，没有发现拼写错误。SSH 提示指向实际认证身份的仓库权限问题；HTTPS 403 还可能涉及服务入口或网关，需要管理员进一步核查。本次并未确认教学平台推送问题已经解决。

之后作业改为文件提交，并使用自己的 GitHub 仓库存放练习成果；已有本地文件、分支和提交历史均予以保留。

### 3. 添加远程连接提示名字已存在

尝试为 GitHub 执行 `git remote add origin ...` 时，出现 `remote origin already exists`。

原因是当前仓库已经有名为 `origin` 的连接。后来使用 `github` 作为新连接名，保留教学平台地址，同时新增 GitHub 地址。

### 4. 区分仓库地址、账号和权限

远程地址决定访问哪个仓库；SSH 密钥或 HTTPS 凭据用于证明身份；服务器再检查该身份是否有读取或写入权限。

同一把密钥可以用于多个有权限的仓库，但添加密钥本身不会自动获得仓库写入权限。个人主页没有列出某个组织仓库，也不能单凭这一点断定是否拥有该仓库的权限。

## 六、本次练习的收获

1. 理解了 Git 不只是保存最终文件，还能保存每一步修改的版本记录。
2. 掌握了“保存文件 → 查看状态 → 暂存 → 检查暂存内容 → 提交”的基本流程。
3. 学会让每次提交对应一个明确目的，并使用能够说明修改内容的 commit message。
4. 认识到目录名和分支名是不同概念，即使都叫 `git-practice` 也不会冲突。
5. 理解了创建分支、切换分支、合并分支的先后关系，以及快进合并不一定新增提交。
6. 学会区分本地提交和远程推送：远程推送失败不会自动删除本地提交。
7. 在排查问题时，先检查状态、目标地址和认证身份，再判断权限，避免盲目重复操作。

## 七、后续提交与检查

完善文档后，应先检查修改，再自行暂存、提交和推送：

```bash
git diff --check
git diff --stat
git add README.md git-practice/README.md git-practice/notes/git-practice.md
git diff --cached --stat
git commit -m "docs: 完善 Git 练习报告与学习笔记"
git push github master
git status
```

提交前确认 README、学习笔记、根目录 `.gitignore` 和截图均已包含在材料中。本文档完善发生在 `master` 上，不必为了更新报告重新创建练习分支。
