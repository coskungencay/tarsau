#ifndef SAU_ERROR_H
#define SAU_ERROR_H

/*
 * Programın tüm hata kodlarını burada topladım. Her modül exit()
 * çağırmak yerine bu enumdan bir değer döner; main.c de bunu
 * process exit koduna çevirir. Böylece açık kalan dosyaları kapatmak,
 * geçici dosyaları silmek gibi temizlik işleri tek elden yürür.
 */
typedef enum {
    SAU_OK = 0,
    SAU_ERR_USAGE,            /* komut satırı argümanları hatalı            */
    SAU_ERR_INPUT_MISSING,    /* giriş dosyası açılamadı veya stat alınamadı */
    SAU_ERR_INPUT_FORMAT,     /* giriş saf 7-bit ASCII değil                */
    SAU_ERR_TOO_MANY_FILES,   /* 32'den fazla giriş dosyası verildi         */
    SAU_ERR_SIZE_LIMIT,       /* toplam giriş 200 MB sınırını aştı          */
    SAU_ERR_ARCHIVE_CORRUPT,  /* .sau dosyası eksik / kısa / bozuk          */
    SAU_ERR_IO,               /* read/write/close/rename hatası             */
    SAU_ERR_PATH_UNSAFE,      /* dosya adı güvenlik kurallarına uymuyor     */
    SAU_ERR_TARGET_DIR,       /* hedef dizin oluşturulamadı / erişilemedi   */
    SAU_ERR_MEMORY            /* heap allocation başarısız oldu             */
} SauStatus;

/*
 * Şartnamenin istediği "giriş dosyası formatı uyumsuzdur" mesajını
 * stderr'e basar. Satır sonu da içerde, çağıran tarafın eklemesine
 * gerek yok.
 */
void sau_error_report_bad_input(const char *display_name);

/*
 * "Arşiv dosyası uygunsuz veya bozuk!" mesajını stderr'e basar.
 * Extract akışında parser ne zaman tıkanırsa bunu çağırıyorum.
 */
void sau_error_report_bad_archive(void);

/*
 * Yukarıdaki iki yardımcı şartnamenin birebir istediği mesajları
 * yazar. Bunların dışında kalan, kullanım hatası veya dosya sistemi
 * problemi gibi durumlar için kendi Türkçe açıklamamı bu fonksiyondan
 * geçiriyorum.
 */
void sau_error_report_generic(const char *message);

/*
 * Kısa bir kullanım yardımı yazar. main.c, SAU_ERR_USAGE döndüğünde
 * bunu çağırıyor.
 */
void sau_error_print_usage_hint(void);

#endif /* SAU_ERROR_H */
