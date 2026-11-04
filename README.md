
# Git 基础操作练习

## 完成的操作

1. 使用 git status 查看仓库状态。
2. 创建 Git 学习笔记，使用 git add 和 git commit 完成提交。
3. 添加 .gitignore 忽略规则并提交。
4. 创建并切换到 git-practice 分支。
5. 在练习分支补充笔记并提交。
6. 切回主分支，合并 git-practice 分支。
7. 使用 git log --oneline 查看提交记录。

## Commit message

- docs: 添加 Git 练习笔记
- chore: 添加 Git 忽略规则
- docs: 补充 Git 分支学习笔记
- docs: 记录 Git 练习过程和总结

## 分支创建和合并过程

使用 git switch -c git-practice 创建并切换到练习分支。
补充笔记并提交后，切回主分支，
使用 git merge git-practice 完成合并。

## .gitignore 规则

- node_modules/：忽略依赖目录。
- dist/：忽略构建输出目录。
- .env：忽略环境配置文件。
- *.log：忽略日志文件。

## 本次练习的收获

理解了工作区、暂存区和本地仓库的区别，
掌握了编辑、暂存、提交的基本流程，
学会了创建、切换和合并分支。
