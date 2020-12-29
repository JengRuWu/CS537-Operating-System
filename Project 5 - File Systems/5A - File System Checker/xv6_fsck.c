#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

int bonus_mismatch(void *img, struct dinode *inode_tables, uint child, uint parent){
	//root
	if(child==parent){
		return 1;
	}
	int entries_num = BSIZE / sizeof(struct dirent);
	for (int i = 0; i < NDIRECT; i++){
		struct dirent* entries = (struct dirent *)(img + inode_tables[parent].addrs[i]*BSIZE);
		for (int j = 0; j<entries_num; j++){
			if(entries[j].inum == child){
				return 1;
			}
		}
	}
	return 0;
}

int bonus_loop(void *img, struct dinode *inode_tables, int ninodes, uint child){
	int entries_num = BSIZE / sizeof(struct dirent);
	int traversed[ninodes];
	int current = 0;
	int current_inode = child;

	while(1){
		if(current_inode == 1){
			return 1;
		}
		for (int i = 0; i < ninodes; i++){
            if (traversed[i] == current_inode){
				return 0;
			}
        }
		traversed[current] = current_inode;
		for(int k = 0; k < NDIRECT; k++){
			if(inode_tables[current_inode].addrs[k]!=0){
				struct dirent *entries = (struct dirent *)(img + inode_tables[current_inode].addrs[k] * BSIZE);
				for (int j = 0; j < entries_num; j++){
					if (strcmp(entries[j].name, "..") == 0){
						current_inode = entries[j].inum;
						break;
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
     
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s fs.img\n", argv[0]);
	    exit(1);
    }
    int fsfd = open(argv[1], O_RDONLY);
    if (fsfd<0){
        fprintf(stderr, "image not found.\n");
        exit(1);
    }

    struct stat stat_buf;
    int ret = stat(argv[1], &stat_buf);
    if (ret<0){
        exit(1);
    }

    void * img = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
    if (img == MAP_FAILED){
        exit(1);
    }

    struct superblock *sb = (struct superblock *)(img + BSIZE);
    struct dinode* inode_tables = (struct dinode*) (img + 2 * BSIZE);
	char* bitmap_addr = (char *) (img + 3 * BSIZE + ((sb->ninodes/IPB) * BSIZE));
    uint datablock_start = 3 + sb->ninodes/IPB + sb->nblocks/(BSIZE*8);
	int entries_num = BSIZE / sizeof(struct dirent);

    if((sb->nblocks)%(BSIZE*8) > 0) {
    	datablock_start = datablock_start + 1;
    }
    int datablock_end = datablock_start + sb->nblocks;

	/* Check 3 - root directory*/
    if (inode_tables[1].type != T_DIR) {
        fprintf(stderr, "%s", "ERROR: root directory does not exist.\n");
		exit(1);
    }
    else {
        struct dirent *entries = (struct dirent *)(img + inode_tables[1].addrs[0] * BSIZE);
    	if (entries[1].inum != 1) {
            fprintf(stderr, "%s", "ERROR: root directory does not exist.\n");
            exit(1);
        } 
	} 

    int mybitmap[sb->size];
    memset(mybitmap, 0, (sb->size)*sizeof(int));
	int inodemap[sb->ninodes];
    memset(inodemap, 0, (sb->ninodes)*sizeof(int));
	int dir_count[sb->ninodes];
    memset(dir_count, 0, (sb->ninodes)*sizeof(int));
    
    for (int i=0; i < sb->ninodes; i++) {
		/* Check 1 - bad inode*/
		if (inode_tables[i].type != 0 && inode_tables[i].type != T_FILE && inode_tables[i].type != T_DIR &&inode_tables[i].type != T_DEV){
    	    fprintf(stderr, "%s", "ERROR: bad inode.\n");
    	    exit(1);
    	}

    	if (inode_tables[i].type != 0){
            for (int k=0; k < NDIRECT; k++) {
    	    	if(inode_tables[i].addrs[k]!=0) {
					/* Check 2 - bad direct address in inode*/
    	    		if(inode_tables[i].addrs[k] < datablock_start || inode_tables[i].addrs[k] > datablock_end){
                        fprintf(stderr, "%s", "ERROR: bad direct address in inode.\n");
    	    	 	    exit(1);
                    }
					/* Check 7 - direct address more than once*/
                    if(mybitmap[inode_tables[i].addrs[k]] == 1) {
                        fprintf(stderr, "%s", "ERROR: direct address used more than once.\n");
                        exit(1);
    	    	    }
                    mybitmap[inode_tables[i].addrs[k]] = 1;
					/* Check 5 - inuse inode & bitmap*/
					uint bitmap_group = inode_tables[i].addrs[k]/8;
					uint bitmap_offset = inode_tables[i].addrs[k]%8;
					if(((bitmap_addr[bitmap_group] >> bitmap_offset) & 1) == 0) {
						fprintf(stderr, "%s", "ERROR: address used by inode but marked free in bitmap.\n");
						exit(1);
					}
    	    	}
    	    }
            if(inode_tables[i].addrs[NDIRECT]!=0) {
				/* Check 2 - bad indirect address in inode*/
	    	    if(inode_tables[i].addrs[NDIRECT] < datablock_start || inode_tables[i].addrs[NDIRECT] > datablock_end){
                    fprintf(stderr, "%s", "ERROR: bad indirect address in inode.\n");
	    	        exit(1);
                }
				/* Check 8 - indirect address more than once*/
                if(mybitmap[inode_tables[i].addrs[NDIRECT]] == 1) {
                    fprintf(stderr, "%s", "ERROR: indirect address used more than once.\n");
                    exit(1);
    	    	}
                mybitmap[inode_tables[i].addrs[NDIRECT]] = 1;

                uint* indirect_pointer = (uint*) (img + BSIZE * inode_tables[i].addrs[NDIRECT]);
                for (int j = 0; j < NINDIRECT; j++) {
					if(indirect_pointer[j]!=0){
						/* Check 2 - bad indirect address in inode*/
						if(indirect_pointer[j] < datablock_start|| indirect_pointer[j] >datablock_end) {
							fprintf(stderr, "%s", "ERROR: bad indirect address in inode.\n");
							exit(1);
						}
						/* Check 8 - indirect address more than once*/
						if(mybitmap[indirect_pointer[j]] == 1) {
							fprintf(stderr, "%s", "ERROR: indirect address used more than once.\n");
							exit(1);
						}
						mybitmap[indirect_pointer[j]] = 1;
						/* Check 5 - inuse inode & bitmap*/
						uint bitmap_group = indirect_pointer[j]/8;
						uint bitmap_offset = indirect_pointer[j]%8;
						if(((bitmap_addr[bitmap_group] >> bitmap_offset) & 1) == 0) {
							fprintf(stderr, "%s", "ERROR: address used by inode but marked free in bitmap.\n");
							exit(1);
						}
					}
                }

	        }
			/* Check 4 - directory formatting*/
			int self_pointer = 0;
			int parent_pointer = 0;
			if (inode_tables[i].type == T_DIR) {
				for (int j=0; j < NDIRECT; j++){
					if(inode_tables[i].addrs[j] != 0){
						struct dirent *entries = (struct dirent *)(img + inode_tables[i].addrs[j]*BSIZE);
						for (int k = 0; k<entries_num; k++){
							if (strcmp(entries[k].name, ".") == 0){
								self_pointer = 1;
								if (entries[k].inum != i){
									fprintf(stderr, "ERROR: directory not properly formatted.\n");
									exit(1);
								}
							}else if(strcmp(entries[k].name, "..") == 0){
								if(bonus_mismatch(img, inode_tables, i,entries[k].inum)==0){
									fprintf(stderr, "ERROR: parent directory mismatch.\n");
									exit(1);
								}
								// if(bonus_loop(img, inode_tables, sb->ninodes, i)==0){
								// 	fprintf(stderr, "ERROR: inaccessible directory exists.\n");
								// 	exit(1);
								// }
								parent_pointer = 1;
							}else {
								/* for check 9-12*/
								dir_count[entries[k].inum] += 1;
							}
							inodemap[entries[k].inum] += 1;
						}
					}
				}
				if(self_pointer==0||parent_pointer==0){
					fprintf(stderr, "%s", "ERROR: directory not properly formatted.\n");
					exit(1);
				}
				/* for check 9-12*/
				uint* indirect_pointer = (uint*) (img + BSIZE * inode_tables[i].addrs[NDIRECT]);
				for (int j=0; j < NINDIRECT; j++) {
					if(indirect_pointer[j] != 0){
						struct dirent *entries = (struct dirent *)(img + indirect_pointer[j] * BSIZE);
						for (int k=0; k<entries_num; k++) {
							inodemap[entries[k].inum] += 1;
							dir_count[entries[k].inum] += 1;
						}
					}
				}
			}
    	}
    }

    /* Check 6- unuse inode & bitmap*/
    for (int i=datablock_start; i<datablock_start+sb->nblocks; i++) {
    	uint bitmap_group = i/8;
    	uint bitmap_offset = i%8;
    	if(((bitmap_addr[bitmap_group] >> bitmap_offset) & 1) == 1) {
    		if(mybitmap[i]!=1) {
    			fprintf(stderr, "%s", "ERROR: bitmap marks block in use but it is not in use.\n");
    	    	exit(1);
    		}
    	}
    }

    
    for(int i = 1; i < sb->ninodes; i++) {
		/* Check 9 - inode marked use but not found in a directory*/
		if(inodemap[i] == 0 && inode_tables[i].type != 0) {
			fprintf(stderr, "%s", "ERROR: inode marked use but not found in a directory.\n");
    	    exit(1);
		}
		/* Check 10 - inode referred to in directory but marked free*/
		if(inodemap[i] != 0 && inode_tables[i].type == 0) {
			fprintf(stderr, "%s", "ERROR: inode referred to in directory but marked free.\n");
    	    exit(1);
		}
		/* Check 11 - bad reference count for file*/
		if(inode_tables[i].type == T_FILE && inodemap[i] != inode_tables[i].nlink) {
			fprintf(stderr, "%s", "ERROR: bad reference count for file.\n");
    	    exit(1);
		}
		/* Check 12 - directory appears more than once in file system*/
		if(inode_tables[i].type == T_DIR && dir_count[i] > 1) {
			fprintf(stderr, "%s", "ERROR: directory appears more than once in file system.\n");
    	    exit(1);
		}

		if (inode_tables[i].type == T_DIR){
			if(bonus_loop(img, inode_tables, sb->ninodes, i)==0){
		 		fprintf(stderr, "ERROR: inaccessible directory exists.\n");
		 		exit(1);
		 	}
		}
	}
	exit(0);
}