#include "sau_error.h"

#include <stdio.h>

/*
 * Şartnamenin birebir istediği iki Türkçe mesajı burada tek noktada
 * tuttum. İkisi de stderr'e yazılıyor; stdout'u başarı satırına ayırmak
 * için kasıtlı bir tercih.
 */

void sau_error_report_bad_input(const char *display_name)
{
    if (display_name == NULL) {
        display_name = "(bilinmeyen dosya)";
    }
    fprintf(stderr, "%s giriş dosyasının formatı uyumsuzdur!\n", display_name);
}

void sau_error_report_bad_archive(void)
{
    fprintf(stderr, "Arşiv dosyası uygunsuz veya bozuk!\n");
}

void sau_error_report_generic(const char *message)
{
    if (message == NULL || message[0] == '\0') {
        return;
    }
    fprintf(stderr, "%s\n", message);
}

void sau_error_print_usage_hint(void)
{
    fputs(
        "Kullanım:\n"
        "  tarsau -b dosya1 [dosya2 ... dosyaN] [-o cikti.sau]\n"
        "  tarsau -a arsiv.sau [hedef_dizin]\n",
        stderr);
}
