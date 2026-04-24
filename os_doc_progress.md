# `os/` API Documentation Progress

## Workspace

- Base repository: `/home/ewoo/project/public/TizenRT`
- Working tree: `/home/ewoo/project/public/TizenRT/.worktrees/TizenRT_260417_os-api-docs`
- Working branch: `TizenRT_260417_os-api-docs`
- Base ref: `upstream/master`

## Working Rules

- Never edit the primary `master` checkout for this task.
- Keep all task changes inside this worktree.
- Commit in module-sized batches.
- Update the root tracking files whenever a module tranche changes state.
- Document public functions from `os/include/**` by reading implementation code first.
- Generate `{function}.md` only for public APIs with a real implementation function.
- Explain `static inline` and function-like macros in the owning header or module `AGENTS.md`.
- Treat `os/board/**` as a summary track. Do not follow board-private implementations for normal API docs.

## Status Definitions

- `todo`: identified but not started
- `in_progress`: currently assigned to a worker
- `done`: function doc created
- `comment_fixed`: public header/source comments corrected to match code
- `pending_dependency`: blocked on a related function or unresolved ownership
- `board_summary`: handled through the `os/board` summary track

## Current Tranche

### Active module slices

- `os/kernel/signal`: add the module guide

### Last completed tranche

- `os/kernel/signal`: wait APIs `sigsuspend()`, `sigwaitinfo()`, and `sigtimedwait()` completed in `de42b05ce`

### Expected outputs in this tranche

- `os/kernel/signal/AGENTS.md`
- Signal module-status closure after the folder guide lands
- Inventory / pending-list refresh for the module-guide tranche

## Resume Notes

- Expand `os_api_inventory.md` incrementally as new slices start.
- Record unresolved cross-module references in `os_api_pending.md`.
- Update `os_module_status.md` before and after each module commit.

## Completed Commits

