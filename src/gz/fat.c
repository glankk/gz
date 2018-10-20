#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "fat.h"

/*
   string operations
*/

/* transform string to lower case */
static void cvt_lower(char *s, int length)
{
  while (length-- > 0) {
    if (*s >= 'A' && *s <= 'Z')
      *s += 'a' - 'A';
    ++s;
  }
}

/* transform string to upper case */
static void cvt_upper(char *s, int length)
{
  while (length-- > 0) {
    if (*s >= 'a' && *s <= 'z')
      *s += 'A' - 'a';
    ++s;
  }
}

/* case-insensitive compare */
static _Bool name_comp(const char *a, const char *b)
{
  while (*a && *b) {
    char ca = *a++;
    char cb = *b++;
    if (ca >= 'a' && ca <= 'z')
      ca += 'A' - 'a';
    if (cb >= 'a' && cb <= 'z')
      cb += 'A' - 'a';
    if (ca != cb)
      return 0;
  }
  return !*a && !*b;
}

/* compute the length of a string without trailing spaces and dots */
static size_t name_trim(const char *s, size_t length)
{
  while (length > 0 && (s[length - 1] == ' ' || s[length - 1] == '.'))
    --length;
  return length;
}

/* split a string into its name and extension components */
static void name_split(const char *s, const char **name, const char **ext,
                       int *name_length, int *ext_length)
{
  const char *end = s + name_trim(s, strlen(s));
  const char *dot = NULL;
  for (const char *c = end - 1; c >= s; --c)
    if (*c == '.') {
      dot = c;
      break;
    }
  *name = s;
  if (dot) {
    *name_length = dot - *name;
    *ext = dot + 1;
    *ext_length = end - *ext;
  }
  else {
    *name_length = end - *name;
    *ext = NULL;
    *ext_length = 0;
  }
}

enum sfn_case
{
  SFN_CASE_ANY,
  SFN_CASE_LOWER,
  SFN_CASE_UPPER,
};

/* check if a character is a valid sfn character in the given case */
static _Bool char_is_sfn(char c, enum sfn_case *cse)
{
  if (c >= 'A' && c <= 'Z') {
    if (cse) {
      if (*cse == SFN_CASE_LOWER)
        return 0;
      else if (*cse == SFN_CASE_ANY)
        *cse = SFN_CASE_UPPER;
    }
  }
  if (c >= 'a' && c <= 'z') {
    if (cse) {
      if (*cse == SFN_CASE_UPPER)
        return 0;
      else if (*cse == SFN_CASE_ANY)
        *cse = SFN_CASE_LOWER;
    }
    c += 'A' - 'a';
  }
  return (c >= '@' && c <= 'Z') || (c >= '0' && c <= '9') ||
         (c >= '#' && c <= ')') || (c >= '^' && c <= '`') ||
         (c >= '\x80' && c <= '\xFF') ||
         c == ' ' || c == '!' || c == '-' || c == '{' || c == '}' || c == '~';
}

/* convert an lfn name component to sfn characters */
static int cvt_sfn(const char *s, int s_length, char *buf, int buf_size)
{
  int p = 0;
  for (int i = 0; i < s_length && p < buf_size; ++i) {
    char c = s[i];
    if (c >= 'a' && c <= 'z')
      c += 'A' - 'a';
    else if (c == '.' || c == ' ')
      continue;
    else if (!char_is_sfn(c, NULL))
      c = '_';
    buf[p++] = c;
  }
  int length = p;
  while (p < buf_size)
    buf[p++] = ' ';
  return length;
}

/* check if a name is a valid sfn,
   possibly with one or two lower case components */
static _Bool name_is_sfn(const char *s, _Bool *lower_name, _Bool *lower_ext)
{
  if (strcmp(s, ".") == 0 || strcmp(s, "..") == 0) {
    if (lower_name)
      *lower_name = 0;
    if (lower_ext)
      *lower_ext = 0;
    return 1;
  }
  const char *name;
  const char *ext;
  int name_l;
  int ext_l;
  name_split(s, &name, &ext, &name_l, &ext_l);
  if (name_l == 0 || name_l > 8 || (ext && ext_l > 3))
    return 0;
  if (name[name_l - 1] == ' ' || (ext && ext[ext_l - 1] == ' '))
    return 0;
  enum sfn_case name_cse = SFN_CASE_ANY;
  enum sfn_case ext_cse = SFN_CASE_ANY;
  for (int i = 0; i < name_l; ++i) {
    if (!char_is_sfn(name[i], &name_cse))
      return 0;
  }
  for (int i = 0; i < ext_l; ++i) {
    if (!char_is_sfn(ext[i], &ext_cse))
      return 0;
  }
  if (lower_name)
    *lower_name = (name_cse == SFN_CASE_LOWER);
  if (lower_ext)
    *lower_ext = (ext_cse == SFN_CASE_LOWER);
  return 1;
}

/* translate an sfn from a directory entry to its normal form */
static int get_sfn(const char *sfn, char *buf, int *name_l, int *ext_l)
{
  int nl = 8;
  int el = 3;
  while (nl > 0 && sfn[nl - 1] == ' ')
    --nl;
  while (el > 0 && sfn[8 + el - 1] == ' ')
    --el;
  memcpy(&buf[0], &sfn[0], nl);
  int l = nl;
  if (el > 0) {
    buf[nl] = '.';
    memcpy(&buf[nl + 1], &sfn[8], el);
    l += 1 + el;
  }
  buf[l] = 0;
  if (name_l)
    *name_l = nl;
  if (ext_l)
    *ext_l = el;
  return l;
}

/* validate the name in a directory entry */
static _Bool validate_sfn(const char *sfn)
{
  /* check for dot entry */
  if (sfn[0] == '.') {
    for (int i = 2; i < 11; ++i)
      if (sfn[i] != ' ')
        return 0;
    return sfn[1] == '.' || sfn[1] == ' ';
  }
  /* validate characters */
  enum sfn_case cse = SFN_CASE_UPPER;
  for (int i = 0; i < 11; ++i)
    if (!char_is_sfn(sfn[i], &cse))
      return 0;
  return 1;
}

/* convert a name to 8.3 */
static void cvt_83(const char *s, char *sfn)
{
  memset(sfn, ' ', 11);
  if (strcmp(s, ".") == 0)
    sfn[0] = '.';
  else if (strcmp(s, "..") == 0) {
    sfn[0] = '.';
    sfn[1] = '.';
  }
  else {
    const char *name;
    const char *ext;
    int name_length;
    int ext_length;
    name_split(s, &name, &ext, &name_length, &ext_length);
    memcpy(&sfn[0], name, name_length);
    memcpy(&sfn[8], ext, ext_length);
    cvt_upper(sfn, 11);
    if (sfn[0] == '\xE5')
      sfn[0] = '\x05';
  }
}

/* compute the vfat checksum of an 8.3 name */
static uint8_t compute_lfn_checksum(const char *sfn)
{
  uint8_t *p = (void*)sfn;
  uint8_t checksum = 0;
  for (int i = 0; i < 11; ++i)
    checksum = ((checksum & 1) << 7) + (checksum >> 1) + p[i];
  return checksum;
}

/*
   time operations
*/

/* convert unix time to dos date and time */
static void unix2dos(time_t time, uint16_t *dos_date, uint16_t *dos_time)
{
  int sec = time % 60;
  time /= 60;
  int min = time % 60;
  time /= 60;
  int hr = time % 24;
  time /= 24;
  time += 719468;
  int era = (time >= 0 ? time : time - 146096) / 146097;
  int doe = time - era * 146097;
  int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int y = yoe + era * 400;
  int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  int mp = (5 * doy + 2) / 153;
  int d = doy - (153 * mp + 2) / 5 + 1;
  int m = mp + (mp < 10 ? 3 : -9);
  y += (m <= 2);
  if (y < 1980 || y > 2107) {
    if (dos_date)
      *dos_date = 0;
    if (dos_time)
      *dos_time = 0;
  }
  else {
    if (dos_date)
      *dos_date = ((uint16_t)(y - 1980) << 9) | ((uint16_t)m << 5) |
                  ((uint16_t)d << 0);
    if (dos_time)
      *dos_time = ((uint16_t)hr << 11) | ((uint16_t)min << 5) |
                  ((uint16_t)(sec / 2) << 0);
  }
}

/* convert dos date and time to unix time */
static time_t dos2unix(uint16_t dos_date, uint16_t dos_time)
{
  int y = 1980 + ((dos_date >> 9) & 0x7F);
  int m = (int)((dos_date >> 5) & 0xF);
  int d = (int)((dos_date >> 0) & 0x1F);
  int hr = (dos_time >> 11) & 0x1F;
  int min = (dos_time >> 5) & 0x3F;
  int sec = ((dos_time >> 0) & 0x1F) * 2;
  if (m < 1 || m > 12 || d < 1 || d > 31 || hr > 23 || min > 59 || sec > 59)
    return 0;
  y -= (m <= 2);
  int era = (y >= 0 ? y : y - 399) / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (era * 146097 + doe - 719468) * 86400 + hr * 3600 + min * 60 + sec;
}

