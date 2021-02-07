#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>

#define MAXFILES 1500
#define MAXINTLEN 50

struct result {
  char *name;
  char *permissions;
  char *user;
  char *group;
  char *size;
  char *time;
  int nlink;
};

struct flags {
  unsigned int l : 1;
  unsigned int a : 1;
  unsigned int h : 1;
};

void get_stats(char *pathname, struct stat *statbuf){
  lstat(pathname, statbuf);
}

int add_slash_to_path(char *path){
  int last_element = strlen(path) - 1;

  if(*(path + last_element) != '/')
    return 1;
  return 0;
}

char * human_readable_size(long size){
  char sizes[] = {'K','M','G','T','P','E','Z','Y'};
  char * human_readable_size = malloc(sizeof(char) * 5); //Max chars will always be 5 -> 1024G = 1T 

 
  if (size < 1024){
   sprintf(human_readable_size, "%ld", size); 
  }else{ 
    int i = 0;
    while(size >= 1024){
      size = size / 1024;
    }

    sprintf(human_readable_size, "%ld%c", size, sizes[i]);
  }
  return human_readable_size;

}

static int filetypeletter(int mode)
{
    char    c;

    if (S_ISREG(mode))
        c = '-';
    else if (S_ISDIR(mode))
        c = 'd';
    else if (S_ISBLK(mode))
        c = 'b';
    else if (S_ISCHR(mode))
        c = 'c';
#ifdef S_ISFIFO
    else if (S_ISFIFO(mode))
        c = 'p';
#endif  /* S_ISFIFO */
#ifdef S_ISLNK
    else if (S_ISLNK(mode))
        c = 'l';
#endif  /* S_ISLNK */
#ifdef S_ISSOCK
    else if (S_ISSOCK(mode))
        c = 's';
#endif  /* S_ISSOCK */
#ifdef S_ISDOOR
    /* Solaris 2.6, etc. */
    else if (S_ISDOOR(mode))
        c = 'D';
#endif  /* S_ISDOOR */
    else
    {
        /* Unknown type -- possibly a regular file? */
        c = '?';
    }
    return(c);
}

