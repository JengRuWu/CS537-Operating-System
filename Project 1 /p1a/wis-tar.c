#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>

//this is the function adding in new file into .tar//
void add_file(FILE *tar, const char* file){


    //create an array for header//
    char headername[256];

    //Fill in the padding and fileaname in the header//
    memset(&headername, '\0', 256);
    snprintf(headername, 256, "%s", file);


    //get size of the fie//
    struct stat info;
    stat(file, &info);
    uint64_t filesize = info.st_size; 


    //open the input file//
    FILE* input_file = fopen( file, "r" );
    if( input_file == NULL ){
        fprintf( stderr, "wis-tar: Cannot open file: %s\n", file);
        exit(1);
    }

    //write the header and size into .tar//
    fwrite( &headername, sizeof(headername), 1, tar );
    fwrite( &filesize, 8, 1, tar );

    //write the content into .tar//
    while( (info.st_size!=0)&&(!feof(input_file)) ){
    char buffer[2000];
    size_t read = fread( buffer, 1, 2000, input_file );
    fwrite( buffer, 1, read, tar);
    }


    //put the eight null paddings//
    int i;
    for(i=0;i<8;i++){
        fputc('\0', tar);
    }

    fclose(input_file);

}

int main(int argc, char *argv[]){
    
    //if user did not input correctly in the command line //
    if( argc < 3 ) { 
        fprintf(stderr, "Usage: wis-tar ARCHIVE [FILE ...]\n");
        exit(1);
    }
    
    //if user did not input correctly in the command line //
    FILE* tar_output = fopen( argv[1], "w" );
        //if something goes wrong while creating the archive//
        if( tar_output == NULL ){
            fprintf( stderr, "wis-tar: Cannot create file: %s\n", argv[1] );
            exit(1);
        }

        //call add file function to add the files into .tar//
        int i;
        for( i = 2; i < argc; ++i ){
            add_file(tar_output, argv[i]);
        }

    fclose(tar_output);
    exit(0);
}