/*
   cache operations
*/

static int cache_flush(struct fat *fat, int index)
{
  struct fat_cache *cache = &fat->cache[index];
  if (!cache->valid || !cache->dirty)
    return 0;
  if (fat->write(cache->lba, 1, cache->data)) {
    errno = EIO;
    return -1;
  }
  cache->dirty = 0;
  return 0;
}

static void *cache_prep(struct fat *fat, int index, uint32_t lba, _Bool load)
{
  struct fat_cache *cache = &fat->cache[index];
  if (cache->valid && cache->lba == lba)
    return cache->data;
  if (cache_flush(fat, index))
    return NULL;
  if (load && fat->read(lba, 1, cache->data)) {
    errno = EIO;
    return NULL;
  }
  cache->valid = 1;
  cache->dirty = 0;
  cache->lba = lba;
  return cache->data;
}

static void cache_dirty(struct fat *fat, int index)
{
  fat->cache[index].dirty = 1;
}

static void cache_inval(struct fat *fat, int index)
{
  fat->cache[index].valid = 0;
}

static void cache_read(struct fat *fat, int index, uint32_t offset,
                       void *dst, uint32_t length)
{
  struct fat_cache *cache = &fat->cache[index];
  if (dst)
    memcpy(dst, &cache->data[offset], length);
}

static void cache_write(struct fat *fat, int index, uint32_t offset,
                        const void *src, uint32_t length)
{
  struct fat_cache *cache = &fat->cache[index];
  if (src)
    memcpy(&cache->data[offset], src, length);
  else
    memset(&cache->data[offset], 0, length);
  cache->dirty = 1;
}

static uint32_t get_word(const void *buf, uint32_t offset, int width)
{
  const uint8_t *p = buf;
  uint32_t word = 0;
  for (int i = 0; i < width; ++i)
    word |= (uint32_t)p[offset + i] << (i * 8);
  return word;
}

static void set_word(void *buf, uint32_t offset, int width, uint32_t value)
{
  uint8_t *p = buf;
  for (int i = 0; i < width; ++i)
    p[offset + i] = value >> (i * 8);
}

/*
   cluster operations
*/

static int get_clust_fat12(struct fat *fat, uint32_t clust, uint32_t *value)
{
  uint32_t offset = clust / 2 * 3;
  uint32_t lba = offset / fat->n_sect_byte;
  offset %= fat->n_sect_byte;
  int n = 3;
  if (offset + n > fat->n_sect_byte)
    n = fat->n_sect_byte - offset;
  void *block = cache_prep(fat, FAT_CACHE_FAT, fat->fat_lba + lba, 1);
  if (!block)
    return -1;
  uint32_t group = get_word(block, offset, n);
  if (n < 3) {
    block = cache_prep(fat, FAT_CACHE_FAT, fat->fat_lba + lba + 1, 1);
    if (!block)
      return -1;
    group |= (get_word(block, 0, 3 - n) << (8 * n));
  }
  if (clust % 2 == 1)
    group >>= 12;
  *value = group & 0xFFF;
  if (*value >= 0xFF7)
    *value |= 0x0FFFF000;
  return 0;
}

static int set_clust_fat12(struct fat *fat, uint32_t clust, uint32_t value)
{
  uint32_t offset = clust / 2 * 3;
  uint32_t lba = offset / fat->n_sect_byte;
  offset %= fat->n_sect_byte;
  uint32_t mask = 0xFFF;
  value &= mask;
  if (clust % 2 == 1) {
    value <<= 12;
    mask <<= 12;
  }
  int n = 3;
  if (offset + n > fat->n_sect_byte)
    n = fat->n_sect_byte - offset;
  void *block = cache_prep(fat, FAT_CACHE_FAT, fat->fat_lba + lba, 1);
  if (!block)
    return -1;
  value |= (get_word(block, offset, n) & ~mask);
  set_word(block, offset, n, value);
  if (n < 3) {
    cache_dirty(fat, FAT_CACHE_FAT);
    block = cache_prep(fat, FAT_CACHE_FAT, fat->fat_lba + lba + 1, 1);
    if (!block)
      return -1;
    value |= ((get_word(block, 0, 3 - n) << (8 * n)) & ~mask);
    set_word(block, offset, 3 - n, value >> (8 * n));
  }
  cache_dirty(fat, FAT_CACHE_FAT);
  return 0;
}

/* get the value of a cluster entry in the FAT */
static int get_clust(struct fat *fat, uint32_t clust, uint32_t *value)
{
  if (clust >= fat->max_clust) {
    errno = EOVERFLOW;
    return -1;
  }
  if (fat->type == FAT12)
    return get_clust_fat12(fat, clust, value);
  uint32_t ent_size = fat->type == FAT16 ? 2 : 4;
  uint32_t lba = clust / (fat->n_sect_byte / ent_size);
  uint32_t offset = clust % (fat->n_sect_byte / ent_size) * ent_size;
  void *block = cache_prep(fat, FAT_CACHE_FAT, fat->fat_lba + lba, 1);
  if (!block)
    return -1;
  if (fat->type == FAT16) {
    *value = get_word(block, offset, 2);
    if (*value >= 0xFFF7)
      *value |= 0x0FFF0000;
  }
  else {
    *value = get_word(block, offset, 4);
    *value &= 0x0FFFFFFF;
  }
  return 0;
}

/* set the value of a cluster entry in the FAT */
static int set_clust(struct fat *fat, uint32_t clust, uint32_t value)
{
  if (clust >= fat->max_clust) {
    errno = EOVERFLOW;
    return -1;
  }
  if (fat->type == FAT12)
    return set_clust_fat12(fat, clust, value);
  uint32_t ent_size = fat->type == FAT16 ? 2 : 4;
  uint32_t lba = clust / (fat->n_sect_byte / ent_size);
  uint32_t offset = clust % (fat->n_sect_byte / ent_size) * ent_size;
  void *block = cache_prep(fat, FAT_CACHE_FAT, fat->fat_lba + lba, 1);
  if (!block)
    return -1;
  if (fat->type == FAT16)
    set_word(block, offset, 2, value & 0x0000FFFF);
  else
    set_word(block, offset, 4, value & 0x0FFFFFFF);
  cache_dirty(fat, FAT_CACHE_FAT);
  return 0;
}

/* get the next cluster in a cluster chain, returns 1 on success, 0 on eof,
   -1 on error */
static int advance_clust(struct fat *fat, uint32_t *clust)
{
  /* treat reserved clusters as the root directory */
  uint32_t current = *clust;
  if (current < 2) {
    if (fat->type == FAT32)
      current = fat->root_clust;
    else
      return 0;
  }
  /* get next cluster index */
  uint32_t next;
  if (get_clust(fat, current, &next))
    return -1;
  /* check for end of chain */
  if (next < 2 || next >= 0x0FFFFFF8 || next == current)
    return 0;
  *clust = next;
  return 1;
}

/* check the number of free clusters in a sequence from `clust`, up to `max` */
static uint32_t check_free_chunk_length(struct fat *fat, uint32_t clust,
                                        uint32_t max)
{
  /* treat reserved clusters as the root directory */
  if (clust < 2) {
    if (fat->type == FAT32)
      clust = fat->root_clust;
    else
      return 0;
  }
  uint32_t length = 0;
  while (length < max && clust < fat->max_clust) {
    uint32_t value;
    if (get_clust(fat, clust, &value))
      return 0;
    if (value != 0)
      break;
    ++length;
    ++clust;
  }
  return length;
}

/* find the first free cluster after `clust` (inclusive),
   preferably with at most `pref_length` free clusters chunked in a sequence.
   returns the actual chunk length in `*length` if given. */
static uint32_t find_free_clust(struct fat *fat, uint32_t clust,
                                uint32_t pref_length, uint32_t *length)
{
  if (clust < 2)
    clust = 2;
  uint32_t max_clust = 0;
  uint32_t max_length = 0;
  while (max_length < pref_length && clust < fat->max_clust) {
    uint32_t chunk_length = check_free_chunk_length(fat, clust, pref_length);
    if (chunk_length > max_length) {
      max_clust = clust;
      max_length = chunk_length;
    }
    if (chunk_length > 0)
      clust += chunk_length;
    else
      ++clust;
  }
  if (max_length == 0)
    errno = ENOSPC;
  if (length)
    *length = max_length;
  return max_clust;
}

