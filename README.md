# tarsau 📦

**tarsau**, Linux/Unix sistemler için C dilinde geliştirilmiş, tar/rar/zip araçlarına benzer şekilde davranan ancak sıkıştırma yapmayan hafif bir arşivleme aracıdır. Sadece ASCII metin dosyalarını tek bir `.sau` arşiv dosyasında birleştirir ve çıkarma (extract) işlemi sırasında dosyaları orijinal isimleri ve sistem izinleriyle (chmod) birlikte geri yükler. 

Bu proje, Sakarya Üniversitesi Sistem Programlama dersi kapsamında geliştirilmiştir.

## 🚀 Özellikler

**Güvenli ASCII Arşivleme:** Girdi dosyaları byte düzeyinde taranır; yalnızca yazdırılabilir ASCII ve temel boşluk karakterleri (0x09, 0x0A, 0x0D, 0x20-0x7E) kabul edilir.
 UTF-8 ve binary dosyalar anında reddedilir.**İzinlerin Korunması (Permission Preservation):** Dosyaların orijinal stat/chmod izinleri (örn. 755) arşive kaydedilir ve çıkarma işlemi sırasında birebir geri yüklenir.
**Düşük Bellek Tüketimi (Streaming Payload):** 200 MB gibi büyük boyut sınırlarına ulaşılsa bile veriler RAM'e tek seferde alınmaz;]64 KB chunk'lar halinde streaming (akış) ile kopyalanır.

**Defansif ve Güvenli Çıkarma:** * *Path Traversal Koruması:* Dosya adlarında `/`, `\`, `..` gibi kontrol veya dizin atlama karakterleri engellenir.
    *Boyut Tutarlılığı:* Çıkarma işleminden önce metadata, payload boyutu ve header yapıları doğrulanır.
**Atomik Yazım (Atomic Commit):** Arşivleme sırasında geçici bir dosya (temp_path) kullanılır.Herhangi bir g/ç (I/O) hatasında bozuk dosya silinir, işlem başarılı olursa `rename()` ile atomik olarak son adını alır.

## 🛠️ Kurulum ve Derleme

Proje `Makefile` kullanılarak kolayca derlenebilir.Kaynak kod, katı `-Wall -Wextra -Werror -Wpedantic -std=c11` bayraklarıyla derlenmek üzere yapılandırılmış olup sıfır uyarı/hata politikasına sahiptir.

# Projeyi derlemek için:
make clean && make
