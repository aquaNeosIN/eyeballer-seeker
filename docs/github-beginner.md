# 第一次用 GitHub 看这个仓库

你不需要先学会 Git 命令。用浏览器就能看完这一版的接线图。

## 这个页面是什么

仓库地址：

https://github.com/aquaNeosIN/eyeballer-seeker

打开以后，中间一大列是文件。点一个文件夹就进到里面，点一个文件就打开它。左上角的分支名字如果是 `main`，你看的是已经合并进主线的内容。如果是别的名字，你看的是某一次改动还没合并的版本。

## 看接线图

1. 点 `hardware` 文件夹。
2. 点 `schematic.png`。
3. 图片打开后，用两根手指放大。线比较密，放大才能看清脚号。

同一张图也有 PDF：`hardware/schematic.pdf`。手机上 PNG 更方便。

## 下载整个仓库（不用装 Git）

1. 打开仓库首页。
2. 点绿色的 **Code** 按钮。
3. 选 **Download ZIP**。
4. 解压后，用手机相册或文件 App 打开 `hardware/schematic.png`。

ZIP 是当时那个分支的一份拷贝。以后仓库更新了，要重新下载，旧 ZIP 不会自己变。

## 用手机看

- 手机浏览器打开上面的网址就行，GitHub 会显示一个简化页面。
- 也可以装 GitHub 官方 App，登录后搜索 `eyeballer-seeker`。
- 图片要点进去再放大。App 里有时要先点文件名，再点预览。

## 什么是 Pull Request

Pull Request（页面上常写成 **PR**）是一份「请把我的改动并进主线」的申请。里面有：

- 说明：这次改了什么。
- **Files changed**：具体哪些文件有增删。绿色是新加的，红色是删掉的。
- **Conversation**：讨论。

你现在要看的硬件图，就是在这样一个 PR 里。

## 在网页上打开这个 PR

1. 打开仓库首页。
2. 点上面的 **Pull requests**。
3. 点标题里带「硬件」或 `hardware` 的那一条。
4. 想看图，有两个办法：
   - 点 **Files changed**，找到 `hardware/schematic.png`，GitHub 会直接显示图片。
   - 点右侧或说明里的分支名，回到文件列表，再按上面的步骤打开 `hardware/schematic.png`。

## 在手机上打开这个 PR

1. 打开仓库，点 **Pull requests**（有的手机页面要先点菜单）。
2. 点那一条 PR。
3. 点 **Files** 或 **Files changed**。
4. 点 `schematic.png`。如果只看到一堆文字、没有图，回到 PR 说明里写的分支，用前面的办法直接打开 PNG。

看不懂某一行英文按钮时，记住这三个词就够用这一版：

| 按钮 | 做什么 |
| --- | --- |
| Code → Download ZIP | 把文件打包下载 |
| Pull requests | 看还没合并的改动 |
| Files changed | 看这次具体改了哪些文件 |