/* link `clust` to `next_clust`, mark as end-of-chain if `eoc`. */
static int link_clust(struct fat *fat, uint32_t clust, uint32_t next_clust,
                      _Bool eoc)
{
  /* treat reserved clusters as the root directory */
  if (clust < 2) {
    if (fat->type == FAT32)
      clust = fat->root_clust;
    else {
      errno = EINVAL;
      return -1;
    }
  }
  /* link cluster */
  if (set_clust(fat, clust, next_clust))
    return -1;
  /* add end-of-chain marker */
  if (eoc) {
    if (set_clust(fat, next_clust, 0x0FFFFFFF))
      return -1;
  }
  return 0;
}

/* check if there are at least `needed` free clusters available */
static int check_free_space(struct fat *fat, uint32_t needed)
{
  uint32_t n_free = 0;
  for (uint32_t i = 2; i < fat->max_clust && n_free < needed; ++i) {
    uint32_t value;
    if (get_clust(fat, i, &value))
      return -1;
    if (value == 0x00000000)
      ++n_free;
  }
  if (n_free < needed) {
    errno = ENOSPC;
    return -1;
  }
  return 0;
}

/* resize the cluster chain that begins at `cluster` to length `n`,
   link and unlink clusters as needed. `chunk_length` is the precomputed
   number of free clusters in sequence at the end of the chain, or zero. */
static int resize_clust_chain(struct fat *fat, uint32_t clust, uint32_t n,
                              uint32_t chunk_length)
{
  /* treat reserved clusters as the root directory */
  if (clust < 2) {
    if (fat->type == FAT32)
      clust = fat->root_clust;
    else {
      errno = ENOSPC;
      return -1;
    }
  }
  /* walk cluster chain */
  _Bool eoc = 0;
  uint32_t n_alloc = 0;
  uint32_t new_clust = 0;
  for (uint32_t i = 0; i < n || !eoc; ++i) {
    uint32_t value;
    if (get_clust(fat, clust, &value))
      return -1;
    if (!eoc && (value >= 0x0FFFFFF8 || value < 2)) {
      if (set_clust(fat, clust, 0x0FFFFFFF))
        return -1;
      if (n > i + 1) {
        /* check if there's enough space left for the
           additional requested clusters */
        n_alloc = n - (i + 1);
        if (check_free_space(fat, n_alloc))
          return -1;
      }
      eoc = 1;
    }
    if (i >= n) {
      if (set_clust(fat, clust, 0x00000000))
        return -1;
      clust = value;
    }
    else if (i == n - 1) {
      if (set_clust(fat, clust, 0x0FFFFFFF))
        return -1;
      clust = value;
    }
    else if (eoc) {
      if (chunk_length == 0) {
        new_clust = find_free_clust(fat, new_clust, n_alloc, &chunk_length);
        if (new_clust == clust)
          new_clust = find_free_clust(fat, clust + 1, n_alloc, &chunk_length);
        if (new_clust == 0)
          return -1;
        n_alloc -= chunk_length;
      }
      else
        new_clust = clust + 1;
      if (link_clust(fat, clust, new_clust, 0))
        return -1;
      --chunk_length;
      clust = new_clust;
    }
    else
      clust = value;
  }
  return 0;
}

/*
   basic file operations
*/

/* prep or load the block pointed to by `file` */
static int file_sect(const struct fat_file *file, _Bool load)
{
  struct fat *fat = file->fat;
  uint32_t clust_lba;
  /* treat reserved clusters as the root directory */
  if (file->p_clust < 2) {
    if (fat->type == FAT32)
      clust_lba = fat->data_lba + (fat->root_clust - 2) * fat->n_clust_sect;
    else
      clust_lba = fat->root_lba;
  }
  else
    clust_lba = fat->data_lba + (file->p_clust - 2) * fat->n_clust_sect;
  if (!cache_prep(fat, FAT_CACHE_DATA, clust_lba + file->p_clust_sect, load))
    return -1;
  return 0;
}

/* return a pointer to the data cache at the file offset in `file` */
static void *file_data(const struct fat_file *file)
{
  return &file->fat->cache[FAT_CACHE_DATA].data[file->p_sect_off];
}

/* point `file` to the beginning of the root directory */
void fat_root(struct fat *fat, struct fat_file *file)
{
  file->fat = fat;
  file->clust = 0;
  if (fat->type == FAT32)
    file->size = 0;
  else
    file->size = fat->n_entry * 0x20;
  file->is_dir = 1;
  fat_rewind(file);
}

/* point `file` to the beginning of the file represented by `entry` */
void fat_begin(struct fat_entry *entry, struct fat_file *file)
{
  if ((entry->attrib & FAT_ATTRIB_DIRECTORY) && entry->clust < 2)
    fat_root(entry->fat, file);
  else {
    file->fat = entry->fat;
    file->clust = entry->clust;
    file->size = entry->size;
    file->is_dir = entry->attrib & FAT_ATTRIB_DIRECTORY;
    fat_rewind(file);
  }
}

/* rewind `file` to file offset 0 */
void fat_rewind(struct fat_file *file)
{
  file->p_off = 0;
  file->p_clust = file->clust;
  file->p_clust_seq = 0;
  file->p_clust_sect = 0;
  file->p_sect_off = 0;
}

/* advance a file pointer by `n_byte`, returns the number of bytes advanced.
   if eof is reached and `eof` is given, `*eof` is set to true. */
uint32_t fat_advance(struct fat_file *file, uint32_t n_byte, _Bool *eof)
{
  struct fat *fat = file->fat;
  _Bool ate = 0;
  uint32_t p_off = file->p_off;
  uint32_t old_off = p_off;
  uint32_t new_off = p_off + n_byte;
  /* do boundary check if size is known (i.e. non-zero size directory) */
  if (!(file->is_dir && file->size == 0)) {
    if (new_off > file->size || new_off < p_off) {
      new_off = file->size;
      n_byte = new_off - p_off;
      ate = 1;
    }
  }
  /* revert offset to start of current cluster */
  uint32_t p_clust_off = file->p_clust_sect * fat->n_sect_byte;
  p_clust_off += file->p_sect_off;
  p_off -= p_clust_off;
  n_byte += p_clust_off;
  /* advance through cluster chain
     (unless `file` is the FAT12/16 root directory,
     which is not in a cluster) */
  _Bool no_clust = fat->type != FAT32 && file->clust < 2;
  if (!no_clust) {
    /* compute current and target cluster sequence index */
    uint32_t clust = file->p_clust;
    uint32_t clust_seq = file->p_clust_seq;
    uint32_t new_clust_seq = new_off / fat->n_clust_byte;
    /* walk cluster chain */
    while (clust_seq < new_clust_seq) {
      int e = advance_clust(fat, &clust);
      if (e == -1) {
        if (eof)
          *eof = 0;
        return 0;
      }
      /* if the end of the cluster chain is reached,
         advance to the end of the last cluster */
      if (e == 0) {
        n_byte = fat->n_clust_byte;
        ate = 1;
        break;
      }
      p_off += fat->n_clust_byte;
      n_byte -= fat->n_clust_byte;
      ++clust_seq;
    }
    file->p_clust = clust;
    file->p_clust_seq = clust_seq;
  }
  /* advance sector and offset */
  p_off += n_byte;
  file->p_off = p_off;
  file->p_clust_sect = n_byte / fat->n_sect_byte;
  file->p_sect_off = n_byte % fat->n_sect_byte;
  /* ensure that the sector is within the cluster range */
  if (!no_clust) {
    if (file->p_clust_sect == fat->n_clust_sect) {
      --file->p_clust_sect;
      file->p_sect_off += fat->n_sect_byte;
    }
  }
  if (eof)
    *eof = ate;
  return p_off - old_off;
}

/* copy multiple clusters to or from a file and advance.
   assumes the file is pointed at the start of a cluster. */
