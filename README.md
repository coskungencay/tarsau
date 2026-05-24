# tarsau

Sıkıştırma yapmadan ASCII metin dosyalarını tek bir `.sau` arşivinde
birleştiren, daha sonra arşiv içeriğini orijinal dosya izinleriyle birlikte
geri açan bir Sistem Programlama dersi projesi. Linux/Unix üzerinde, C dili
ile yazıldı; tek bir Makefile ile derlenir.

**Öğrenciler:** Süleyman Gencay Coşkun (G231210073), İsmail Hakkı Uludağ (G231210049)
**GitHub Repository:** https://github.com/coskungencay/tarsau

## Derleme

```bash
make            # tarsau binary'sini proje köküne üretir
make clean      # binary, object ve geçici .sau dosyalarını temizler
```

Derleme bayrakları:
`-Wall -Wextra -Werror -Wpedantic -std=c11 -O2 -D_POSIX_C_SOURCE=200809L`

## Kullanım

### Arşiv oluşturma (`-b`)

```bash
./tarsau -b t1 t2 t3 t4.txt t5.dat -o s1.sau
```

- En fazla **32 giriş dosyası** kabul edilir.
- Tüm girişlerin toplam boyutu **200 MB**'ı aşamaz.
- Yalnızca ASCII metin dosyaları kabul edilir (yalnızca yatay tab `0x09`,
  satır besleme `0x0A`, satır başı `0x0D` ve yazdırılabilir ASCII `0x20`-`0x7E`).
- UTF-8 Türkçe karakter, binary byte ve diğer kontrol karakterleri reddedilir.
- Aynı basename'e sahip iki giriş yolu (örn. `dir1/t1` ve `dir2/t1`) çakışmaya
  yol açacağı için reddedilir.
- Çıktı adı `-o` ile belirtilir; verilmezse varsayılan `a.sau` üretilir.
- Başarılı çıktı: `Dosyalar birleştirildi.`
- Uyumsuz giriş için birebir: `<dosya> giriş dosyasının formatı uyumsuzdur!`

### Arşiv açma (`-a`)

```bash
./tarsau -a s1.sau d1          # d1 dizinine aç
./tarsau -a s1.sau              # geçerli dizine aç
```

- `-a` sonrasında en fazla iki parametre verilir (arşiv + opsiyonel dizin).
- Arşiv dosyası uzantısı `.sau` olmalı; aksi takdirde bozuk arşiv mesajıyla
  reddedilir.
- Hedef dizin yoksa otomatik oluşturulur, göreceli veya mutlak olabilir.
- Dosya izinleri arşivleme öncesindekiyle birebir geri yüklenir.
- Başarılı çıktı (hedef dizin verildiğinde):
  `<hedef> dizininde <liste> dosyaları açıldı.`
  Hedef belirtilmediğinde:
  `Geçerli dizinde <liste> dosyaları açıldı.`
- Dosya listesi formatı:
  - 1 dosya: `t1`
  - 2 dosya: `t1 ve t2`
  - 3+ dosya: `t1, t2 ve t3`
- Bozuk/uyumsuz arşiv için birebir: `Arşiv dosyası uygunsuz veya bozuk!`

## `.sau` dosya formatı

```
[10 byte sıfır dolgulu ASCII uzunluk][|name,perm,size|name,perm,size|...|][payload]
```

### Header (ilk 10 byte)

- Sıfır dolgulu decimal ASCII sayı.
- Sonraki organizasyon bölümünün byte uzunluğunu verir.
- Örnek: organizasyon bölümü 64 byte ise `0000000064`.
- Extract sırasında bu 10 byte tamamen rakam değilse arşiv bozuk kabul edilir.

### Organizasyon kayıtları

Kayıt grameri:

```
record   := name ',' permission ',' size
archive  := '|' record '|' record '|' ... record '|'
```

İki kayıt arasında **paylaşılan tek bir `|`** vardır. Toplam `|` sayısı her
zaman `entry_count + 1` olur.

| Alan | Açıklama |
|------|----------|
| `name` | Yalnızca güvenli basename. `/`, `\`, `..`, boş, kontrol karakteri, >255 byte reddedilir. |
| `permission` | 4 haneli octal string. Örnek: `0644`, `0755`. `stat().st_mode & 0777` değerinden üretilir. |
| `size` | Decimal byte sayısı. ASCII metin için 1 byte = 1 karakter varsayılır. |

### Payload

- Organizasyon bölümünün hemen ardından gelir.
- Hiçbir ayırıcı veya magic byte içermez.
- Her dosyanın kaç byte okunacağı metadatadaki `size` alanından çözülür.

### Bozuk arşiv yakalama

Extract sırasında aşağıdaki durumlar **arşivi reddeder**:

- Dosya 10 byte'tan kısa.
- İlk 10 byte tamamen rakam değil.
- Organizasyon uzunluğu dosya boyutunu aşıyor.
- Kayıt formatı bozuk (alan sayısı ≠ 3, izin octal değil, boyut decimal değil).
- Dosya adı path traversal/güvenlik kurallarına uymuyor.
- Header + organisation + Σpayload ≠ arşiv boyutu.
- Payload akışı erken EOF veriyor.

Filesystem'e dokunmadan önce tüm bu kontroller yapılır; reddedilen bir arşiv
kullanıcının dizininde yarım dosya bırakmaz.

## Limitler

| Limit | Değer |
|-------|-------|
| Maksimum giriş dosyası sayısı | 32 |
| Maksimum toplam payload | 200 MB (= `200 * 1024 * 1024` byte) |
| Maksimum stored basename uzunluğu | 255 byte |

200 MB sınırı sabit `SAU_MAX_TOTAL_BYTES` üzerinden uygulanır; sabit
`-DSAU_MAX_TOTAL_BYTES=...` ile compile-time override edilebilir


## Teslim

`make clean` çalıştırıldığında geriye yalnızca kaynak dosyalar, Makefile,
README, RAPOR ve samples kalır. Binary ve geçici çıktılar silinir. Teslim
klasörü zip'lenmeden önce `make clean` çağrılmalıdır.
