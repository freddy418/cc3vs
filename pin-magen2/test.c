#include <stdlib.h>
#include <stdio.h>

#define LSIZE 18

int main(int argc, char * argv){
  FILE *fp;
  char * buf1;
  char buf2[LSIZE];
  size_t result;
  int i;

  fp=fopen("test.txt", "r");

  buf1 = malloc(sizeof(char) * LSIZE);
  
  if (fp!=NULL){
    fgets(buf1, LSIZE, fp);
    fgets(buf2, LSIZE, fp);
  }

  fclose(fp);
  
  printf("characters read: %s %s\n", buf1, buf2);

  exit(0);
}
