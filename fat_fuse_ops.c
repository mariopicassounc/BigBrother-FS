/*
 * fat_fuse_ops.c
 *
 * FAT32 filesystem operations for FUSE (Filesystem in Userspace)
 */

#include "fat_fuse_ops.h"

#include "big_brother.h"
#include "fat_file.h"
#include "fat_filename_util.h"
#include "fat_fs_tree.h"
#include "fat_util.h"
#include "fat_volume.h"
#include <alloca.h>
#include <errno.h>
#include <gmodule.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#define LOG_PATH "/fs.log"
#define LOG_MESSAGE_SIZE 100
#define DATE_MESSAGE_SIZE 30

static void fat_fuse_init_log(fat_volume vol){
    fat_tree_node file_node;

    file_node = fat_tree_node_search(vol->file_tree, LOG_PATH);
    
    if (file_node != NULL){
        DEBUG("log already exists");
        DEBUG(LOG_PATH);
        return;
    }
    
    DEBUG("creating log");
    int mknod_err = fat_fuse_mknod(LOG_PATH, 0, 0);
    
    if (mknod_err != 0){
        DEBUG("Not possible create log file");
        return;
    }
    
    // check correct creation of file
    file_node = fat_tree_node_search(vol->file_tree, LOG_PATH);
    assert(file_node != NULL);
    return;
}

/* Writes @log to the log file if it exists
 *
 * PRE: log != NULL
 */
static void fat_fuse_log_write(const char *log) {
    assert(log != NULL);

    fat_volume vol = get_fat_volume();
    fat_tree_node log_node = fat_tree_node_search(vol->file_tree, LOG_PATH);
    
    if (log_node == NULL) {
        DEBUG(LOG_PATH "doesn't exist, can't log");
        return;
    }
    fat_file log_file = fat_tree_get_file(log_node);
    fat_file parent = fat_tree_get_parent(log_node);
    fat_file_pwrite(log_file, log, strlen(log), log_file->dentry->file_size, parent);
}

static bool is_fs_log(fat_file file) {
    assert(file != NULL);
    return (fat_file_cmp_path(file, LOG_PATH) == 0);
}

static void format_log(char *buf, char * filepath, char *operation_type) {
    // now to str   
    time_t now = time(NULL);
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    strftime(buf, DATE_MESSAGE_SIZE, "%d-%m-%Y %H:%M", timeinfo);

    strcat(buf, "\t");
    strcat(buf, getlogin());
    strcat(buf, "\t");
    strcat(buf, filepath);
    strcat(buf, "\t");
    strcat(buf, operation_type);
    strcat(buf, "\n");
}

static void fat_fuse_log_activity(char *operation_type, fat_file file) {
    char buf[LOG_MESSAGE_SIZE] = "";
    format_log(buf, file->filepath, operation_type);
    fat_fuse_log_write(buf);
}

/* Get file attributes (file descriptor version) */
int fat_fuse_fgetattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi) {
    fat_file file = (fat_file)fat_tree_get_file((fat_tree_node)fi->fh);
    fat_file_to_stbuf(file, stbuf);
    return 0;
}

/* Get file attributes (path version) */
int fat_fuse_getattr(const char *path, struct stat *stbuf) {
    fat_volume vol;
    fat_file file;

    vol = get_fat_volume();
    file = fat_tree_search(vol->file_tree, path);
    if (file == NULL) {
        errno = ENOENT;
        return -errno;
    }
    fat_file_to_stbuf(file, stbuf);
    return 0;
}

/* Open a file */
int fat_fuse_open(const char *path, struct fuse_file_info *fi) {
    fat_volume vol;
    fat_tree_node file_node;
    fat_file file;

    vol = get_fat_volume();
    file_node = fat_tree_node_search(vol->file_tree, path);
    if (!file_node)
        return -errno;
    file = fat_tree_get_file(file_node);
    if (fat_file_is_directory(file))
        return -EISDIR;
    fat_tree_inc_num_times_opened(file_node);
    fi->fh = (uintptr_t)file_node;
    return 0;
}

/* Open a directory */
int fat_fuse_opendir(const char *path, struct fuse_file_info *fi) {
    fat_volume vol = NULL;
    fat_tree_node file_node = NULL;
    fat_file file = NULL;

    vol = get_fat_volume();
    file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL) {
        return -errno;
    }
    file = fat_tree_get_file(file_node);
    if (!fat_file_is_directory(file)) {
        return -ENOTDIR;
    }
    fat_tree_inc_num_times_opened(file_node);
    fi->fh = (uintptr_t)file_node;
    return 0;
}

