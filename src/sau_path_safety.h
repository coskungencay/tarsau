#ifndef SAU_PATH_SAFETY_H
#define SAU_PATH_SAFETY_H

#include <stddef.h>

#include "sau_error.h"

/*
 * Arşiv içine yazılacak basename'in maksimum uzunluğu. Renderer'ı
 * bu sınırla kapatıyorum ki içine virgül veya pipe sızdıran abartı
 * uzun adlar formatı bozamasın.
 */
#define SAU_MAX_STORED_NAME 255


SauStatus sau_path_make_stored_basename(const char *raw_path,
                                        char       *out_buffer,
                                        size_t      out_capacity);


SauStatus sau_path_reject_unsafe_member(const char *member_name);

#endif /* SAU_PATH_SAFETY_H */
