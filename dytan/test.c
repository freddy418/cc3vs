#include <stdlib.h>
#include <stdio.h>

#define LSIZE 18

int main(int argc, char * argv){
  FILE *fp;
  char buffer[100];
  size_t result;
  int i;

  fp=fopen("test.txt", "r");
  
  // getc implementation
  //for(i=0;i<LSIZE;i++){
  //  buffer[i] = getc(fp);
  //} 
  // fread implementation
  if (fp!=NULL){
    fgets(buffer, LSIZE, fp);
  }
  //if (result != LSIZE){
  //printf("Reading error, only %d characters read\n", result);
  //}
  fclose(fp);
  
  printf("%d characters: %s\n", result, buffer);
  //printf("%u\n", &buffer[0]);
  //printf("%u\n", &buffer[1]);
  //printf("%u\n", &buffer[2]);

  //free(buffer);
  //printf("Hello world!\n");

  exit(0);
}
