#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "sys.h"
#include "ed.h"
#include "fat.h"

struct desc
{
  int               fildes;
  struct fat_path  *fp;
  struct fat_file   file;
  int               flags;
};

struct file_desc
{
  struct desc       desc;
  uint32_t          pos;
};

struct dir_desc
{
  struct desc       desc;
  long              pos;
  struct dirent     dirent;
};

static _Bool            fat_ready = 0;
static struct fat       fat;
static void            *desc_list[OPEN_MAX] = {NULL};
static struct fat_path *wd = NULL;
static int              io_mode = SYS_IO_PIO;

static int read_sd(uint32_t lba, uint32_t n_block, void *buf)
{
  enum ed_error e;
  if (io_mode == SYS_IO_PIO)
    e = ed_sd_read(lba, n_block, buf);
  else if (io_mode == SYS_IO_DMA)
    e = ed_sd_read_dma(lba, n_block, buf);
  else {
    errno = EINVAL;
    return -1;
  }
  if (e != ED_ERROR_SUCCESS) {
    errno = EIO;
    return -1;
  }
  return 0;
}

static int write_sd(uint32_t lba, uint32_t n_block, void *buf)
{
  if (ed_sd_write(lba, n_block, buf) != ED_ERROR_SUCCESS) {
    errno = EIO;
    return -1;
  }
  return 0;
}

static int init_fat(void)
{
  if (fat_ready)
    return 0;
  if (ed_sd_init()) {
    errno = ENODEV;
    return -1;
  }
  if (fat_init(&fat, read_sd, write_sd, 0, 0))
    return -1;
  wd = fat_path(&fat, NULL, "", NULL);
  if (!wd)
    return -1;
  fat_ready = 1;
  return 0;
}

static struct fat_path *get_origin(const char *path, const char **tail)
{
  if (path[0] == '/' || path[0] == '\\') {
    if (tail)
      *tail = &path[1];
    return NULL;
  }
  else {
    if (tail)
      *tail = path;
    return wd;
  }
}

static int wd_find(const char *path, struct fat_entry *entry)
{
  const char *tail;
  struct fat_path *origin = get_origin(path, &tail);
  return fat_find(&fat, fat_path_target(origin), tail, entry);
}

static struct fat_path *wd_path(const char *path)
{
  const char *tail;
  struct fat_path *origin = get_origin(path, &tail);
  return fat_path(&fat, origin, tail, NULL);
}

static ino_t make_sn(struct fat_entry *entry)
{
  if ((entry->attrib & FAT_ATTRIB_DIRECTORY) && entry->clust < 2)
    return fat.part_lba;
  return fat.part_lba + 1 +
         entry->last.clust * (fat.n_clust_byte / 0x20) +
         entry->last.p_off / 0x20;
}

static int check_path(ino_t sn, struct fat_path *fp)
{
  for (struct fat_entry *p_ent = fp->ent_list.first; p_ent;
       p_ent = list_next(p_ent))
  {
    if (sn == make_sn(p_ent)) {
      errno = EACCES;
      return -1;
    }
  }
  return 0;
}

static int ent_access(struct fat_entry *entry, _Bool write)
{
  ino_t sn = make_sn(entry);
  if (write && check_path(sn, wd))
    return -1;
  for (int i = 0; i < FOPEN_MAX; ++i) {
    struct desc *desc = desc_list[i];
    if (!desc)
      continue;
    if (write && check_path(sn, desc->fp))
      return -1;
    if ((desc->flags & _FWRITE) && sn == make_sn(fat_path_target(desc->fp))) {
      errno = EACCES;
      return -1;
    }
  }
  return 0;
}

static void *new_desc(size_t size, struct fat_path *fp, int flags)
{
  for (int i = 0; i < OPEN_MAX; ++i)
    if (!desc_list[i]) {
      struct desc *desc = malloc(size);
      if (!desc) {
        errno = ENOMEM;
        return NULL;
      }
      desc->fildes = i;
      desc->fp = fp;
      fat_begin(fat_path_target(fp), &desc->file);
      desc->flags = flags;
      desc_list[i] = desc;
      return desc;
    }
  errno = EMFILE;
  return NULL;
}