static char *lsperms(int mode)
{
    static const char *rwx[] = {"---", "--x", "-w-", "-wx",
    "r--", "r-x", "rw-", "rwx"};
    static char bits[11];

    bits[0] = filetypeletter(mode);
    strcpy(&bits[1], rwx[(mode >> 6)& 7]);
    strcpy(&bits[4], rwx[(mode >> 3)& 7]);
    strcpy(&bits[7], rwx[(mode & 7)]);
    if (mode & S_ISUID)
        bits[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        bits[6] = (mode & S_IXGRP) ? 's' : 'l';
    if (mode & S_ISVTX)
        bits[9] = (mode & S_IXOTH) ? 't' : 'T';
    bits[10] = '\0';
    return(bits);
}

char *getuser(uid_t uid){
  struct passwd *user_details = getpwuid(uid);
  return user_details->pw_name;
}

char *getgroup(gid_t gid){
  struct group *group_details = getgrgid(gid);
  return group_details->gr_name;
}

void build_result(char *name, struct stat *statbuf, struct flags *flags_ptr){
      char *size;
      char *time = ctime(&statbuf->st_mtime);
      
      char * permissions = lsperms((uintmax_t)statbuf->st_mode);
      char * user = getuser(statbuf->st_uid);
      char * group = getgroup(statbuf->st_gid);


      if((flags_ptr->a != 1) && (*name == '.')){
        return;
      }

      if(flags_ptr->h == 1){
        size = human_readable_size(statbuf->st_size);
      }
      
      if (flags_ptr->l == 1){
        if (flags_ptr-> h == 1){
          printf("%.11s %-ld %.6s %.6s %s %.16s %-s\n", permissions, statbuf->st_nlink, 
            user, group, 
            size, time, name);
        }else{
          printf("%.11s %-ld %.6s %.6s %ld %.16s %-s\n", permissions, statbuf->st_nlink, 
            user, group, 
            statbuf->st_size, time, name);
        }
      }else{
        printf("%s\t\n", name);
      }
}


void list_files(char *path, struct flags *flags_ptr){
  struct dirent *dp;
  struct dirent **sorted_dp;
  struct result total_result[MAXFILES];
  int i, j, k, n;
  i = j = k = 0;
  n = scandir(path, &sorted_dp, 0, alphasort);
  size_t max_nlink = 0;
  size_t max_user_len = 0;
  size_t max_group_len = 0;
  size_t max_size_len = 0;


  if (n >= 0){
      while((dp = sorted_dp[j])){
       
        int add_slash = add_slash_to_path(path)? 1 : 0;
        int null_space = 1;

        char *path_to_name = malloc(strlen(path) + strlen(dp->d_name) + null_space + add_slash); 
        if (!path_to_name){
          perror("Memory Allocation Failed");
        }
        
        strcpy(path_to_name, path);
        add_slash ? strcat(path_to_name, "/"): 0 ;
        strcat(path_to_name, dp->d_name);
       
        struct stat statbuf;
        get_stats(path_to_name, &statbuf);
        
        char *time = ctime(&statbuf.st_mtime);
        char *permissions = lsperms((uintmax_t)statbuf.st_mode);
        char *user = getuser(statbuf.st_uid);
        char *group = getgroup(statbuf.st_gid);
        char *size = malloc(sizeof(char)*MAXINTLEN);
        
        if((flags_ptr->l == 1)){

          if((flags_ptr->a != 1) && (*dp->d_name == '.')){
            j++;
            continue;
          }
          if(flags_ptr->h == 1){
            size = human_readable_size(statbuf.st_size);
          }else{
            sprintf(size, "%ld", statbuf.st_size);
          }
          
          max_size_len = max_size_len < strlen(size) ? strlen(size) : max_size_len;
          max_nlink = max_nlink < statbuf.st_nlink ? statbuf.st_nlink : max_nlink;
          max_user_len = max_user_len < strlen(user) ? strlen(user) : max_user_len; 
          max_group_len = max_group_len < strlen(group) ? strlen(group) : max_group_len; 
          struct result temp_result = {dp->d_name, permissions, user, group, size, time, statbuf.st_nlink};
          

          total_result[k++] = temp_result;
        }else{
          printf("%s\t", dp->d_name);
        }
        free(path_to_name);

        j++;
      }
      
      struct result temp_result;
      while(i < k){
        temp_result=total_result[i];
        
        char *max_link = malloc(sizeof(char)*MAXINTLEN);
        sprintf(max_link, "%ld", max_nlink);
        int len_nlink = strlen(max_link);

        printf("%.11s %*d %*s %*s %*s %.16s %-s\n", temp_result.permissions, (int) len_nlink, temp_result.nlink, 
            (int) max_user_len, temp_result.user, (int) max_group_len, temp_result.group, 
            (int) max_size_len, temp_result.size, temp_result.time, temp_result.name);

        i++;
      }
  }
  else{
    perror("Open Directory Failed");
  }

  free(sorted_dp);
}

void set_flags(char *arg, struct flags *current_flags){
  char c;
  while((c=*(arg++)) != '\0'){
    if(c == '-'){
      continue;
    }
    else if(c == 'l'){
      current_flags->l = 1;
    }
    else if(c == 'a'){
      current_flags->a = 1;
    }
    else if(c == 'h'){
      current_flags->h = 1;
    }
    else{
      printf("USAGE: <binary> <path> <-lah>");
      exit(1);
    }
    
  } 
}

int main(int argc, char **argv){
  // Get all the arguments
  struct flags *flags_ptr = (struct flags *)malloc(sizeof(struct flags));

  for(int i=1; i < argc; i++){
    if(**(argv + i) == '-'){
      set_flags(*(argv + i), flags_ptr);
    }
  }
  
  int i = 0;
  while(++i < argc){
    if(**(argv+i) == '-'){
      continue;
    }

    char *directory_name = *(argv + i);
    list_files(directory_name, flags_ptr);
  }
}
