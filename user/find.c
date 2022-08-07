#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *path, char *fname)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  
  //开启的一定是目录，文件不会继续进行

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    fprintf(2, "Usage: find <DIRECTORY> <filename>\n");
    return;
  }

  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
    fprintf(2, "find: path too long\n");
    return;
  }

  //将文件名或是目录名添加到buf之后
  strcpy(buf, path);
  p = buf+strlen(buf);
  *p++ = '/';  

  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ); 
    p[DIRSIZ] = 0;
    if(stat(buf, &st) < 0){
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }

    switch(st.type){
      case T_FILE:
        if(strcmp(fname, de.name) == 0){
          printf("%s\n", buf);
        } 
        break;

      case T_DIR:
        if(strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0){
          find(buf, fname);
        } 
        break;
    }
  }
  
  close(fd);
}

int
main(int argc, char *argv[])
{
  //判断是否小于3个参数
  if(argc < 3){
    fprintf(2, "Usage: find <path> <experssion>");
    exit(1);
  }
  //进入递归函数
  find(argv[1], argv[2]);
  exit(0);
}
