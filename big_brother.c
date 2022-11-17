#include "big_brother.h"
#include "fat_volume.h"
#include "fat_table.h"
#include "fat_fuse_ops.h"
#include "fat_util.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int bb_is_log_file_dentry(fat_dir_entry dir_entry) {
    return strncmp(LOG_FILE_BASENAME, (char *)(dir_entry->base_name), 3) == 0 &&
           strncmp(LOG_FILE_EXTENSION, (char *)(dir_entry->extension), 3) == 0;
}

int bb_is_log_filepath(char *filepath) {
    return strncmp(BB_LOG_FILE, filepath, 8) == 0;
}

int bb_is_log_dirpath(char *filepath) {
    return strncmp(BB_DIRNAME, filepath, 15) == 0;
}

/* Returns true if @cur_cluster is the bb cluster
 * Checks in fat table if it is bad/reserved
 * extracts dentry from cluster offset
 * checks in dentry if it has the log
*/
static bool is_orphan_dir_cluster(u32 cur_cluster){
    bool is_orphan = false;
    fat_volume vol = get_fat_volume();
    void * buf = calloc(1, sizeof(struct fat_dir_entry_s));

    if(buf == NULL){
    fprintf(stderr, "Invalid buf allocated memory");
    exit(EXIT_FAILURE);
    }    
    
    // Checks in fat table if it is bad/reserved
    if (fat_table_cluster_is_bad_sector(le32_to_cpu(((const le32 *)vol->table->fat_map)[cur_cluster]))){
        
        // Extract dentry
        off_t offset = fat_table_cluster_offset(vol->table, cur_cluster);
        size_t number_of_bytes_read = full_pread(vol->table->fd, buf, 32, offset);
        if(number_of_bytes_read == -1){
            fat_error("Bad pread of cluster info");
        }

        fat_dir_entry dentry = (fat_dir_entry) buf;
        fat_file_print_dentry(dentry);
        
        if(bb_is_log_file_dentry(dentry)){
            is_orphan = true;
        }
    }

    return is_orphan;
}

/* Searches for a cluster that could correspond to the bb directory and returns
 * its index. If the cluster is not found, returns the position of the next free cluster.
 */
static u32 search_bb_orphan_dir_cluster(bool *dir_exist) {
    fat_volume vol = NULL;
    u32 curr_cluster = 0;

    vol = get_fat_volume();
     
    while(is_orphan_dir_cluster(curr_cluster) && curr_cluster < 10000){
        curr_cluster++; 
    }

    // if orphan dir is not found => get the next free cluste to create it
    if(curr_cluster == 10000){
        curr_cluster = fat_table_get_next_free_cluster(vol->table);
        *dir_exist = false;
    }
    return curr_cluster;
}    

/* Creates the /bb directory as an orphan and adds it to the file tree as 
 * child of root dir.
 */
static int bb_init_log_dir(u32 start_cluster, bool *dir_exist) {
    errno = 0;
    fat_volume vol = NULL;
    fat_tree_node root_node = NULL;

    vol = get_fat_volume();

    // If dir not exist set in FAT table as bad/reserverved
    if(!(*dir_exist)){
        fat_table_set_next_cluster(vol->table, start_cluster, FAT_CLUSTER_BAD_SECTOR);
    }

    // Create a new file from scratch, instead of using a direntry like normally done.
    fat_file loaded_bb_dir = fat_file_init_orphan_dir(BB_DIRNAME, vol->table, start_cluster);

    // Add directory to file tree. It's entries will be like any other dir.
    root_node = fat_tree_node_search(vol->file_tree, "/");

    if(root_node == NULL || errno != 0){
        errno = ENOENT; //Not such file or directory
        return -errno;
    }
    
    vol->file_tree = fat_tree_insert(vol->file_tree, root_node, loaded_bb_dir);

    return -errno;
}

void bb_init_log_file(){
    fat_tree_node file_node;
    fat_volume vol = get_fat_volume();
    
    file_node = fat_tree_node_search(vol->file_tree, BB_LOG_FILE);
    
    if (file_node != NULL){
        DEBUG("log already exists");
        DEBUG(BB_LOG_FILE);
        return;
    }
    DEBUG("creating log");
    int mknod_err = fat_fuse_mknod(BB_LOG_FILE, 0, 0);
    
    if (mknod_err != 0){
        DEBUG("Not possible create log file");
        return;
    }
    
    // check correct creation of file
    file_node = fat_tree_node_search(vol->file_tree, BB_LOG_FILE);
    assert(file_node != NULL);
    return;
}

int bb_init_log() {
    errno = 0;
    bool dir_exist = true;
    u32 cluster = search_bb_orphan_dir_cluster(&dir_exist);
    bb_init_log_dir(cluster, &dir_exist);
    bb_init_log_file();

    return -errno;
}