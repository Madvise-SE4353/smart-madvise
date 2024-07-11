# Smart-Madvise
A kernel module to replace sys call madvise and make it smarter

## Env
- ubuntu 22.04 
- kernel version 6.5.0.41-generic

## Files
- smart-madvise-kprobe.c: the verison that uses kprobe API to implement 
- smart-madvise-syscalltable.c: the version that uses sys_call_table ABI to implement (deprecated)

## Usage & Docs
reference: [https://ipads.feishu.cn/docx/EWLydWFgxo8goqxvVPscdO86nMT?from=from_copylink](https://ipads.feishu.cn/docx/EWLydWFgxo8goqxvVPscdO86nMT?from=from_copylink)