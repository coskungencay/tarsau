#ifndef SAU_ASCII_GUARD_H
#define SAU_ASCII_GUARD_H

#include <stdio.h>

#include "sau_error.h"

typedef struct {
    long long byte_count;
    int       accepted;
} SauAsciiVerdict;

/*
 * `path` ile verilen dosyayı sabit boyutlu bir buffer ile byte byte
 * tarar. Okuma temiz biterse (doğrulama başarısız olsa bile) SAU_OK
 * döner. Open/read hatasında SAU_ERR_IO, açılamayan dosyalarda
 * SAU_ERR_INPUT_MISSING döner.
 *
 * Bu fonksiyon kullanıcıya bir mesaj basmaz; hangi Türkçe hatanın
 * yazılacağına çağıran taraf `verdict.accepted` değerine bakıp karar
 * verir.
 */
SauStatus sau_ascii_guard_scan(const char *path, SauAsciiVerdict *verdict);

#endif /* SAU_ASCII_GUARD_H */