static uint32_t clust_rw(struct fat_file *file, enum fat_rw rw, void *buf,
                         uint32_t n_clust, _Bool *eof)
{
  struct fat *fat = file->fat;
  char *p = buf;
  /* flush and invalidate data cache to prevent conflicts */
  if (cache_flush(fat, FAT_CACHE_DATA))
    return 0;
  cache_inval(fat, FAT_CACHE_DATA);
  /* treat reserved clusters as the root directory */
  uint32_t clust = file->p_clust;
  if (clust < 2)
    clust = fat->root_clust;
  /* cluster loop */
  uint32_t n_copy = 0;
  while (n_clust > 0) {
    uint32_t chunk_start = clust;
    uint32_t chunk_length = 1;
    /* compute consecutive cluster chunk length */
    while (1) {
      uint32_t p_clust = clust;
      if (get_clust(fat, clust, &clust))
        return n_copy;
      if (clust >= 0x0FFFFFF7 || clust != p_clust + 1 ||
          chunk_length >= n_clust)
      {
        break;
      }
      ++chunk_length;
    }
    /* copy chunk */
    uint32_t lba = fat->data_lba + fat->n_clust_sect * (chunk_start - 2);
    uint32_t n_block = fat->n_clust_sect * chunk_length;
    uint32_t n_byte = n_block * fat->n_sect_byte;
    int e;
    if (rw == FAT_READ)
      e = fat->read(lba, n_block, p);
    else
      e = fat->write(lba, n_block, p);
    if (e)
      break;
    n_clust -= chunk_length;
    n_copy += chunk_length;
    file->p_off += n_byte;
    file->p_clust_seq += chunk_length;
    if (clust < 2 || clust >= 0x0FFFFFF7) {
      file->p_clust_sect = fat->n_clust_sect - 1;
      file->p_sect_off = fat->n_sect_byte;
      if (eof)
        *eof = 1;
      break;
    }
    else {
      file->p_clust = clust;
      p += n_byte;
    }
  }
  return n_copy;
}

/* copy bytes to or from a file, returns the number of bytes copied.
   the updated file pointer is stored to `new_file` if given. */
uint32_t fat_rw(struct fat_file *file, enum fat_rw rw, void *buf,
                uint32_t n_byte, struct fat_file *new_file, _Bool *eof)
{
  if (n_byte == 0) {
    if (eof)
      *eof = 0;
    return 0;
  }
  struct fat *fat = file->fat;
  _Bool ate = 0;
  /* do boundary check if size is known (i.e. non-zero size directory) */
  if (!(file->is_dir && file->size == 0)) {
    if (file->p_off >= file->size) {
      if (eof)
        *eof = 1;
      return 0;
    }
    uint32_t new_off = file->p_off + n_byte;
    if (new_off > file->size || new_off < file->p_off)
      n_byte = file->size - file->p_off;
  }
  _Bool no_clust = fat->type != FAT32 && file->clust < 2;
  /* traverse file with a local copy of the file pointer */
  struct fat_file pos = *file;
  /* sector loop */
  char *p = buf;
  uint32_t n_copy = 0;
  while (n_byte > 0) {
    /* write cluster chunks if possible */
    if (!no_clust && n_byte >= fat->n_clust_byte &&
        pos.p_clust_sect == 0 && pos.p_sect_off == 0)
    {
      uint32_t n_clust = n_byte / fat->n_clust_byte;
      uint32_t n_copy_clust = clust_rw(&pos, rw, p, n_clust, &ate);
      uint32_t n_byte_clust = n_copy_clust * fat->n_clust_byte;
      if (p)
        p += n_byte_clust;
      n_byte -= n_byte_clust;
      n_copy += n_byte_clust;
      if (n_copy_clust != n_clust || ate)
        break;
      else
        continue;
    }
    /* compute chunk size */
    uint32_t chunk_size = fat->n_sect_byte - pos.p_sect_off;
    if (chunk_size > n_byte)
      chunk_size = n_byte;
    /* prep or load sector */
    uint32_t p_sect_off = pos.p_sect_off;
    if (chunk_size > 0) {
      if (file_sect(&pos, rw == FAT_READ || chunk_size != fat->n_sect_byte))
        break;
    }
    /* advance position pointer, clear errno to check for errors */
    int e = errno;
    errno = 0;
    uint32_t adv = fat_advance(&pos, chunk_size, &ate);
    /* copy chunk */
    if (adv > 0) {
      if (rw == FAT_READ)
        cache_read(fat, FAT_CACHE_DATA, p_sect_off, p, adv);
      else
        cache_write(fat, FAT_CACHE_DATA, p_sect_off, p, adv);
      if (p)
        p += adv;
      n_byte -= adv;
      n_copy += adv;
    }
    /* restore errno */
    if (errno == 0)
      errno = e;
    else
      break;
    if (adv != chunk_size || ate)
      break;
  }
  if (new_file)
    *new_file = pos;
  if (eof)
    *eof = ate;
  return n_copy;
}

/*
   directory operations
*/

/* read the next entry in a directory file */
int fat_dir(struct fat_file *dir, struct fat_entry *entry)
{
  /* sanity check */
  if (!dir->is_dir) {
    errno = ENOTDIR;
    return -1;
  }
  struct fat *fat = dir->fat;
  /* lfn state */
  int lfn_seq = -1;
  struct fat_file lfn_p;
  uint8_t lfn_checksum = 0;
  char lfn_buf[256];
  /* physical entry buffer */
  char ent_buf[0x20];
  /* next entry pointer */
  struct fat_file dir_next;
  /* entry loop */
  while (fat_rw(dir, FAT_READ, ent_buf, 0x20, &dir_next, NULL) == 0x20) {
    /* get potential special entry marker */
    uint8_t mark = get_word(ent_buf, 0x00, 1);
    /* store entry pointer advance directory file pointer */
    struct fat_file ent_p = *dir;
    *dir = dir_next;
    /* check for free entry */
    if (mark == 0x00 || mark == 0xE5) {
      lfn_seq = -1;
      continue;
    }
    /* check for lfn entry */
    uint8_t attrib = get_word(ent_buf, 0x0B, 1);
    if (attrib == 0x0F) {
      uint8_t seq = mark & 0x1F;
      /* validate sequence number */
      if (seq < 0x01 || seq > 0x14) {
        lfn_seq = -1;
        continue;
      }
      uint8_t checksum = get_word(ent_buf, 0x0D, 1);
      /* check for last lfn flag, indicating start of lfn entry chain */
      if (mark & 0x40) {
        lfn_seq = seq;
        lfn_p = ent_p;
        lfn_checksum = checksum;
        memset(lfn_buf, 0, sizeof(lfn_buf));
      }
      else {
        /* validate sequence coherency */
        if (seq != lfn_seq - 1 || checksum != lfn_checksum) {
          lfn_seq = -1;
          continue;
        }
        lfn_seq = seq;
      }
      /* read lfn part (truncate wide characters) */
      int n = (lfn_seq - 1) * 13;
      for (int j = 0; j < 13 && n < 255; ++j) {
        uint32_t p = 1 + j * 2;
        if (j >= 5)
          p += 3;
        if (j >= 11)
          p += 2;
        uint16_t c = get_word(ent_buf, p, 2);
        if (c > 0xFF)
          c = 0x7F;
        lfn_buf[n++] = c;
      }
    }
    /* handle regular entry */
    else {
      /* check for lfn */
      _Bool have_lfn = 0;
      if (lfn_seq == 1 && lfn_checksum == compute_lfn_checksum(ent_buf))
        have_lfn = 1;
      lfn_seq = -1;
      /* sanity check */
      if ((attrib & FAT_ATTRIB_DIRECTORY) && (attrib & FAT_ATTRIB_LABEL))
        continue;
      /* validate name field */
      if (!validate_sfn(&ent_buf[0]))
        continue;
      /* get sfn, check for empty name */
      int name_l;
      int ext_l;
      if (get_sfn(&ent_buf[0], entry->short_name, &name_l, &ext_l) == 0)
        continue;
      /* check for 0xE5 escape character */
      if (entry->short_name[0] == '\x05')
        entry->short_name[0] = '\xE5';
      /* copy lfn to entry name */
      if (have_lfn) {
        strcpy(entry->name, lfn_buf);
        entry->first = lfn_p;
      }
      /* use sfn as entry name if there's no lfn */
      else {
        strcpy(entry->name, entry->short_name);
        entry->first = ent_p;
        /* do case conversions */
        uint8_t cse = get_word(ent_buf, 0x0C, 1);
        if (cse & 0x08)
          cvt_lower(&entry->name[0], name_l);
        if (cse & 0x10)
          cvt_lower(&entry->name[name_l + 1], ext_l);
      }
      entry->last = ent_p;
      /* insert metadata */
      entry->ctime = dos2unix(get_word(ent_buf, 0x10, 2),
                              get_word(ent_buf, 0x0E, 2));
      entry->cms = get_word(ent_buf, 0x0D, 1) * 10;
      entry->ctime += entry->cms / 1000;
      entry->cms %= 1000;
      entry->atime = dos2unix(get_word(ent_buf, 0x12, 2), 0);
      entry->mtime = dos2unix(get_word(ent_buf, 0x18, 2),
                              get_word(ent_buf, 0x16, 2));
      entry->attrib = attrib;
      if (entry->attrib & FAT_ATTRIB_LABEL)
        entry->clust = 0;
      else {
        entry->clust = get_word(ent_buf, 0x1A, 2);
        if (fat->type == FAT32) {
          entry->clust |= get_word(ent_buf, 0x14, 2) << 16;
          /* ensure that the root cluster is always presented as 0 to ensure
             serial number consistency */
          if (entry->clust == fat->root_clust)
            entry->clust = 0;
        }
        if (entry->clust == 1)
          entry->clust = 0;
      }
      if (fat->type != FAT32 &&
          (entry->attrib & FAT_ATTRIB_DIRECTORY) && entry->clust < 2)
      {
        entry->size = fat->n_entry * 0x20;
      }
      else if (entry->attrib & (FAT_ATTRIB_DIRECTORY | FAT_ATTRIB_LABEL))
        entry->size = 0;
      else
        entry->size = get_word(ent_buf, 0x1C, 4);
      /* success */
      entry->fat = dir->fat;
      return 0;
    }
  }
  return -1;
}