/* Read directory children. Calls function fat_file_read_children which returns
 * a list of files inside a GList. The children were read from the directory
 * entries in the cluster of the directory. 
 * This function iterates over the list of children and adds them to the 
 * file tree. 
 * This operation should be performed only once per directory, the first time
 * readdir is called.
 */
static void fat_fuse_read_children(fat_tree_node dir_node) {
    fat_volume vol = get_fat_volume();
    fat_file dir = fat_tree_get_file(dir_node);
    GList *children_list = fat_file_read_children(dir);
    // Add child to tree. TODO handle duplicates
    for (GList *l = children_list; l != NULL; l = l->next) {
        vol->file_tree =
            fat_tree_insert(vol->file_tree, dir_node, (fat_file)l->data);
        
    }
    fat_fuse_init_log(vol);
    DEBUG("estoy??");
}

/* Add entries of a directory in @fi to @buf using @filler function. */
int fat_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi) {
    errno = 0;
    fat_tree_node dir_node = (fat_tree_node)fi->fh;
    fat_file dir = fat_tree_get_file(dir_node);
    fat_file *children = NULL, *child = NULL;
    int error = 0;

    // Insert first two filenames (. and ..)
    if ((*filler)(buf, ".", NULL, 0) || (*filler)(buf, "..", NULL, 0)) {
        return -errno;
    }
    if (!fat_file_is_directory(dir)) {
        errno = ENOTDIR;
        return -errno;
    }
    if (dir->children_read != 1) {
        fat_fuse_read_children(dir_node);
        if (errno < 0) {
            return -errno;
        }
    }

    children = fat_tree_flatten_h_children(dir_node);
    child = children;
    while (*child != NULL) {
        // hide fs.log from ls
        if (!is_fs_log(*child)){
            DEBUG("ENTRE");
            error = (*filler)(buf, (*child)->name, NULL, 0);
            if (error != 0) {
                return -errno;
            }
        }
        child++;
    }
    return 0;
}

/* Read data from a file */
int fat_fuse_read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    errno = 0;
    int bytes_read;
    fat_tree_node file_node = (fat_tree_node)fi->fh;
    fat_file file = fat_tree_get_file(file_node);
    fat_file parent = fat_tree_get_parent(file_node);

    fat_fuse_log_activity("read", file);
    bytes_read = fat_file_pread(file, buf, size, offset, parent);
    if (errno != 0) {
        return -errno;
    }
    
    return bytes_read;
}

/* Write data from a file */
int fat_fuse_write(const char *path, const char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    fat_tree_node file_node = (fat_tree_node)fi->fh;
    fat_file file = fat_tree_get_file(file_node);
    fat_file parent = fat_tree_get_parent(file_node);

    if (size == 0)
        return 0; // Nothing to write
    if (offset > file->dentry->file_size)
        return -EOVERFLOW;

    fat_fuse_log_activity("write", file);
    return fat_file_pwrite(file, buf, size, offset, parent);
}

/* Close a file */
int fat_fuse_release(const char *path, struct fuse_file_info *fi) {
    fat_tree_node file = (fat_tree_node)fi->fh;
    fat_tree_dec_num_times_opened(file);
    return 0;
}

/* Close a directory */
int fat_fuse_releasedir(const char *path, struct fuse_file_info *fi) {
    fat_tree_node file = (fat_tree_node)fi->fh;
    fat_tree_dec_num_times_opened(file);
    return 0;
}

int fat_fuse_mkdir(const char *path, mode_t mode) {
    errno = 0;
    fat_volume vol = NULL;
    fat_file parent = NULL, new_file = NULL;
    fat_tree_node parent_node = NULL;

    // The system has already checked the path does not exist. We get the parent
    vol = get_fat_volume();
    parent_node = fat_tree_node_search(vol->file_tree, dirname(strdup(path)));
    if (parent_node == NULL) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_file(parent_node);
    if (!fat_file_is_directory(parent)) {
        fat_error("Error! Parent is not directory\n");
        errno = ENOTDIR;
        return -errno;
    }

    // init child
    new_file = fat_file_init(vol->table, true, strdup(path));
    if (errno != 0) {
        return -errno;
    }
    // insert to directory tree representation
    vol->file_tree = fat_tree_insert(vol->file_tree, parent_node, new_file);
    // write file in parent's entry (disk)
    fat_file_dentry_add_child(parent, new_file);
    fat_file_init_dir_cluster(new_file);
    return -errno;
}