static void delete_desc(int fildes)
{
  struct desc *desc = desc_list[fildes];
  fat_free(desc->fp);
  free(desc);
  desc_list[fildes] = NULL;
}

static void *get_desc(int fildes)
{
  if (fildes < 0 || fildes >= OPEN_MAX) {
    errno = EBADF;
    return NULL;
  }
  void *desc = desc_list[fildes];
  if (!desc) {
    errno = EBADF;
    return NULL;
  }
  return desc;
}

static int seek_file(struct file_desc *fdesc)
{
  struct fat_file *file = &fdesc->desc.file;
  if (fdesc->pos == file->p_off)
    return 0;
  if (fdesc->pos < file->p_off)
    fat_rewind(file);
  int e = errno;
  errno = 0;
  fat_advance(file, fdesc->pos - file->p_off, NULL);
  if (errno != 0)
    return -1;
  errno = e;
  return 0;
}

static mode_t ent_mode(struct fat_entry *entry)
{
  mode_t mode;
  if (entry->attrib & FAT_ATTRIB_DIRECTORY)
    mode = S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
  else
    mode = S_IFREG;
  if (!(entry->attrib & FAT_ATTRIB_HIDDEN))
    mode |= S_IRUSR | S_IRGRP | S_IROTH;
  if (!(entry->attrib & FAT_ATTRIB_READONLY))
    mode |= S_IWUSR;
  return mode;
}

static void ent_stat(struct fat_entry *entry, struct stat *buf)
{
  buf->st_dev = 0;
  buf->st_ino = make_sn(entry);
  buf->st_mode = ent_mode(entry);
  buf->st_nlink = 1;
  buf->st_uid = 0;
  buf->st_gid = 0;
  buf->st_size = entry->size;
  buf->st_atime = entry->atime;
  buf->st_mtime = entry->mtime;
  buf->st_ctime = entry->ctime;
  buf->st_blksize = fat.n_clust_byte;
  buf->st_blocks = (entry->size + fat.n_clust_byte - 1) / fat.n_clust_byte;
}

int open(const char *path, int oflags, ...)
{
  if (init_fat())
    return -1;
  /* find/create file */
  int flags = oflags + 1;
  const char *tail;
  struct fat_path *origin = get_origin(path, &tail);
  int e = errno;
  errno = 0;
  struct fat_path *fp = fat_path(&fat, origin, tail, NULL);
  struct fat_entry *entry;
  if (errno == 0) {
    entry = fat_path_target(fp);
    if ((oflags & O_CREAT) && (oflags & O_EXCL)) {
      errno = EEXIST;
      goto error;
    }
    if ((entry->attrib & FAT_ATTRIB_DIRECTORY) && (flags & _FWRITE)) {
      errno = EISDIR;
      goto error;
    }
    if (ent_access(entry, flags & _FWRITE))
      goto error;
    errno = e;
  }
  else {
    if (errno == ENOENT && (oflags & O_CREAT)) {
      va_list va;
      va_start(va, oflags);
      mode_t mode = va_arg(va, mode_t);
      va_end(va);
      uint8_t attrib = FAT_ATTRIB_ARCHIVE;
      if (!(mode & S_IRUSR))
        attrib |= FAT_ATTRIB_HIDDEN;
      if (!(mode & S_IWUSR))
        attrib |= FAT_ATTRIB_READONLY;
      if (fp)
        fat_free(fp);
      fp = fat_create_path(&fat, origin, tail, attrib);
      if (!fp)
        goto error;
      entry = fat_path_target(fp);
      errno = e;
    }
    else
      goto error;
  }
  /* clear file */
  if ((oflags & O_TRUNC) && entry->size > 0) {
    if (fat_resize(entry, 0, NULL))
      goto error;
  }
  /* allocate and initialize file desc */
  struct file_desc *fdesc = new_desc(sizeof(*fdesc), fp, flags);
  if (!fdesc)
    goto error;
  fdesc->pos = 0;
  return fdesc->desc.fildes;
error:
  if (fp)
    fat_free(fp);
  return -1;
}