/* point `file` to the start of the directory at `cluster` */
static void begin_dir(struct fat *fat, struct fat_file *file, uint32_t clust)
{
  if (clust < 2)
    fat_root(fat, file);
  else {
    file->fat = fat;
    file->clust = clust;
    file->size = 0;
    file->is_dir = 1;
    fat_rewind(file);
  }
}

/* find a directory entry by name */
static int dir_find(struct fat *fat, uint32_t clust, const char *name,
                    struct fat_entry *entry)
{
  struct fat_file pos;
  begin_dir(fat, &pos, clust);
  _Bool is_sfn = name_is_sfn(name, NULL, NULL);
  struct fat_entry ent;
  int e = errno;
  errno = 0;
  while (fat_dir(&pos, &ent) == 0) {
    if (ent.attrib & FAT_ATTRIB_LABEL)
      continue;
    _Bool match;
    if (is_sfn)
      match = name_comp(name, ent.short_name);
    else
      match = name_comp(name, ent.name);
    if (match) {
      if (entry)
        *entry = ent;
      errno = e;
      return 0;
    }
  }
  if (errno == 0)
    errno = ENOENT;
  return -1;
}

/* point an entry structure to the root directory */
static void make_root(struct fat *fat, struct fat_entry *entry)
{
  memset(entry, 0, sizeof(*entry));
  entry->fat = fat;
  entry->attrib = FAT_ATTRIB_DIRECTORY;
  if (fat->type != FAT32)
    entry->size = fat->n_entry * 0x20;
}

/* find the entry named by `path`, relative to `dir`.
   if `dir` is not given, `path` is relative to the root directory. */
int fat_find(struct fat *fat, struct fat_entry *dir, const char *path,
             struct fat_entry *entry)
{
  struct fat_entry ent;
  if (dir)
    ent = *dir;
  else
    make_root(fat, &ent);
  if (!path) {
    if (entry)
      *entry = ent;
    return 0;
  }
  /* substring loop */
  const char *p = path;
  while (*p) {
    if (!(ent.attrib & FAT_ATTRIB_DIRECTORY)) {
      errno = ENOTDIR;
      return -1;
    }
    /* extract substring */
    const char *s = p;
    const char *e = p;
    while (*p) {
      char c = *p++;
      if (c == '/' || c == '\\')
        break;
      ++e;
    }
    /* validate */
    size_t name_length = name_trim(s, e - s);
    if (name_length == 0) {
      while (s[name_length] == '.')
        ++name_length;
    }
    if (name_length > 255) {
      errno = ENAMETOOLONG;
      return -1;
    }
    if (name_length == 0)
      continue;
    /* find entry */
    char name[256];
    memcpy(name, s, name_length);
    name[name_length] = 0;
    if (strcmp(name, ".") == 0)
      continue;
    if (dir_find(fat, ent.clust, name, &ent))
      return -1;
  }
  if (entry)
    *entry = ent;
  return 0;
}

/* find the entry named by `path`, relative to the directory pointed to
   by `dir_fp`. if `dir_fp` is not given, `path` is relative to the root.
   returns a new fat_path pointing to the canonical location of the
   furthest sub-entry of `path` that can be found.
   if a sub-entry is not found and `tail` is given,
   `*tail` will point to the name of that entry in `path`. */
struct fat_path *fat_path(struct fat *fat, struct fat_path *dir_fp,
                          const char *path, const char **tail)
{
  struct fat_path *fp = malloc(sizeof(*fp));
  if (!fp) {
    errno = ENOMEM;
    return NULL;
  }
  list_init(&fp->ent_list, sizeof(struct fat_entry));
  if (dir_fp) {
    for (struct fat_entry *l_ent = dir_fp->ent_list.first; l_ent;
         l_ent = list_next(l_ent))
    {
      if (!list_push_back(&fp->ent_list, l_ent)) {
        errno = ENOMEM;
        fat_free(fp);
        return NULL;
      }
    }
  }
  else {
    struct fat_entry *ent = list_push_back(&fp->ent_list, NULL);
    if (!ent) {
      errno = ENOMEM;
      fat_free(fp);
      return NULL;
    }
    make_root(fat, ent);
  }
  struct fat_entry *ent = fp->ent_list.last;
  /* substring loop */
  const char *p = path;
  while (*p) {
    if (!(ent->attrib & FAT_ATTRIB_DIRECTORY)) {
      errno = ENOTDIR;
      break;
    }
    /* extract substring */
    const char *s = p;
    const char *e = p;
    while (*p) {
      char c = *p++;
      if (c == '/' || c == '\\')
        break;
      ++e;
    }
    /* validate */
    size_t name_length = name_trim(s, e - s);
    if (name_length == 0) {
      while (s[name_length] == '.')
        ++name_length;
    }
    if (name_length > 255) {
      errno = ENAMETOOLONG;
      break;
    }
    if (name_length == 0)
      continue;
    /* find entry */
    char name[256];
    memcpy(name, s, name_length);
    name[name_length] = 0;
    if (strcmp(name, ".") == 0)
      continue;
    struct fat_entry d_ent;
    if (dir_find(fat, ent->clust, name, &d_ent)) {
      if (errno == ENOENT && tail)
        *tail = s;
      break;
    }
    /* backtrack if entry exists in path */
    if (d_ent.attrib & FAT_ATTRIB_DIRECTORY) {
      _Bool exist = 0;
      for (struct fat_entry *l_ent = fp->ent_list.first; l_ent;
           l_ent = list_next(l_ent))
      {
        if (exist) {
          struct fat_entry *t = l_ent;
          l_ent = list_prev(l_ent);
          list_erase(&fp->ent_list, t);
        }
        else if ((l_ent->attrib & FAT_ATTRIB_DIRECTORY) &&
                 l_ent->clust == d_ent.clust)
        {
          ent = l_ent;
          exist = 1;
        }
      }
      if (exist)
        continue;
    }
    /* append to entry list in path */
    ent = list_push_back(&fp->ent_list, &d_ent);
    if (!ent) {
      errno = ENOMEM;
      break;
    }
  }
  return fp;
}

/* return the entry that a path points to */
struct fat_entry *fat_path_target(struct fat_path *fp)
{
  if (fp)
    return fp->ent_list.last;
  else
    return NULL;
}

/* return the directory entry which contains the target entry of a path */
struct fat_entry *fat_path_dir(struct fat_path *fp)
{
  return list_prev(fp->ent_list.last);
}

/* destroy and delete a path created by `fat_path` */
void fat_free(struct fat_path *ptr)
{
  list_destroy(&ptr->ent_list);
  free(ptr);
}

/* generate a free sfn from an lfn */
static int generate_sfn(struct fat *fat, uint32_t clust,
                        const char *lfn, char *sfn)
{
  const char *name;
  const char *ext;
  int name_length;
  int ext_length;
  name_split(lfn, &name, &ext, &name_length, &ext_length);
  /* shorten name components */
  int sfn_name_length = cvt_sfn(name, name_length, &sfn[0], 8);
  int sfn_ext_length = cvt_sfn(ext, ext_length, &sfn[8], 3);
  /* find a free short name */
  for (int i = 1; i < 1000000; ++i) {
    /* make discriminator */
    char sfn_disc[8];
    sprintf(sfn_disc, "%i", i);
    int sfn_disc_length = strlen(sfn_disc);
    /* make name */
    int sfn_nd_length = 7 - sfn_disc_length;
    if (sfn_nd_length > sfn_name_length)
      sfn_nd_length = sfn_name_length;
    char name_buf[13];
    sprintf(name_buf, "%.*s~%s.%.*s",
            sfn_nd_length, &sfn[0], sfn_disc, sfn_ext_length, &sfn[8]);
    /* check if exists */
    int e = errno;
    if (dir_find(fat, clust, name_buf, NULL) == 0)
      continue;
    else if (errno != ENOENT)
      return -1;
    errno = e;
    /* return name with discriminator */
    sfn[sfn_nd_length] = '~';
    memcpy(&sfn[sfn_nd_length + 1], sfn_disc, sfn_disc_length);
    return 0;
  }
  errno = EEXIST;
  return -1;
}