/* Creates a new file in @path. @mode and @dev are ignored. */
int fat_fuse_mknod(const char *path, mode_t mode, dev_t dev) {
    errno = 0;
    fat_volume vol;
    fat_file parent, new_file;
    fat_tree_node parent_node;

    // The system has already checked the path does not exist. We get the parent
    vol = get_fat_volume();
    parent_node = fat_tree_node_search(vol->file_tree, dirname(strdup(path)));
    if (parent_node == NULL) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_file(parent_node);
    if (!fat_file_is_directory(parent)) {
        fat_error("Error! Parent is not directory\n");
        errno = ENOTDIR;
        return -errno;
    }
    new_file = fat_file_init(vol->table, false, strdup(path));
    if (errno < 0) {
        return -errno;
    }
    // insert to directory tree representation
    vol->file_tree = fat_tree_insert(vol->file_tree, parent_node, new_file);
    // Write dentry in parent cluster
    fat_file_dentry_add_child(parent, new_file);
    return -errno;
}

int fat_fuse_utime(const char *path, struct utimbuf *buf) {
    errno = 0;
    fat_file parent = NULL;
    fat_volume vol = get_fat_volume();
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL || errno != 0) {
        errno = ENOENT;
        return -errno;
    }
    parent = fat_tree_get_parent(file_node);
    if (parent == NULL || errno != 0) {
        DEBUG("WARNING: Setting time for parent ignored");
        return 0; // We do nothing, no utime for parent
    }
    fat_utime(fat_tree_get_file(file_node), parent, buf);
    return -errno;
}

/* Shortens the file at the given offset.*/
int fat_fuse_truncate(const char *path, off_t offset) {
    errno = 0;
    fat_volume vol = get_fat_volume();
    fat_file file = NULL, parent = NULL;
    fat_tree_node file_node = fat_tree_node_search(vol->file_tree, path);
    if (file_node == NULL || errno != 0) {
        errno = ENOENT;
        return -errno;
    }
    file = fat_tree_get_file(file_node);
    if (fat_file_is_directory(file))
        return -EISDIR;

    parent = fat_tree_get_parent(file_node);
    fat_tree_inc_num_times_opened(file_node);
    fat_file_truncate(file, offset, parent);
    return -errno;
}

/* Delets a file */
int fat_fuse_unlink(const char *path){
    errno = 0;
    fat_volume vol = get_fat_volume();
    fat_tree_node f_node = fat_tree_node_search(vol->file_tree, path);

    // Check for error during execution or existance 
    if(f_node == NULL || errno != 0){
        errno = ENOENT; //Not such file or directory
        return -errno;
    }
    
    fat_file file = fat_tree_get_file(f_node);
    
    // Check if it is a directory
    if(fat_file_is_directory(file)){
        errno = EISDIR; //Illegal operation on a directory
        return -errno;
    }

    // Get parent
    fat_file f_parent = fat_tree_get_parent(f_node);
    fat_file_unlink(file, f_parent);
    fat_tree_delete(vol->file_tree, path);
    return -errno;
}

/* Removes a directory only if it is empty */
int fat_fuse_rmdir(const char *path){
    errno = 0;
    fat_volume vol = get_fat_volume();
    fat_tree_node f_node = fat_tree_node_search(vol->file_tree, path);

    // Check for error during execution or existance
    if(f_node == NULL || errno != 0){
        errno = ENOENT; //Not such file or directory
        return -errno;
    }

    fat_file direct = fat_tree_get_file(f_node);
    
    // Check if it is a directory
    if(!fat_file_is_directory(direct)){
        errno = ENOTDIR; //Not a directory
        return -errno;
    }

    // Check if the directory is empty
    GList *children = fat_file_read_children(direct);
    bool is_child_empty = g_list_length(children) == 0;
    g_list_free(children);
    if(!is_child_empty){
        errno = ENOTEMPTY; //Directory not empty
        return -errno;
    }

    // Check if it has a father guiño guiño
    fat_file f_parent = fat_tree_get_parent(f_node);
    if(f_parent == NULL){
        errno = EBUSY; //Resource busy or locked
        return -errno;
    }
    
    fat_file_unlink(direct, f_parent);
    fat_tree_delete(vol->file_tree, path);
    return -errno;
}
