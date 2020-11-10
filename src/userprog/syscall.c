#include "userprog/syscall.h"
#include "threads/vaddr.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "process.h"
// #include <unistd.h>
#include <string.h>
#include "devices/shutdown.h"
#define MAXCALL 21
#define MaxFiles 200
#define stdin 1

static void syscall_handler (struct intr_frame *);
typedef void(*CALL_PROC)(struct intr_frame*);
CALL_PROC pfn[MAXCALL];

void IWrite(struct intr_frame *f);
void IExit(struct intr_frame *f);
void ExitStatus(int status);
void ICreate(struct intr_frame *f);
void IOpen(struct intr_frame *f);
void IClose(struct intr_frame *f);
void IRead(struct intr_frame *f);
void IFileSize(struct intr_frame *f);
void IExec(struct intr_frame *f);
void IWait(struct intr_frame *f);
void ISeek(struct intr_frame *f);
void IRemove(struct intr_frame *f);
void ITell(struct intr_frame *f);
void IHalt(struct intr_frame *f);
struct file_node *GetFile(struct thread *t,int fd);
// void putbuf (char *buffer,size_t size);
struct lock syslock;

void
syscall_init (void) 
{
  lock_init(&syslock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  for(int i=0;i<MAXCALL;i++)
    pfn[i]=NULL;
  pfn[SYS_WRITE]=IWrite;  
  pfn[SYS_EXIT]=IExit;//退出线程
  pfn[SYS_CREATE]=ICreate;//创建文件
  pfn[SYS_OPEN]=IOpen;//打开文件
  pfn[SYS_CLOSE]=IClose;
  pfn[SYS_READ]=IRead;
  pfn[SYS_FILESIZE]=IFileSize;
  pfn[SYS_EXEC]=IExec;
  pfn[SYS_WAIT]=IWait;
  pfn[SYS_SEEK]=ISeek;
  pfn[SYS_REMOVE]=IRemove;
  pfn[SYS_TELL]=ITell;
  pfn[SYS_HALT]=IHalt;
}

static void
syscall_handler (struct intr_frame *f UNUSED) //系统调用
{
  if(!is_user_vaddr(f->esp))
    ExitStatus(-1);
  int No=*((int*)(f->esp));
  if(No>=MAXCALL||No<0){
    printf("We don't have this System Call!\n");
    ExitStatus(-1);
  }
  if(pfn[No]==NULL){
    // printf("%d\n" ,No);
    printf("this System Call %d not Implement!\n",No);
    ExitStatus(-1);
  }
  // printf("system call!\n");
  pfn[No](f);//调用函数
}

void IWrite(struct intr_frame *f){//三个参数
  int *esp=(int *)f->esp;
  if(!is_user_vaddr(esp+4))
    ExitStatus(-1);
  int fd=*(esp+1);//
  char *buffer=(char *)*(esp+2); 
  unsigned size=*(esp+3);

  if (fd == STDOUT_FILENO)
  {
  //   printf("123\n");
  // printf("fd:%d  \n",fd);
    putbuf(buffer,size);
    f->eax=0;
  }
  else
  {
    struct thread *cur = thread_current();
    struct file_node *fn = GetFile(cur, fd); //获取文件指针
    if (fn == NULL)
    {
      f->eax = 0;
      return;
    }
    f->eax = file_write(fn->f, buffer, size); //写文件
  }
}

void IWait(struct intr_frame *f)
{
  if (!is_user_vaddr(((int *)f->esp) + 2))
    ExitStatus(-1);
  tid_t tid = *((int *)f->esp + 1);
  if (tid != -1)
  {
    f->eax = process_wait(tid);
  }
  else
    f->eax = -1;
}

void IExit(struct intr_frame *f) //一个参数  正常退出时使用
{
  if (!is_user_vaddr(((int *)f->esp) + 2))
    ExitStatus(-1);
  struct thread *cur = thread_current();
  cur->ret = *((int *)f->esp + 1);
  f->eax = 0;
  thread_exit();
}

void ExitStatus(int status){//进程异常时调用
  struct thread *cur=thread_current();
  cur->ret=status;
  thread_exit();
}

void IExec(struct intr_frame *f)
{
  if (!is_user_vaddr(((int *)f->esp) + 2)){
    ExitStatus(-1);
  }
  // lock_acquire(&syslock);
  const char *file = (char *)*((int *)f->esp + 1);
  tid_t tid = -1;
  if (file == NULL)
  {
    // printf("%s\n", file);
    f->eax = -1;
    // lock_release(&syslock);
    return;
  }
  char *newfile = (char *)malloc(sizeof(char) * (strlen(file) + 1));

  memcpy(newfile, file, strlen(file) + 1);
  tid = process_execute(newfile);
  struct thread *t = GetThreadFromTid(tid);
  // printf("%s\n",t->name);
  
  sema_down(&t->SemaWaitSuccess);
  f->eax = t->tid;
  // t->father->sons++;
  free(newfile);
  // lock_release(&syslock);
  // sema_up(&t->SemaWaitSuccess);
}

void ICreate(struct intr_frame *f) //两个参数
{
  if (!is_user_vaddr(((int *)f->esp) + 3))
    ExitStatus(-1);
  if ((const char *)*((unsigned int *)f->esp + 1) == NULL)
  {
    f->eax = -1;
    ExitStatus(-1);
  }
  bool ret = filesys_create((const char *)*((unsigned int *)f->esp + 1), *((unsigned int *)f->esp + 2));
  f->eax = ret;
}

void IOpen(struct intr_frame *f)
{
  // lock_acquire(&syslock);
  if (!is_user_vaddr(((int *)f->esp) + 2)){
    ExitStatus(-1);
  }
  // lock_release(&syslock);
  struct thread *cur = thread_current();
  const char *FileName = (char *)*((int *)f->esp + 1);
  // printf("test:%s\n", FileName);
  if (FileName == NULL)
  {
    f->eax = -1;
    ExitStatus(-1);
  }
  // lock_acquire(&syslock);
  struct file_node *fn = (struct file_node *)malloc(sizeof(struct file_node));
  fn->f = filesys_open(FileName);
  // lock_release(&syslock);
  if (fn->f == NULL || cur->FileNum >= MaxFiles) //文件不存在或者已达到能打开的最大文件数{}
    fn->fd = -1;
  else
    fn->fd = ++cur->maxfd;//记录已打开的文件数（文件句柄）
  f->eax = fn->fd;
  // printf("%d\n",fn->fd);
  if (fn->fd == -1)//打开失败就释放内存
    free(fn);
  else
  {
    cur->FileNum++;
    list_push_back(&cur->file_list, &fn->elem);//把文件放入线程的文件列表
  }
  // lock_release(&syslock);
}

struct file_node *GetFile(struct thread *t,int fd){
  struct list_elem *e;
  for (e = list_begin(&t->file_list); e != list_end(&t->file_list); e = list_next(e))
  {
    struct file_node *fn = list_entry(e, struct file_node, elem);
    if (fn->fd == fd)
      return fn;
  }
  return NULL;
}

void IHalt(struct intr_frame *f)
{
  shutdown_power_off();
  f->eax = 0;
}

void IClose(struct intr_frame *f)
{
  if (!is_user_vaddr(((int *)f->esp) + 2))
    ExitStatus(-1);
  struct thread *cur = thread_current();
  int fd = *((int *)f->esp + 1);
  f->eax = CloseFile(cur, fd, false);//return值
}

int CloseFile(struct thread *t, int fd, int bAll)
{
  struct list_elem *e, *p;
  if (bAll)//关闭所有文件
  {
    while (!list_empty(&t->file_list))
    {
      struct file_node *fn = list_entry(list_pop_front(&t->file_list), struct file_node, elem);
      file_close(fn->f);
      free(fn);
    }
    t->FileNum = 0;
    return 0;
  }

  for (e = list_begin(&t->file_list); e != list_end(&t->file_list);)//关闭指定文件
  {
    struct file_node *fn = list_entry(e, struct file_node, elem);
    if (fn->fd == fd)
    {
      list_remove(e);
      if (fd == t->maxfd)
        t->maxfd--;
      t->FileNum--;
      file_close(fn->f);
      free(fn);

      return 0;
    }
  }
}

void IRemove(struct intr_frame *f)
{
  if (!is_user_vaddr(((int *)f->esp) + 2))
    ExitStatus(-1);
  char *fl = (char *)*((int *)f->esp + 1);
  f->eax = filesys_remove(fl);
}

void IRead(struct intr_frame *f)
{
  
  int *esp = (int *)f->esp;
  if (!is_user_vaddr(esp + 4))
    ExitStatus(-1);
  // lock_acquire(&syslock);
  int fd = *(esp + 1);
  char *buffer = (char *)*(esp + 2);
  unsigned size = *(esp + 3);

  if (buffer == NULL || !is_user_vaddr(buffer + size))
  {
    f->eax = -1;
    ExitStatus(-1);
  }

  struct thread *cur = thread_current();
  struct file_node *fn = NULL;
  unsigned int i;
  if (fd == STDIN_FILENO) //从标准输入设备读
  {
    for (i = 0; i < size; i++)
      buffer[i] = input_getc();
  }
  else //从文件读
  { 
    // lock_init(&syslock);
    // printf("%d\n",syslock.semaphore.value);
    fn = GetFile(cur, fd); //获取文件指针
    if (fn == NULL)
    {
      
      f->eax = -1;
      // lock_release(&syslock);
      return;
    }
    
    f->eax = file_read(fn->f, buffer, size);
    // lock_release(&syslock);
  }
}

void IFileSize(struct intr_frame *f)
{
  if (!is_user_vaddr(((int *)f->esp) + 2))
    ExitStatus(-1);
  struct thread *cur = thread_current();
  int fd = *((int *)f->esp + 1);
  struct file_node *fn = GetFile(cur, fd);
  if (fn == NULL)
  {
    f->eax = -1;
    return;
  }
  f->eax = file_length(fn->f);
}

void ISeek(struct intr_frame *f)
{
  if (!is_user_vaddr(((int *)f->esp) + 3))
    ExitStatus(-1);

  int fd = *((int *)f->esp + 1);
  unsigned int pos = *((unsigned int *)f->esp + 2);
  struct file_node *fl = GetFile(thread_current(), fd);
  file_seek(fl->f, pos);
}

void ITell(struct intr_frame *f)
{
  if (!is_user_vaddr(((int *)f->esp) + 2))
    ExitStatus(-1);
  int fd = *((int *)f->esp + 1);
  struct file_node *fl = GetFile(thread_current(), fd);
  if (fl == NULL || fl->f == NULL)
  {
    f->eax = -1;
    return;
  }
  f->eax = file_tell(fl->f);
}