/* basic vfat directory entry insertion */
static int dir_insert(struct fat *fat, uint32_t dir_clust, const char *name,
                      time_t ctime, int cms, time_t atime, time_t mtime,
                      uint8_t attrib, uint32_t clust, uint32_t size,
                      struct fat_entry *entry)
{
  char lfn_ent_buf[0x20];
  char sfn_ent_buf[0x20];
  /* check name type and determine the required number of physical entries */
  int name_length = strlen(name);
  int n_pent = 1;
  {
    _Bool lower_name;
    _Bool lower_ext;
    if (name_is_sfn(name, &lower_name, &lower_ext)) {
      /* sfn characters */
      cvt_83(name, &sfn_ent_buf[0x00]);
      /* case info */
      uint8_t cse = 0x00;
      if (lower_name)
        cse |= 0x08;
      if (lower_ext)
        cse |= 0x10;
      set_word(sfn_ent_buf, 0x0C, 1, cse);
    }
    else {
      n_pent += (name_length + 12) / 13;
      /* sfn characters */
      if (generate_sfn(fat, dir_clust, name, &sfn_ent_buf[0x00]))
        return -1;
      /* case info */
      set_word(sfn_ent_buf, 0x0C, 1, 0x00);
      /* attrib */
      set_word(lfn_ent_buf, 0x0B, 1, 0x0F);
      /* type */
      set_word(lfn_ent_buf, 0x0C, 1, 0x00);
      /* checksum */
      set_word(lfn_ent_buf, 0x0D, 1, compute_lfn_checksum(&sfn_ent_buf[0x00]));
      /* cluster */
      set_word(lfn_ent_buf, 0x1A, 2, 0x0000);
    }
  }
  /* initialize search position */
  struct fat_file start;
  struct fat_file pos;
  begin_dir(fat, &pos, dir_clust);
  /* find a free entry sequence of the required length */
  for (int n_free = 0; n_free < n_pent; ) {
    /* check for eof */
    struct fat_file ent_p = pos;
    _Bool ate;
    if (fat_advance(&pos, 0x20, &ate) != 0x20) {
      if (!ate)
        return -1;
      pos = ent_p;
      /* expand directory file */
      uint32_t new_clust = find_free_clust(fat, 0, 1, NULL);
      if (new_clust == 0)
        return -1;
      if (link_clust(fat, pos.p_clust, new_clust, 1))
        return -1;
      struct fat_file clust_pos;
      begin_dir(fat, &clust_pos, new_clust);
      if (fat_rw(&clust_pos, FAT_WRITE, NULL,
                 fat->n_clust_byte, NULL, NULL) != fat->n_clust_byte)
      {
        return -1;
      }
      continue;
    }
    /* check entry */
    uint8_t mark;
    if (fat_rw(&ent_p, FAT_READ, &mark, 1, NULL, &ate) != 1) {
      if (ate)
        errno = EINVAL;
      return -1;
    }
    /* increase sequence length, or start a new sequence */
    if (mark == 0x00 || mark == 0xE5) {
      if (n_free == 0)
        start = ent_p;
      ++n_free;
    }
    /* break the current sequence if the entry is taken */
    else
      n_free = 0;
  }
  pos = start;
  /* insert lfn entries */
  for (int i = 0; i < n_pent - 1; ++i) {
    /* sequence */
    uint8_t seq = n_pent - 1 - i;
    if (i == 0)
      seq |= 0x40;
    set_word(lfn_ent_buf, 0x00, 1, seq);
    /* name characters */
    for (int j = 0; j < 13; ++j) {
      uint32_t p = 1 + j * 2;
      if (j >= 5)
        p += 3;
      if (j >= 11)
        p += 2;
      int n = 13 * (n_pent - 2 - i) + j;
      uint16_t c;
      if (n > name_length)
        c = 0xFFFF;
      else if (n == name_length)
        c = 0x0000;
      else
        c = (uint8_t)name[n];
      set_word(lfn_ent_buf, p, 2, c);
    }
    _Bool ate;
    if (fat_rw(&pos, FAT_WRITE, lfn_ent_buf, 0x20, &pos, &ate) != 0x20) {
      if (ate)
        errno = EINVAL;
      return -1;
    }
  }
  /* attribute */
  set_word(sfn_ent_buf, 0x0B, 1, attrib);
  /* ctime */
  cms += (ctime % 2) * 1000;
  ctime -= ctime % 2;
  ctime += cms / 2000;
  cms %= 2000;
  uint16_t dos_cdate;
  uint16_t dos_ctime;
  unix2dos(ctime, &dos_cdate, &dos_ctime);
  set_word(sfn_ent_buf, 0x0D, 1, cms / 10);
  set_word(sfn_ent_buf, 0x0E, 2, dos_ctime);
  set_word(sfn_ent_buf, 0x10, 2, dos_cdate);
  /* atime */
  uint16_t dos_adate;
  unix2dos(atime, &dos_adate, NULL);
  set_word(sfn_ent_buf, 0x12, 2, dos_adate);
  /* cluster */
  set_word(sfn_ent_buf, 0x14, 2, clust >> 16);
  set_word(sfn_ent_buf, 0x1A, 2, clust);
  /* mtime */
  uint16_t dos_mdate;
  uint16_t dos_mtime;
  unix2dos(ctime, &dos_mdate, &dos_mtime);
  set_word(sfn_ent_buf, 0x16, 2, dos_mtime);
  set_word(sfn_ent_buf, 0x18, 2, dos_mdate);
  /* size */
  set_word(sfn_ent_buf, 0x1C, 4, size);
  /* insert sfn entry */
  struct fat_file eot_pos;
  {
    _Bool ate;
    if (fat_rw(&pos, FAT_WRITE, sfn_ent_buf, 0x20, &eot_pos, &ate) != 0x20) {
      if (ate)
        errno = EINVAL;
      return -1;
    }
  }
  /* return entry details */
  if (entry) {
    get_sfn(&sfn_ent_buf[0], entry->short_name, NULL, NULL);
    memcpy(entry->name, name, name_length);
    entry->name[name_length] = 0;
    entry->fat = fat;
    entry->first = start;
    entry->last = pos;
    entry->ctime = dos2unix(dos_cdate, dos_ctime);
    entry->cms = cms;
    entry->atime = dos2unix(dos_adate, 0);
    entry->mtime = dos2unix(dos_mdate, dos_mtime);
    entry->attrib = attrib;
    entry->clust = clust;
    entry->size = size;
  }
  return 0;
}

/* basic vfat directory entry removal */
static int dir_remove(struct fat_entry *entry)
{
  struct fat *fat = entry->fat;
  struct fat_file pos = entry->first;
  while (pos.p_off <= entry->last.p_off) {
    if (file_sect(&pos, 1))
      return -1;
    void *data = file_data(&pos);
    set_word(data, 0x0D, 1, get_word(data, 0x00, 1));
    set_word(data, 0x00, 1, 0xE5);
    cache_dirty(fat, FAT_CACHE_DATA);
    if (pos.p_off == entry->last.p_off)
      break;
    _Bool ate;
    if (fat_advance(&pos, 0x20, &ate) != 0x20) {
      if (ate)
        errno = EINVAL;
      return -1;
    }
  }
  return 0;
}

/* create an entry named by `path`, relative to `dir`.
   if `dir` is not given, `path` is relative to the root directory.
   if `attrib` has FAT_ATTRIB_DIRECTORY, a cluster is allocated for the
   directory and dot entries are inserted.
   the entry must not exist. */
