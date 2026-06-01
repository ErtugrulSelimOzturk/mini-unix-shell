# Mini Unix Shell Proje Raporu

## Proje Bilgileri

**Proje Adı:** Mini Unix Shell

**Ders/Konu:** Sistem Programlama

**Proje Üyesi:** Ertuğrul Selim Öztürk

**Geliştirme Ortamı:** WSL/Linux, C dili, POSIX/Linux API

**Kaynak Dosyalar:**
- `shell.c`
- `Makefile`
- `README.md`
- `shell.log`

## Projenin Amacı

Bu projenin amacı, C dili ve POSIX/Linux sistem çağrıları kullanılarak temel bir Unix shell uygulaması geliştirmektir. Shell kullanıcıdan komut alır, komutları çalıştırır, foreground/background süreçlerini yönetir, pipe desteği sağlar ve komut geçmişini tutar.

Proje kapsamında özellikle süreç yönetimi, sistem çağrıları, pipe kullanımı, komut ayrıştırma, hata yönetimi, loglama ve senkronizasyon kavramları uygulanmıştır.

## Projenin Genel Çalışma Mantığı

Program klasik bir **Read-Parse-Execute** döngüsü ile çalışır:

1. Kullanıcıdan komut satırı okunur.
2. Komut ve argümanlar ayrıştırılır.
3. Komut built-in ise shell içinde çalıştırılır.
4. Komut dış komut ise `fork()` ile çocuk süreç oluşturulur.
5. Çocuk süreçte `execvp()` ile komut çalıştırılır.
6. Foreground komutlarda ana süreç `waitpid()` ile çocuğun bitmesini bekler.
7. Background komutlarda shell beklemeden yeni komut almaya devam eder.

[Görüntü 1: Programın ilk açılış ekranı]

Bu görüntüde `./shell` komutu ile programın başlatılması, başlık ekranı ve `help` listesinin otomatik olarak gösterilmesi anlatılacak.

## Kullanılan Sistem Programlama Kavramları

### Process Management

Programda normal komutlar ve pipe içindeki komutlar için `fork()` kullanılarak çocuk süreçler oluşturulur. Çocuk süreçlerde `execvp()` çağrısı ile kullanıcı komutları çalıştırılır.

Kullanılan örnek sistem çağrıları:

```c
fork();
execvp(args[0], args);
waitpid(pid, &child_status, 0);
```

[Görüntü 2: `fork`, `execvp`, `waitpid` kullanılan kod bölümü]

Bu görüntüde `shell.c` içinde normal komut çalıştırma bölümündeki process yönetimi gösterilecek.

### Pipe Kullanımı

Tek seviyeli pipe desteği vardır. Örneğin:

```bash
ls | grep txt
```

Bu komutta `ls` çıktısı pipe ile `grep txt` komutuna aktarılır.

Programda `pipe()` ile pipe oluşturulur, `dup2()` ile standart giriş/çıkış yönlendirilir.

[Görüntü 3: Pipe komutunun çalıştırılması]

Bu görüntüde önce bir dosya oluşturulup sonra pipe komutu gösterilecek:

```bash
touch deneme.txt
ls | grep txt
```

Beklenen çıktı:

```text
deneme.txt
```

### Background Process

Komutun sonuna `&` eklenirse komut arka planda çalışır:

```bash
sleep 10 &
```

Shell bu komutu başlatır ve beklemeden yeni komut alır. Arka plan süreçleri `jobs` komutu ile izlenebilir.

[Görüntü 4: Background process ve jobs kullanımı]

Bu görüntüde şu komutlar gösterilecek:

```bash
sleep 10 &
jobs
```

### Senkronizasyon

Arka plan süreçlerinin tutulduğu ortak liste `pthread_mutex_t` ile korunur. Bu sayede background job listesine ekleme, silme ve listeleme işlemleri kontrollü yapılır.

Kullanılan senkronizasyon mekanizması:

```c
pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_lock(&jobs_mutex);
pthread_mutex_unlock(&jobs_mutex);
```

[Görüntü 5: Mutex kullanılan kod bölümü]

Bu görüntüde `add_job`, `remove_job` veya `show_jobs` fonksiyonundaki mutex kullanımı gösterilecek.

## Built-in Komutlar

Projede shell içinde çalışan built-in komutlar şunlardır:

| Komut | Açıklama | Örnek |
| --- | --- | --- |
| `cd <dizin>` | Dizin değiştirir | `cd test` |
| `pwd` | Bulunulan dizini gösterir | `pwd` |
| `exit` | Shell'den çıkar | `exit` |
| `help` | Komut listesini gösterir | `help` |
| `help <komut>` | Komuta özel yardım gösterir | `help pipe` |
| `history` | Son 10 komutu gösterir | `history` |
| `history clear` | Komut geçmişini temizler | `history clear` |
| `jobs` | Aktif ve geçmiş background süreçleri gösterir | `jobs` |
| `killjob <no>` | Aktif background sürecini sonlandırır | `killjob 1` |
| `fg <no>` | Background süreci foreground olarak bekler | `fg 1` |
| `clear` | Ekranı temizler | `clear` |
| `log` | Log kayıtlarını gösterir | `log` |
| `log <sayi>` | Son N log kaydını gösterir | `log 5` |
| `log clear` | Log dosyasını temizler | `log clear` |

[Görüntü 6: Help ekranı]

Bu görüntüde `help` komutu çalıştırılıp komutların açıklamaları ve örnekleri gösterilecek.

## Dış Komutlar ve Dosya İşlemleri

Shell, sistemde bulunan dış komutları da `execvp()` ile çalıştırır. Örneğin:

```bash
ls
mkdir test
rmdir test
touch not.txt
cat not.txt
rm not.txt
nano not.txt
```

Bu komutlar built-in değildir; Linux komutları olarak çocuk süreçte çalıştırılır.

[Görüntü 7: Dosya ve dizin komutları]

Bu görüntüde `mkdir`, `touch`, `cat`, `rm` gibi komutların shell içinde çalıştırıldığı gösterilecek.

## Yönlendirme Desteği

Projede basit giriş/çıkış yönlendirme desteği vardır.

Çıktıyı dosyaya yazma:

```bash
echo merhaba > out.txt
```

Dosyadan girdi okuma:

```bash
cat < out.txt
```

[Görüntü 8: Yönlendirme kullanımı]

Bu görüntüde `echo merhaba > out.txt` ve `cat < out.txt` komutlarının çıktısı gösterilecek.

## Hata Yönetimi

Hatalı komut girildiğinde shell kapanmaz. Kullanıcıya hata mesajı verilir ve yeni komut almaya devam eder.

Örnek:

```bash
olmayan_komut
```

Beklenen çıktı:

```text
shell: execution error: No such file or directory
```

Pipe hataları için de özel hata mesajı vardır:

```bash
ls |
```

Beklenen çıktı:

```text
shell: pipe kullanimi hatali. Ornek: komut1 | komut2
```

[Görüntü 9: Hatalı komut ve hatalı pipe kullanımı]

Bu görüntüde shell'in kapanmadan hata mesajı verdiği gösterilecek.

## History Yapısı

Program son 10 komutu bellekte tutar. `history` komutu ile bu komutlar listelenir. Komut sayısı 10'u aşarsa en eski komut silinir.

Örnek:

```bash
history
```

Geçmiş temizleme:

```bash
history clear
```

[Görüntü 10: History komutu]

Bu görüntüde `history` komutu ile son komutların listelenmesi gösterilecek.

## Loglama

Tüm komutlar `shell.log` dosyasına kaydedilir. Log satırlarında komut adı, çalışma süresi, çalışma tipi ve durum bilgisi bulunur.

Örnek log satırı:

```text
[Mon Jun  1 14:00:00 2026] CMD: ls | Sure: 2.3570 ms | Tip: On Plan | Durum: Tamamlandi
```

Log alanları:

- `CMD`: Çalıştırılan komut
- `Sure`: Komutun çalışma süresi
- `Tip`: Komutun foreground veya background çalıştığını belirtir
- `Durum`: Komutun başarılı, hatalı veya başlatılmış olduğunu gösterir

Log görüntüleme:

```bash
log
log 5
```

Log temizleme:

```bash
log clear
```

[Görüntü 11: Log kayıtları]

Bu görüntüde `log 5` komutu ile son log kayıtları gösterilecek.

## Performans Değerlendirmesi

Foreground komutlarda başlangıç ve bitiş zamanı `gettimeofday()` ile ölçülür. Süre milisaniye cinsinden log dosyasına yazılır.

Örneğin:

```bash
sleep 2
```

Bu komut yaklaşık 2000 ms sürer. Buna karşılık:

```bash
sleep 10 &
```

arka planda başlatıldığı için shell beklemez ve logda `Tip: Arka Plan` olarak görünür.

Bu yapı ile foreground komutların shell'i beklettiği, background komutların ise shell'i bloklamadığı gözlemlenmiştir.

[Görüntü 12: Performans ve süre ölçümü]

Bu görüntüde `sleep 2` ve `sleep 10 &` komutlarının log kayıtları karşılaştırılacak.

## Teknik Bileşenler ve Kod Karşılıkları

