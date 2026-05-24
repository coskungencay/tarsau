# tarsau 📦

[cite_start]**tarsau**, Linux/Unix sistemler için C dilinde geliştirilmiş, tar/rar/zip araçlarına benzer şekilde davranan ancak sıkıştırma yapmayan hafif bir arşivleme aracıdır[cite: 6, 8]. [cite_start]Sadece ASCII metin dosyalarını tek bir `.sau` arşiv dosyasında birleştirir [cite: 9] [cite_start]ve çıkarma (extract) işlemi sırasında dosyaları orijinal isimleri ve sistem izinleriyle (chmod) birlikte geri yükler[cite: 10]. 

[cite_start]Bu proje, Sakarya Üniversitesi Sistem Programlama dersi kapsamında geliştirilmiştir[cite: 1, 2].

## 🚀 Özellikler

* [cite_start]**Güvenli ASCII Arşivleme:** Girdi dosyaları byte düzeyinde taranır; yalnızca yazdırılabilir ASCII ve temel boşluk karakterleri (0x09, 0x0A, 0x0D, 0x20-0x7E) kabul edilir[cite: 35, 36]. [cite_start]UTF-8 ve binary dosyalar anında reddedilir[cite: 38].
* [cite_start]**İzinlerin Korunması (Permission Preservation):** Dosyaların orijinal stat/chmod izinleri (örn. 755) arşive kaydedilir ve çıkarma işlemi sırasında birebir geri yüklenir[cite: 100, 101].
* **Düşük Bellek Tüketimi (Streaming Payload):** 200 MB gibi büyük boyut sınırlarına ulaşılsa bile veriler RAM'e tek seferde alınmaz; [cite_start]64 KB chunk'lar halinde streaming (akış) ile kopyalanır[cite: 67].
* [cite_start]**Defansif ve Güvenli Çıkarma:** * *Path Traversal Koruması:* Dosya adlarında `/`, `\`, `..` gibi kontrol veya dizin atlama karakterleri engellenir[cite: 45, 77].
    * [cite_start]*Boyut Tutarlılığı:* Çıkarma işleminden önce metadata, payload boyutu ve header yapıları doğrulanır[cite: 46, 78].
* [cite_start]**Atomik Yazım (Atomic Commit):** Arşivleme sırasında geçici bir dosya (temp_path) kullanılır[cite: 74]. [cite_start]Herhangi bir g/ç (I/O) hatasında bozuk dosya silinir, işlem başarılı olursa `rename()` ile atomik olarak son adını alır[cite: 75, 81].

## 🛠️ Kurulum ve Derleme

[cite_start]Proje `Makefile` kullanılarak kolayca derlenebilir[cite: 112]. [cite_start]Kaynak kod, katı `-Wall -Wextra -Werror -Wpedantic -std=c11` bayraklarıyla derlenmek üzere yapılandırılmış olup sıfır uyarı/hata politikasına sahiptir[cite: 88, 115].

```bash
# Projeyi derlemek için:
make clean && make
