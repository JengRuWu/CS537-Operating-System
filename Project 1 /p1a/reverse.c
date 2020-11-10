#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void reversecontend(char *line, int characters){
    int i;
    if(characters>1){
        for(i=0;i<(characters-1)/2;i++){
            char temp = line[i];
            line[i] = line[characters-2-i];
            line[characters-2-i] = temp;
        }
    }
}

int main(int argc, char *argv[]){
    
    //if user did not input correctly in the command line //
    if( (argc!= 5) ) { 
        fprintf(stderr, "Usage: reverse -i inputfile -o outputfile\n");
        exit(1);
    }

    if( (strcmp(argv[1], "-i")!=0)||(strcmp(argv[3], "-o")!=0)) { 
        fprintf(stderr, "Usage: reverse -i inputfile -o outputfile\n");
        exit(1);
    }
    
    // open the input file //
    FILE *inFile = fopen(argv[2],"r");
    if (inFile == NULL){
        fprintf(stderr, "reverse: Cannot open file: %s\n", argv[2]);
        exit(1);
    }

    struct stat info;
    stat(argv[2], &info);
    int filesize = info.st_size; 

     // open the output file //
    FILE *outFile = fopen(argv[4],"w");
    if (outFile == NULL){
        fprintf(stderr, "reverse: Cannot create file: %s\n", argv[4]);
        exit(1);
    }

    int i;
    char *buffer = NULL;
    size_t bufsize = 0;
    size_t characters;
    char **line = malloc(filesize*sizeof(char*)); 

    int linecount = 0;
    int ft = ftell(inFile);
   
    while(ft<filesize){
        line[linecount] = malloc(512);
        characters = getline(&buffer, &bufsize, inFile);
        memcpy(line[linecount], buffer, characters);
        reversecontend(line[linecount], characters);
        linecount++;
        ft = ftell(inFile);
    }

 
    for(i = linecount-1; i>=0; i--){
    fwrite(line[i], strlen(line[i]), 1 ,outFile);
    }

    for(i=0; i<linecount;i++){
            free(line[i]);
    }
    free(line);
    //int ft, i=1;
    // Set the position indicator to the end of the input file
    //fseek(inFile, 0, SEEK_END);
    // Get the num of characers in the input file
    //ft = ftell(inFile);
   // while(i<ft){
        
      //  i++;
        // Move the position indicator//
       // fseek(inFile, -i, SEEK_END); 
        // Get and print the character//
       // fprintf(outFile, "%c", fgetc(inFile));
  //  }
    // fprintf(outFile, "%c", '\n');


    fclose(inFile);
    fclose(outFile);
    exit(0);
}