#ifndef FAT_H
#define FAT_H
#include <stdint.h>
#include <time.h>
#include <list/list.h>

#define FAT_MAX_CACHE_SECT    4

#define FAT_CACHE_FAT         0
#define FAT_CACHE_DATA        1
#define FAT_CACHE_MAX         2

#define FAT_ATTRIB_DEFAULT    0x00
#define FAT_ATTRIB_READONLY   0x01
#define FAT_ATTRIB_HIDDEN     0x02
#define FAT_ATTRIB_SYSTEM     0x04
#define FAT_ATTRIB_LABEL      0x08
#define FAT_ATTRIB_DIRECTORY  0x10
#define FAT_ATTRIB_ARCHIVE    0x20
#define FAT_ATTRIB_DEVICE     0x40

enum fat_type
{
  FAT12,
  FAT16,
  FAT32,
};

enum fat_rw
{
  FAT_READ,
  FAT_WRITE,
};

/* block cache */
struct fat_cache
{
  _Bool     valid;
  _Bool     dirty;
  uint32_t  max_lba;
  uint32_t  load_lba;
  uint32_t  prep_lba;
  int       n_sect;
  _Alignas(0x10)
  char      data[0x200 * FAT_MAX_CACHE_SECT];
};

typedef int (*fat_rd_proc)(size_t lba, size_t n_block, void *buf);
typedef int (*fat_wr_proc)(size_t lba, size_t n_block, const void *buf);

/* fat context */
struct fat
{
  /* block io interface */
  fat_rd_proc       read;
  fat_wr_proc       write;
  /* file system info */
  enum fat_type     type;
  uint32_t          part_lba;
  uint32_t          n_part_sect;
  uint16_t          n_sect_byte;
  uint8_t           n_clust_sect;
  uint16_t          n_resv_sect;
  uint8_t           n_fat;
  uint16_t          n_entry;
  uint32_t          n_fs_sect;
  uint32_t          n_fat_sect;
  uint32_t          root_clust;
  uint16_t          fsis_lba;
  uint32_t          fat_lba;
  uint32_t          root_lba;
  uint32_t          data_lba;
  uint32_t          n_clust_byte;
  uint32_t          max_clust;
  uint32_t          free_lb;
  /* cache */
  struct fat_cache  cache[FAT_CACHE_MAX];
};

/* file pointer */
struct fat_file
{
  struct fat       *fat;
  /* file info */
  uint32_t          clust;
  uint32_t          size;
  _Bool             is_dir;
  /* file offset */
  uint32_t          p_off;
  /* file system geometry pointers */
  uint32_t          p_clust;
  uint32_t          p_clust_seq;
  uint32_t          p_clust_sect;
  uint32_t          p_sect_off;
};

/* canonical path */
struct fat_path
{
  struct list       ent_list;
};

/* directory entry */
struct fat_entry
{
  struct fat       *fat;
  /* pointer to first physical entry (sfn entry or start of lfn chain) */
  struct fat_file   first;
  /* pointer to last physical entry (sfn entry) */
  struct fat_file   last;
  /* sfn */
  char              short_name[13];
  /* lfn or case-adjusted sfn */
  char              name[256];
  /* metadata */
  time_t            ctime;
  int               cms;
  time_t            atime;
  time_t            mtime;
  uint8_t           attrib;
  uint32_t          clust;
  uint32_t          size;
};

void              fat_root(struct fat *fat, struct fat_file *file);
void              fat_begin(struct fat_entry *entry, struct fat_file *file);
void              fat_rewind(struct fat_file *file);
uint32_t          fat_advance(struct fat_file *file, uint32_t n_byte,
                              _Bool *eof);
uint32_t          fat_rw(struct fat_file *file, enum fat_rw rw, void *buf,
                         uint32_t n_byte, struct fat_file *new_file,
                         _Bool *eof);
int               fat_dir(struct fat_file *dir, struct fat_entry *entry);
int               fat_find(struct fat *fat, struct fat_entry *dir,
                           const char *path, struct fat_entry *entry);
struct fat_path  *fat_path(struct fat *fat, struct fat_path *dir_fp,
                           const char *path, const char **tail);
struct fat_entry *fat_path_target(struct fat_path *fp);
struct fat_entry *fat_path_dir(struct fat_path *fp);
void              fat_free(struct fat_path *ptr);
int               fat_create(struct fat *fat, struct fat_entry *dir,
                             const char *path, uint8_t attrib,
                             struct fat_entry *entry);
struct fat_path  *fat_create_path(struct fat *fat, struct fat_path *dir_fp,
                                  const char *path, uint8_t attrib);
int               fat_resize(struct fat_entry *entry, uint32_t size,
                             struct fat_file *file);
int               fat_empty(struct fat *fat, struct fat_entry *dir);
int               fat_rename(struct fat *fat, struct fat_path *entry_fp,
                             struct fat_path *dir_fp, const char *path,
                             struct fat_entry *new_entry);
int               fat_remove(struct fat_entry *entry);
int               fat_attrib(struct fat_entry *entry, uint8_t attrib);
int               fat_atime(struct fat_entry *entry, time_t timeval);
int               fat_mtime(struct fat_entry *entry, time_t timeval);
int               fat_init(struct fat *fat, fat_rd_proc read,
                           fat_wr_proc write, uint32_t rec_lba, int part);
int               fat_flush(struct fat *fat);

#endif
