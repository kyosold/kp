## 说明
从一个文件中读取每行的任务，分发给子进行进行处理

task_master 进程从配置文件 `[master]task_file` 设置的文件中循环读取每一行，然后调用 task_process 去处理。

我们只需要修改 `my_process.c` 中的 my_process 函数即可，并返回 `K_SUCC`。

## 具体操作
1. 设置配置文件的任务
```
[master]
task_file="/root/dev/task_distributor_mp/maillog"
```
2. 添加自定义处理
修改文件 `my_process.c`
```c
int my_process(char *buf, size_t buf_len)
{
    return K_SUCC;
}
```
- 处理成功返回 `K_SUCC`，失败返回 `K_ERR`。
- 当返回 `K_ERR` 后，系统会自动把失败的任务保存到 `[child]task_proc_fail_dir` 指定的目录下。
