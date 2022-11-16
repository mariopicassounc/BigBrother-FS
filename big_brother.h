#ifndef _BIG_BROTHER_H
#define _BIG_BROTHER_H

#define BB_LOG_FILE "/bb/fs.log"
#define BB_DIRNAME "/bb"
#define LOG_FILE_BASENAME "fs"
#define LOG_FILE_EXTENSION "log"

#include "fat_file.h"

int bb_is_log_file_dentry(fat_dir_entry dir_entry);

int bb_is_log_filepath(char *filepath);

int bb_is_log_dirpath(char *filepath);

int bb_init_log();

#endif