int fat_create(struct fat *fat, struct fat_entry *dir, const char *path,
               uint8_t attrib, struct fat_entry *entry)
{
  _Bool is_dir = attrib & FAT_ATTRIB_DIRECTORY;
  _Bool is_label = attrib & FAT_ATTRIB_LABEL;
  /* sanity check */
  if (is_dir && is_label) {
    errno = EINVAL;
    return -1;
  }
  /* split the path into directory path and file name */
  const char *dir_s;
  size_t dir_l;
  const char *file_s;
  size_t file_l;
  {
    const char *end = path + name_trim(path, strlen(path));
    const char *slash = NULL;
    for (const char *p = end; p >= path; --p)
      if (*p == '\\' || *p == '/') {
        slash = p;
        break;
      }
    if (slash) {
      dir_s = path;
      dir_l = slash - dir_s;
      file_s = slash + 1;
    }
    else {
      dir_s = NULL;
      dir_l = 0;
      file_s = path;
    }
    file_l = end - file_s;
    /* validate name */
    if (file_l > 255) {
      errno = ENAMETOOLONG;
      return -1;
    }
    if (file_l == 0) {
      errno = EINVAL;
      return -1;
    }
  }
  /* navigate to directory */
  struct fat_entry dir_ent;
  if (dir_l > 0) {
    char *dir_path = malloc(dir_l + 1);
    if (!dir_path) {
      errno = ENOMEM;
      return -1;
    }
    memcpy(dir_path, dir_s, dir_l);
    dir_path[dir_l] = 0;
    int e = fat_find(fat, dir, dir_path, &dir_ent);
    free(dir_path);
    if (e)
      return -1;
  }
  else {
    if (fat_find(fat, dir, NULL, &dir_ent))
      return -1;
  }
  /* check if the file exists */
  {
    int e = errno;
    if (fat_find(fat, &dir_ent, file_s, NULL) == 0) {
      errno = EEXIST;
      return -1;
    }
    if (errno != ENOENT)
      return -1;
    errno = e;
  }
  /* allocate directory cluster */
  uint32_t clust = 0;
  if (is_dir) {
    clust = find_free_clust(fat, 0, 1, NULL);
    if (clust < 2)
      return -1;
    /* end cluster chain */
    if (set_clust(fat, clust, 0x0FFFFFFF))
      return -1;
  }
  /* insert entry */
  uint32_t dir_clust = dir_ent.clust;
  time_t t = time(NULL);
  {
    char name[256];
    memcpy(name, file_s, file_l);
    name[file_l] = 0;
    if (dir_insert(fat, dir_clust, name, t, 0, t, t, attrib, clust, 0, entry))
      return -1;
  }
  if (is_dir) {
    /* clear directory cluster */
    struct fat_file clust_pos;
    begin_dir(fat, &clust_pos, clust);
    if (fat_rw(&clust_pos, FAT_WRITE, NULL,
               fat->n_clust_byte, NULL, NULL) != fat->n_clust_byte)
    {
      return -1;
    }
    /* insert dot entries */
    int d = dir_insert(fat, clust, ".", t, 0, t, t,
                       FAT_ATTRIB_DIRECTORY, clust, 0, NULL);
    int dd = dir_insert(fat, clust, "..", t, 0, t, t,
                        FAT_ATTRIB_DIRECTORY, dir_clust, 0, NULL);
    if (d || dd)
      return -1;
  }
  return 0;
}

/* same as `fat_create`, except return a path to the new entry */
struct fat_path *fat_create_path(struct fat *fat, struct fat_path *dir_fp,
                                 const char *path, uint8_t attrib)
{
  /* seek destination */
  int e = errno;
  errno = 0;
  const char *tail;
  struct fat_path *dest_fp = fat_path(fat, dir_fp, path, &tail);
  if (errno == 0) {
    errno = EEXIST;
    goto error;
  }
  else {
    if (errno == ENOENT && strlen(tail) > 0 &&
        !strchr(tail, '/') && !strchr(tail, '\\'))
    {
      errno = e;
    }
    else
      goto error;
  }
  /* create entry and insert into path */
  struct fat_entry entry;
  if (fat_create(fat, fat_path_target(dest_fp), tail, attrib, &entry))
    goto error;
  if (!list_push_back(&dest_fp->ent_list, &entry)) {
    errno = ENOMEM;
    goto error;
  }
  return dest_fp;
error:
  if (dest_fp)
    fat_free(dest_fp);
  return NULL;
}

/* resize `entry` to `size` (must be a file). `file`, if given, should
   be a valid pointer within `entry`. it will be put into a valid but
   unspecified state after the operation is completed. */
int fat_resize(struct fat_entry *entry, uint32_t size, struct fat_file *file)
{
  struct fat *fat = entry->fat;
  /* sanity check */
  if (entry->attrib & FAT_ATTRIB_DIRECTORY) {
    errno = EISDIR;
    return -1;
  }
  if (entry->attrib & FAT_ATTRIB_LABEL) {
    errno = ENOENT;
    return -1;
  }
  if (size == entry->size)
    return 0;
  /* allocate a cluster if the file is empty */
  uint32_t n_clust = (size + fat->n_clust_byte - 1) / fat->n_clust_byte;
  uint32_t clust = entry->clust;
  uint32_t chunk_length = 0;
  if (size > 0 && clust < 2) {
    clust = find_free_clust(fat, 0, n_clust, &chunk_length);
    if (clust == 0)
      return -1;
    --chunk_length;
  }
  /* resize cluster chain */
  if (clust >= 2) {
    if (resize_clust_chain(fat, clust, n_clust, chunk_length))
      return -1;
  }
  if (n_clust == 0)
    clust = 0;
  /* update entry */
  entry->clust = clust;
  entry->size = size;
  /* write entry to directory file */
  if (file_sect(&entry->last, 1))
    return -1;
  void *data = file_data(&entry->last);
  if (fat->type == FAT32)
    set_word(data, 0x14, 2, entry->clust >> 16);
  set_word(data, 0x1A, 2, entry->clust);
  set_word(data, 0x1C, 4, entry->size);
  cache_dirty(fat, FAT_CACHE_DATA);
   /* update file pointer */
  if (file) {
    file->size = entry->size;
    if (file->size == 0 || file->clust < 2) {
      file->clust = entry->clust;
      fat_rewind(file);
    }
    else if (file->p_off > size || file->p_clust_seq >= n_clust)
      fat_rewind(file);
  }
  return 0;
}

/* check if a directory is empty */
int fat_empty(struct fat *fat, struct fat_entry *dir)
{
  if (!(dir->attrib & FAT_ATTRIB_DIRECTORY)) {
    errno = ENOTDIR;
    return -1;
  }
  struct fat_file pos;
  if (dir)
    fat_begin(dir, &pos);
  else
    fat_root(fat, &pos);
  struct fat_entry ent;
  int e = errno;
  errno = 0;
  while (fat_dir(&pos, &ent) == 0) {
    if (ent.attrib & FAT_ATTRIB_LABEL)
      continue;
    if (strcmp(ent.name, ".") == 0 || strcmp(ent.name, "..") == 0)
      continue;
    errno = ENOTEMPTY;
    return -1;
  }
  if (errno != 0)
    return -1;
  errno = e;
  return 0;
}

/* check if an entry is allowed to be modified */
static int entry_mod(struct fat_entry *entry)
{
  struct fat *fat = entry->fat;
  /* check for dot entry */
  if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
    errno = EINVAL;
    return -1;
  }
  /* check for root directory */
  if ((entry->attrib & FAT_ATTRIB_DIRECTORY) &&
      (entry->clust < 2 ||
       (fat->type == FAT32 && entry->clust == fat->root_clust)))
  {
    errno = EINVAL;
    return -1;
  }
  return 0;
}

/* rename/move a directory entry. canonical paths must be given for the
   pertinent entry and the directory to which `path` is relative.
   `dir_fp` can be null for root.
   `entry_fp` will be invalidated on success and should not be used.
   the new entry is stored in `*new_entry`, if given. */
int fat_rename(struct fat *fat, struct fat_path *entry_fp,
               struct fat_path *dir_fp, const char *path,
               struct fat_entry *new_entry)
{
  struct fat_entry *entry = fat_path_target(entry_fp);
  /* sanity check */
  if (entry_mod(entry))
    return -1;
  /* seek destination */
  int e = errno;
  errno = 0;
  const char *tail;
  struct fat_path *dest_fp = fat_path(fat, dir_fp, path, &tail);
  if (errno == 0) {
    /* check for no-op */
    struct fat_entry *dest_entry = fat_path_target(dest_fp);
    if (dest_entry->last.clust == entry->last.clust &&
        dest_entry->last.p_off == entry->last.p_off)
    {
      errno = e;
      goto exit;
    }
    else {
      errno = EEXIST;
      goto error;
    }
  }
  else {
    if (errno == ENOENT && strlen(tail) > 0 &&
        !strchr(tail, '/') && !strchr(tail, '\\'))
    {
      errno = e;
    }
    else
      goto error;
  }
  /* check for directory recursion */
  if (entry->attrib & FAT_ATTRIB_DIRECTORY) {
    for (struct fat_entry *l_ent = dest_fp->ent_list.first; l_ent;
         l_ent = list_next(l_ent))
    {
      if ((l_ent->attrib & FAT_ATTRIB_DIRECTORY) &&
          l_ent->clust == entry->clust)
      {
        errno = EINVAL;
        goto error;
      }
    }
  }
  /* validate name */
  size_t name_l = name_trim(tail, strlen(tail));
  if (name_l == 0) {
    errno = EINVAL;
    goto error;
  }
  if (name_l > 255) {
    errno = ENAMETOOLONG;
    goto error;
  }
  char name[256];
  memcpy(name, tail, name_l);
  name[name_l] = 0;
  /* insert new entry, remove old entry */
  if (dir_insert(fat, fat_path_target(dest_fp)->clust, name,
                 entry->ctime, entry->cms, entry->atime, entry->mtime,
                 entry->attrib, entry->clust, entry->size, new_entry) ||
      dir_remove(entry))
  {
    goto error;
  }
exit:
  fat_free(dest_fp);
  return 0;
error:
  if (dest_fp)
    fat_free(dest_fp);
  return -1;
}