int creat(const char *path, mode_t mode)
{
  return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int fstat(int fildes, struct stat *buf)
{
  struct desc *desc = get_desc(fildes);
  if (!desc)
    return -1;
  ent_stat(fat_path_target(desc->fp), buf);
  return 0;
}

int fstatat(int fildes, const char *path, struct stat *buf, int flag)
{
  if (init_fat())
    return -1;
  struct fat_entry *dir;
  if (fildes == AT_FDCWD)
    dir = fat_path_target(wd);
  else {
    struct desc *desc = get_desc(fildes);
    if (!desc)
      return -1;
    dir = fat_path_target(desc->fp);
  }
  struct fat_entry entry;
  if (fat_find(&fat, dir, path, &entry))
    return -1;
  ent_stat(&entry, buf);
  return 0;
}

int isatty(int fildes)
{
  if (get_desc(fildes))
    errno = ENOTTY;
  return 0;
}

off_t lseek(int fildes, off_t offset, int whence)
{
  struct file_desc *fdesc = get_desc(fildes);
  if (!fdesc)
    return -1;
  if (whence == SEEK_SET)
    fdesc->pos = offset;
  else if (whence == SEEK_CUR)
    fdesc->pos += offset;
  else if (whence == SEEK_END)
    fdesc->pos = fdesc->desc.file.size + offset;
  else {
    errno = EINVAL;
    return -1;
  }
  return fdesc->pos;
}

int close(int fildes)
{
  struct file_desc *fdesc = get_desc(fildes);
  if (!fdesc)
    return -1;
  delete_desc(fdesc->desc.fildes);
  /* flush data to disk */
  return fat_flush(&fat);
}

int read(int fildes, void *buf, unsigned int nbyte)
{
  struct file_desc *fdesc = get_desc(fildes);
  if (!fdesc)
    return -1;
  /* validate file desc */
  if (!(fdesc->desc.flags & _FREAD)) {
    errno = EBADF;
    return -1;
  }
  if (nbyte == 0)
    return 0;
  /* seek */
  if (seek_file(fdesc))
    return -1;
  /* read data and advance pointer */
  uint32_t n = fat_rw(&fdesc->desc.file, FAT_READ, buf, nbyte,
                      &fdesc->desc.file, NULL);
  fdesc->pos += n;
  return n;
}

int write(int fildes, void *buf, unsigned int nbyte)
{
  struct file_desc *fdesc = get_desc(fildes);
  if (!fdesc)
    return -1;
  struct desc *desc = &fdesc->desc;
  struct fat_entry *entry = fat_path_target(desc->fp);
  /* validate file desc */
  if (!(desc->flags & _FWRITE)) {
    errno = EBADF;
    return -1;
  }
  if (nbyte == 0)
    return 0;
  /* seek to end if FAPPEND is set */
  if (desc->flags & _FAPPEND)
    fdesc->pos = desc->file.size;
  /* write zero padding if needed */
  if (fdesc->pos > desc->file.size) {
    uint32_t size = desc->file.size;
    /* resize file */
    if (fat_resize(entry, fdesc->pos + nbyte, &desc->file))
      return -1;
    /* seek and clear */
    uint32_t adv = size - desc->file.p_off;
    if (fat_advance(&desc->file, adv, NULL) != adv)
      return -1;
    uint32_t n = fdesc->pos - size;
    if (fat_rw(&desc->file, FAT_WRITE, NULL, n, &desc->file, NULL) != n)
      return -1;
  }
  else {
    /* resize file if needed */
    uint32_t new_off = fdesc->pos + nbyte;
    if (new_off > desc->file.size) {
      if (fat_resize(entry, new_off, &desc->file))
        return -1;
    }
    /* seek */
    if (seek_file(fdesc))
      return -1;
  }
  /* write data and advance pointer */
  uint32_t n = fat_rw(&desc->file, FAT_WRITE, buf, nbyte, &desc->file, NULL);
  fdesc->pos += n;
  return n;
}

int truncate(const char *path, off_t length)
{
  if (init_fat())
    return -1;
  if (length < 0) {
    errno = EINVAL;
    return -1;
  }
  struct fat_entry entry;
  if (wd_find(path, &entry))
    return -1;
  if (entry.attrib & FAT_ATTRIB_DIRECTORY) {
    errno = EISDIR;
    return -1;
  }
  if (ent_access(&entry, 1))
    return -1;
  uint32_t size = entry.size;
  if (fat_resize(&entry, length, NULL))
    return -1;
  if (length > size) {
    struct fat_file file;
    fat_begin(&entry, &file);
    int e = errno;
    errno = 0;
    fat_advance(&file, size, NULL);
    if (errno != 0)
      return -1;
    errno = e;
    uint32_t n = length - size;
    if (fat_rw(&file, FAT_WRITE, NULL, n, NULL, NULL) != n)
      return -1;
  }
  return fat_flush(&fat);
}

int rename(const char *old_path, const char *new_path)
{
  if (init_fat())
    return -1;
  int e = errno;
  errno = 0;
  struct fat_path *fp = wd_path(old_path);
  if (errno == 0)
    ent_access(fat_path_target(fp), 1);
  int r = -1;
  if (fp) {
    if (errno == 0) {
      errno = e;
      const char *tail;
      struct fat_path *origin = get_origin(new_path, &tail);
      r = fat_rename(&fat, fp, origin, tail, NULL);
    }
    fat_free(fp);
  }
  if (r == 0)
    return fat_flush(&fat);
  else
    return r;
}

int chmod(const char *path, mode_t mode)
{
  if (init_fat())
    return -1;
  struct fat_entry entry;
  if (wd_find(path, &entry))
    return -1;
  if (ent_access(&entry, 1))
    return -1;
  uint8_t attrib = entry.attrib;
  if (mode & S_IRUSR)
    attrib &= ~FAT_ATTRIB_HIDDEN;
  else
    attrib |= FAT_ATTRIB_HIDDEN;
  if (mode & S_IWUSR)
    attrib &= ~FAT_ATTRIB_READONLY;
  else
    attrib |= FAT_ATTRIB_READONLY;
  if (fat_attrib(&entry, attrib))
    return -1;
  return fat_flush(&fat);
}

int unlink(const char *path)
{
  if (init_fat())
    return -1;
  struct fat_entry entry;
  if (wd_find(path, &entry))
    return -1;
  if (entry.attrib & FAT_ATTRIB_DIRECTORY) {
    errno = EISDIR;
    return -1;
  }
  if (ent_access(&entry, 1))
    return -1;
  if (fat_remove(&entry))
    return -1;
  return fat_flush(&fat);
}

DIR *opendir(const char *dirname)
{
  if (init_fat())
    return NULL;
  /* find directory */
  int e = errno;
  errno = 0;
  struct fat_path *fp = wd_path(dirname);
  if (errno == 0)
    errno = e;
  else
    goto error;
  struct fat_entry *entry = fat_path_target(fp);
  if (!(entry->attrib & FAT_ATTRIB_DIRECTORY)) {
    errno = ENOTDIR;
    goto error;
  }
  if (ent_access(entry, 0))
    goto error;
  /* allocate and initialize dir desc */
  struct dir_desc *ddesc = new_desc(sizeof(*ddesc), fp, _FREAD);
  if (!ddesc)
    goto error;
  ddesc->pos = 0;
  return (void*)ddesc;
error:
  if (fp)
    fat_free(fp);
  return NULL;
}

int closedir(DIR *dirp)
{
  struct dir_desc *ddesc = (void*)dirp;
  delete_desc(ddesc->desc.fildes);
  return 0;
}

struct dirent *readdir(DIR *dirp)
{
  struct dir_desc *ddesc = (void*)dirp;
  struct fat_entry entry;
  do {
    if (fat_dir(&ddesc->desc.file, &entry))
      return NULL;
  } while (entry.attrib & FAT_ATTRIB_LABEL);
  ++ddesc->pos;
  struct dirent *dirent = &ddesc->dirent;
  dirent->d_ino = make_sn(&entry);
  strcpy(dirent->d_name, entry.name);
  /* extensions */
  dirent->mode = ent_mode(&entry);
  dirent->ctime = entry.ctime;
  dirent->mtime = entry.mtime;
  dirent->size = entry.size;
  return &ddesc->dirent;
}

void seekdir(DIR *dirp, long loc)
{
  rewinddir(dirp);
  struct dir_desc *ddesc = (void*)dirp;
  for (long i = 0; i < loc; ++i) {
    struct fat_entry entry;
    do {
      if (fat_dir(&ddesc->desc.file, &entry))
        return;
    } while (entry.attrib & FAT_ATTRIB_LABEL);
    ++ddesc->pos;
  }
}

long telldir(DIR *dirp)
{
  struct dir_desc *ddesc = (void*)dirp;
  return ddesc->pos;
}

void rewinddir(DIR *dirp)
{
  struct dir_desc *ddesc = (void*)dirp;
  fat_rewind(&ddesc->desc.file);
  ddesc->pos = 0;
}

int mkdir(const char *path, mode_t mode)
{
  if (init_fat())
    return -1;
  uint8_t attrib = FAT_ATTRIB_DIRECTORY;
  if (!(mode & S_IRUSR))
    attrib |= FAT_ATTRIB_HIDDEN;
  if (!(mode & S_IWUSR))
    attrib |= FAT_ATTRIB_READONLY;
  const char *tail;
  struct fat_path *fp = get_origin(path, &tail);
  if (fat_create(&fat, fat_path_target(fp), tail, attrib, NULL))
    return -1;
  return fat_flush(&fat);
}

int rmdir(const char *path)
{
  if (init_fat())
    return -1;
  struct fat_entry entry;
  if (wd_find(path, &entry))
    return -1;
  if (!(entry.attrib & FAT_ATTRIB_DIRECTORY)) {
    errno = ENOTDIR;
    return -1;
  }
  if (ent_access(&entry, 1))
    return -1;
  if (fat_remove(&entry))
    return -1;
  return fat_flush(&fat);
}

int stat(const char *path, struct stat *buf)
{
  if (init_fat())
    return -1;
  struct fat_entry entry;
  if (wd_find(path, &entry))
    return -1;
  if (buf)
    ent_stat(&entry, buf);
  return 0;
}

int lstat(const char *path, struct stat *buf)
{
  return stat(path, buf);
}

int chdir(const char *path)
{
  if (init_fat())
    return -1;
  int e = errno;
  errno = 0;
  struct fat_path *fp = wd_path(path);
  if (errno == 0) {
    if (!(fat_path_target(fp)->attrib & FAT_ATTRIB_DIRECTORY)) {
      errno = ENOTDIR;
      goto error;
    }
    else
      errno = e;
  }
  else
    goto error;
  fat_free(wd);
  wd = fp;
  return 0;
error:
  if (fp)
    fat_free(fp);
  return -1;
}

char *getcwd(char *buf, size_t size)
{
  if (init_fat())
    return NULL;
  if (size == 0) {
    errno = EINVAL;
    return NULL;
  }
  char *p = buf;
  char *e = buf + size;
  for (struct fat_entry *p_ent = wd->ent_list.first; p_ent;
       p_ent = list_next(p_ent))
  {
    char *n = p_ent->name;
    while (*n && p != e)
      *p++ = *n++;
    if ((p_ent == wd->ent_list.first || p_ent != wd->ent_list.last) && p != e)
      *p++ = '/';
    if (p == e) {
      errno = ERANGE;
      return NULL;
    }
  }
  *p = 0;
  return buf;
}

time_t time(time_t *tloc)
{
  if (tloc)
    *tloc = 0;
  return 0;
}

int sys_io_mode(int mode)
{
  int p_mode = io_mode;
  io_mode = mode;
  return p_mode;
}

void sys_reset(void)
{
  fat_ready = 0;
  for (int i = 0; i < OPEN_MAX; ++i) {
    if (desc_list[i])
      delete_desc(i);
  }
  if (wd) {
    fat_free(wd);
    wd = 0;
  }
}