- `efbf42eea` `os/logm: document public logger APIs`
- `051ed6a82` `os/compression: document public compression APIs`
- `13f407019` `os/drivers/compression: document public driver entry`
- `e2cf08f48` `os/docs: track boardctl and getrandom api docs`
- `1ec4d46df` `os/docs: refresh api tracking status`
- `f0668d187` `os/kernel/errno: document public errno APIs`
- `504860615` `os/docs: document ioctl and gettimeofday APIs`
- `ed5ba8fca` `os/docs: refresh errno and ioctl tracking`
- `c8db06d62` `os/docs: refine errno accessor comments`
- `46f5eeae8` `os/kernel/timer: document POSIX timer APIs`
- `2cec6a506` `os/docs: refresh timer tracking`
- `15e66ac52` `os/kernel/timer: refine timer docs`
- `d0c467aa2` `os/docs: refresh timer refinement tracking`
- `2b4dcebd2` `os/kernel/timer: document POSIX timer APIs`
- `243be1ae0` `os/kernel/clock: document public clock APIs`
- `3331008ab` `os/kernel/clock: refine clock api comments`
- `db2b3ed0e` `os/docs: refresh clock refinement tracking`
- `3e82634dc` `os/docs: start vfs core io tracking`
- `88741fd83` `os/fs/dirent: document directory stream APIs`
- `7e70b1cc5` `os/fs/dirent: refine seekdir docs`
- `4e4492479` `os/docs: refresh dirent tracking`
- `5bb2406f0` `os/fs/vfs: document core fd io APIs`
- `aa2b88447` `os/fs/vfs: document descriptor control APIs`
- `67469a485` `os/fs/vfs: refine descriptor control docs`
- `070125f07` `os/fs/vfs: restore fcntl api doc`
- `11fea1063` `os/fs/vfs: restore fcntl api doc`
- `727930c05` `os/docs: refresh descriptor control tracking`
- `8d925edc7` `os/docs: refresh vfs descriptor tracking`
- `b198ffe76` `os/fs/vfs: document pathname metadata APIs`
- `fcb54e034` `os/docs: refresh vfs pathname tracking`
- `d2ee2864f` `os/fs/vfs: document filesystem stat APIs`
- `deb662806` `os/fs/vfs: refine filesystem stat docs`
- `307665231` `os/docs: refresh vfs statfs tracking`
- `32f36f092` `os/fs/vfs: refine pathname metadata docs`
- `36bdd4677` `os/docs: refresh vfs pathname refinement tracking`
- `ea61d21eb` `os/fs/vfs: document select readiness APIs`
- `e618cb415` `os/fs/vfs: document poll readiness APIs`
- `5587f7ab4` `os/docs: refresh vfs readiness tracking`
- `1b59dad7e` `os/fs/vfs: document poll helper APIs`
- `0fe90f6f7` `os/fs/vfs: refine poll helper docs`
- `f4b71c86a` `os/docs: refresh poll helper tracking`
- `409b6e1d8` `os/net: document net poll helper API`
- `03b117d2f` `os/net: refine net poll docs`
- `4722715db` `os/docs: refresh net poll tracking`
- `152ddc4ba` `os/net: document socket descriptor bridge APIs`
- `5d3b4becc` `os/docs: refresh net descriptor bridge tracking`
- `37dfc15c6` `os/net: document socket fcntl bridge API`
- `830bcc6fd` `os/docs: refresh net vfcntl tracking`
- `4f85c85f0` `os/net: document socket ioctl bridge API`
- `cbf75ca28` `os/crypto: add module guide`
- `715df7581` `os/docs: refresh net ioctl and crypto tracking`
- `eb633f7ab` `os/arch: add module guide`
- `399f7d54f` `os/docs: refresh arch tracking`
- `5d777624c` `os/board: add board summary guide`
- `e064545eb` `os/docs: refresh board tracking`
- `3bb35ed32` `os/net: add module guide`
- `45cafc667` `os/docs: refresh net module tracking`
- `9b584411d` `os/fs/vfs: add module guide`
- `3a21cb60e` `os/docs: refresh vfs module tracking`
- `44d5a526a` `os/crypto: document random pool helper APIs`
- `dc8ea6326` `os/crypto: document blake2s public APIs`
- `0ab19d530` `os/docs: refresh crypto tracking`
- `f2ea740ef` `os/kernel/semaphore: document public semaphore APIs`
- `424aef0d1` `os/docs: refresh semaphore tracking`
- `64d9f4453` `os/fs/semaphore: document named semaphore APIs`
- `13d468cc7` `os/docs: refresh named semaphore tracking`
- `c09651bab` `os/crypto: document blake2s init variant APIs`
- `a41524aa3` `os/docs: refresh crypto init tracking`
- `83c45a314` `os/mm/mm_gran: document granular allocator APIs`
- `73576aed2` `os/docs: refresh gran tracking`
- `a3bc9001f` `os/fs/aio: document owner async io APIs`
- `df0ad4e98` `os/fs/mqueue: document named message queue APIs`
- `a6846e98a` `os/docs: refresh mqueue tracking`
- `d2a47cea6` `os/kernel/mqueue: document queue attribute APIs`
- `65380c7a8` `os/docs: refresh kernel mqueue attr tracking`
- `59347e835` `os/kernel/mqueue: refine queue attribute docs`
- `d50613612` `os/kernel/mqueue: document queue notification API`
- `80492cd3c` `os/docs: refresh kernel mqueue notify tracking`
- `95d6c4f2c` `os/kernel/mqueue: refine queue notification docs`
- `c247bac47` `os/kernel/mqueue: document queue receive APIs`
- `5be0d8410` `os/docs: refresh kernel mqueue receive tracking`
- `b3410b44a` `os/kernel/mqueue: document queue send APIs`
- `de9775ac4` `os/docs: refresh kernel mqueue send tracking`
- `7d7b0712d` `os/kernel/mqueue: document public helper APIs`
- `d9bc5b469` `os/kernel/mqueue: add module guide`
- `57d99b0ba` `os/docs: refresh kernel mqueue closure tracking`
- `88459c6bc` `os/net: document helper init APIs`
- `ce80ebd49` `os/mm/mm_gran: document page allocator APIs`
- `6433f64df` `os/docs: refresh mm_gran and net tracking`
- `7ab7732c1` `os/kernel/signal: document control signal APIs`
- `648dbfec7` `os/docs: refresh signal control tracking`
- `674e862c9` `os/kernel/signal: document helper signal APIs`
- `854bf47ba` `os/docs: refresh signal helper tracking`
- `8997af174` `os/kernel/signal: document signal send APIs`
- `de42b05ce` `os/kernel/signal: document wait signal APIs`