Bu bölümde projede kullanılan temel sistem programlama bileşenleri ve bu bileşenlerin kod içindeki karşılıkları özetlenmiştir.

| Teknik Bileşen | Projedeki Karşılığı |
| --- | --- |
| C ve POSIX/Linux API kullanımı | Proje C ile yazılmıştır; `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `chdir`, `getcwd`, `open` gibi POSIX/Linux API çağrıları kullanılmıştır. |
| Process veya thread kullanımı | Dış komutlar ve pipe komutları için `fork()` ile çocuk süreçler oluşturulur. |
| Senkronizasyon mekanizması | Background süreç listesini korumak için `pthread_mutex_t jobs_mutex` kullanılır. |
| Loglama | Komutlar `shell.log` dosyasına komut adı, süre, tip ve durum bilgisiyle yazılır. |
| Hata yönetimi | `perror()` ve özel hata mesajları ile hatalı komut, hatalı pipe ve yönlendirme hataları kullanıcıya bildirilir. |
| Performans değerlendirmesi | Foreground komutların çalışma süresi `gettimeofday()` ile ölçülür ve milisaniye cinsinden loglanır. |
| Komut satırı okuma | Kullanıcı girdisi `readline()` ile alınır. |
| `fork`, `exec`, `wait/waitpid` | Komut çalıştırmada `fork()`, `execvp()` ve `waitpid()` birlikte kullanılır. |
| Tek seviyeli pipe desteği | `execute_pipeline()` içinde `pipe()` ve `dup2()` ile `komut1 | komut2` desteği sağlanır. |
| Built-in komutlar | `cd`, `exit`, `pwd`, `history`, `jobs`, `log`, `clear`, `help`, `fg`, `killjob` desteklenir. |
| Background process | Komut sonunda `&` kullanılırsa süreç arka planda başlatılır. |
| Hatalı komutta shell'in kapanmaması | `execvp()` başarısız olursa hata yazdırılır, çocuk süreç sonlanır ve shell yeni komut almaya devam eder. |
| Son 10 komutluk history | `history_arr` dizisi son 10 komutu tutar; sınır aşılırsa en eski komut silinir. |

## Ek Özellikler

Zorunlu maddelere ek olarak projeye kullanımı kolaylaştıran bazı özellikler eklenmiştir:

- `help [komut]` ile komuta özel yardım görüntüleme.
- `log <sayi>` ile son N log kaydını listeleme.
- `log clear` ile log dosyasını temizleme.
- `history clear` ile komut geçmişini temizleme.
- `killjob <no>` ile arka plan sürecini sonlandırma.
- `fg <no>` ile arka plan sürecini foreground olarak bekleme.
- `>` ve `<` operatörleri ile basit çıktı/girdi yönlendirme.
- `jobs` komutunda aktif ve geçmiş arka plan süreçlerini ayrı ayrı gösterme.

Bu ek özellikler projenin temel shell davranışını genişletir ve süreç yönetimi, dosya işlemleri, hata kontrolü ve kullanıcı etkileşimi açısından daha anlaşılır bir yapı sunar.

## Karşılaşılan Problemler ve Çözümler

### Zombi Süreçler

Background süreçler tamamlandığında sistemde zombi süreç olarak kalabilir. Bu problem `SIGCHLD` sinyali ve `waitpid(..., WNOHANG)` kullanılarak çözülmüştür.

### Pipe Dosya Tanımlayıcıları

Pipe kullanırken gereksiz dosya tanımlayıcıları kapatılmazsa süreçler beklemede kalabilir. Bu nedenle pipe işlemlerinde `close()` çağrıları düzenli şekilde yapılmıştır.

### Hatalı Komutlar

Geçersiz komutlarda shell'in kapanmaması için `execvp()` başarısız olduğunda `perror()` ile hata yazdırılmış ve çocuk süreç sonlandırılmıştır.

### Komut Geçmişi Sınırı

History yapısının sadece son 10 komutu tutması için dizi dolduğunda en eski komut silinip yeni komut sona eklenmiştir.

## Sonuç

Bu proje ile C dili ve POSIX/Linux API kullanılarak temel bir Unix shell geliştirilmiştir. Projede process yönetimi, pipe, foreground/background çalışma, mutex ile senkronizasyon, loglama, hata yönetimi, history ve basit yönlendirme özellikleri uygulanmıştır.

Proje, temel bir Unix shell için gerekli süreç yönetimi, pipe, background çalışma, hata yönetimi, loglama ve history özelliklerini içermektedir. Ek olarak `jobs`, `killjob`, `fg`, `log`, `help <komut>` ve yönlendirme gibi kullanımı kolaylaştıran özellikler eklenmiştir.
