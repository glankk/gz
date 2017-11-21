#ifndef ADEX_H
#define ADEX_H
#include <stdint.h>
#include <vector/vector.h>

enum adex_error
{
  ADEX_ERROR_SUCCESS,
  ADEX_ERROR_MEMORY,
  ADEX_ERROR_SYNTAX,
  ADEX_ERROR_ARITHMETIC,
  ADEX_ERROR_ADDRESS,
};

struct adex
{
  struct vector expr;
};

enum adex_error adex_parse(struct adex *adex, const char *str);
enum adex_error adex_eval(struct adex *adex, uint32_t *result);
void adex_destroy(struct adex *adex);

extern const char *adex_error_name[];

#endif