/* remove a directory entry */
int fat_remove(struct fat_entry *entry)
{
  struct fat *fat = entry->fat;
  /* sanity check */
  if (entry_mod(entry))
    return -1;
  /* do not remove non-empty directories */
  if ((entry->attrib & FAT_ATTRIB_DIRECTORY) && fat_empty(fat, entry))
    return -1;
  /* free cluster chain */
  if (entry->clust >= 2) {
    if (resize_clust_chain(fat, entry->clust, 0, 0))
      return -1;
  }
  /* remove physical entries */
  if (dir_remove(entry))
    return -1;
  return 0;
}

/* modify the attribute of an entry */
int fat_attrib(struct fat_entry *entry, uint8_t attrib)
{
  struct fat *fat = entry->fat;
  /* sanity check */
  if (entry_mod(entry))
    return -1;
  uint8_t e_type = entry->attrib & (FAT_ATTRIB_DIRECTORY | FAT_ATTRIB_LABEL);
  uint8_t a_type = attrib & (FAT_ATTRIB_DIRECTORY | FAT_ATTRIB_LABEL);
  if (a_type != e_type) {
    errno = EINVAL;
    return -1;
  }
  /* edit attribute */
  if (file_sect(&entry->last, 1))
    return -1;
  void *data = file_data(&entry->last);
  if (!data)
    return -1;
  entry->attrib = attrib;
  set_word(data, 0x0B, 1, attrib);
  cache_dirty(fat, FAT_CACHE_DATA);
  return 0;
}

/* update the access time of an entry */
int fat_atime(struct fat_entry *entry, time_t timeval)
{
  struct fat *fat = entry->fat;
  /* sanity check */
  if (entry_mod(entry))
    return -1;
  if (file_sect(&entry->last, 1))
    return -1;
  void *data = file_data(&entry->last);
  if (!data)
    return -1;
  uint16_t dos_adate;
  entry->atime = timeval;
  unix2dos(entry->atime, &dos_adate, NULL);
  set_word(data, 0x12, 2, dos_adate);
  cache_dirty(fat, FAT_CACHE_DATA);
  return 0;
}

/* update the modified time of an entry */
int fat_mtime(struct fat_entry *entry, time_t timeval)
{
  struct fat *fat = entry->fat;
  /* sanity check */
  if (entry_mod(entry))
    return -1;
  if (file_sect(&entry->last, 1))
    return -1;
  void *data = file_data(&entry->last);
  if (!data)
    return -1;
  uint16_t dos_mdate;
  uint16_t dos_mtime;
  entry->mtime = timeval;
  unix2dos(entry->mtime, &dos_mdate, &dos_mtime);
  set_word(data, 0x16, 2, dos_mtime);
  set_word(data, 0x18, 2, dos_mdate);
  cache_dirty(fat, FAT_CACHE_DATA);
  return 0;
}

/*
   file system operations
*/

static int check_rec(struct fat *fat, uint32_t rec_lba, int part)
{
  if (part >= 4) {
    errno = ENOENT;
    return -1;
  }
  /* load partition record */
  void *pr = cache_prep(fat, FAT_CACHE_DATA, rec_lba, 1);
  if (!pr)
    return -1;
  /* check signature */
  if (get_word(pr, 0x1FE, 2) != 0xAA55) {
    errno = ENOENT;
    return -1;
  }
  /* check partition type */
  int part_entry = 0x1BE + 0x10 * part;
  uint8_t part_type = get_word(pr, part_entry + 0x4, 1);
  if (part_type != 0x01 && part_type != 0x04 && part_type != 0x06 &&
      part_type != 0x0E && part_type != 0x0B && part_type != 0x0C)
  {
    errno = ENOENT;
    return -1;
  }
  /* get partition address */
  fat->part_lba = get_word(pr, part_entry + 0x8, 4);
  fat->n_part_sect = get_word(pr, part_entry + 0xC, 4);
  if (fat->part_lba == 0 || fat->part_lba == rec_lba ||
      fat->n_part_sect == 0)
  {
    errno = ENOENT;
    return -1;
  }
  return 0;
}

int fat_init(struct fat *fat, fat_io_proc read, fat_io_proc write,
             uint32_t rec_lba, int part)
{
  /* initialize cache */
  fat->read = read;
  fat->write = write;
  for (int i = 0; i < FAT_CACHE_MAX; ++i)
    fat->cache[i].valid = 0;
  /* check partition record for compatible partition */
  if (check_rec(fat, rec_lba, part))
    return -1;
  /* load partition boot record */
  void *pbr = cache_prep(fat, FAT_CACHE_DATA, fat->part_lba, 1);
  if (!pbr)
    return -1;
  /* get file system geometry info */
  fat->n_sect_byte = get_word(pbr, 0x00B, 2);
  fat->n_clust_sect = get_word(pbr, 0x00D, 1);
  fat->n_resv_sect = get_word(pbr, 0x00E, 2);
  fat->n_fat = get_word(pbr, 0x010, 1);
  fat->n_entry = get_word(pbr, 0x011, 2);
  fat->n_fs_sect = get_word(pbr, 0x013, 2);
  if (fat->n_fs_sect == 0)
    fat->n_fs_sect = get_word(pbr, 0x020, 4);
  if (fat->n_fs_sect == 0)
    fat->n_fs_sect = fat->n_part_sect;
  fat->n_fat_sect = get_word(pbr, 0x016, 2);
  if (fat->n_fat_sect == 0)
    fat->n_fat_sect = get_word(pbr, 0x024, 4);
  /* do sanity checks */
  if (fat->n_sect_byte != 0x200 || fat->n_clust_sect == 0 ||
      fat->n_resv_sect == 0 || fat->n_fat == 0 ||
      fat->n_fs_sect > fat->n_part_sect || fat->n_fat_sect == 0)
  {
    errno = ENOENT;
    return -1;
  }
  /* compute addresses and limits */
  fat->fat_lba = fat->part_lba + fat->n_resv_sect;
  fat->root_lba = fat->fat_lba + fat->n_fat * fat->n_fat_sect;
  fat->data_lba = fat->root_lba + (fat->n_entry * 0x20 +
                                   fat->n_sect_byte - 1) / fat->n_sect_byte;
  /* more sanity checks */
  uint32_t max_lba = fat->part_lba + fat->n_part_sect;
  if (max_lba < fat->part_lba ||
      fat->fat_lba < fat->part_lba || fat->fat_lba >= max_lba ||
      fat->root_lba < fat->part_lba || fat->root_lba >= max_lba ||
      fat->data_lba < fat->part_lba || fat->data_lba >= max_lba)
  {
    errno = ENOENT;
    return -1;
  }
  fat->n_clust_byte = fat->n_clust_sect * fat->n_sect_byte;
  fat->max_clust = 2 + (fat->n_fs_sect - fat->data_lba) / fat->n_clust_sect;
  if (fat->max_clust < 0xFF7)
    fat->type = FAT12;
  else if (fat->max_clust < 0xFFF7)
    fat->type = FAT16;
  else {
    fat->type = FAT32;
    if (fat->max_clust > 0x0FFFFFF7)
      fat->max_clust = 0x0FFFFFF7;
  }
  uint32_t n_fat_clust = fat->n_fat_sect * fat->n_sect_byte;
  if (fat->type == FAT12)
    n_fat_clust = n_fat_clust / 3 * 2;
  else if (fat->type == FAT16)
    n_fat_clust /= 2;
  else
    n_fat_clust /= 4;
  if (fat->max_clust > n_fat_clust)
    fat->max_clust = n_fat_clust;
  /* get fat32 info */
  if (fat->type == FAT32) {
    fat->root_clust = get_word(pbr, 0x02C, 4);
    fat->fsis_lba = get_word(pbr, 0x030, 2);
  }
  else {
    fat->root_clust = 0;
    fat->fsis_lba = 0;
  }
  /* even more sanity checks */
  if ((fat->type != FAT32 && fat->n_entry == 0) ||
      (fat->type == FAT32 && fat->root_clust < 2))
  {
    errno = ENOENT;
    return -1;
  }
  return 0;
}

int fat_flush(struct fat *fat)
{
  for (int i = 0; i < FAT_CACHE_MAX; ++i) {
    if (cache_flush(fat, i))
      return -1;
  }
  return 0;
}
