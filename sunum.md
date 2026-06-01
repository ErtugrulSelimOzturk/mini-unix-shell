# Mini Unix Shell Sunum Planı

## Sunum Bilgileri

**Proje:** Mini Unix Shell

**Konu:** C ve POSIX/Linux API ile komut satırı shell uygulaması

**Sunum Ekibi:** 3 kişi

**Üyeler:**
- Üye 1: XXX
- Üye 2: XXX
- Üye 3: XXX

## Sunum Akışı

Toplam sunum üç ana bölüme ayrılmıştır. Her üye yaklaşık eşit süre konuşur. Sunum sırasında hem proje mantığı hem de çalışan terminal çıktıları gösterilir. Kod üzerinden anlatım yapılabilir; özellikle `fork`, `execvp`, `waitpid`, `pipe`, `pthread_mutex_t` ve loglama bölümleri gösterilebilir.

## Üye 1 - Proje Tanıtımı ve Genel Yapı

**Anlatılacak Başlıklar:**
- Projenin amacı
- Mini Unix Shell nedir?
- Projenin hangi zorunlu şartları karşıladığı
- Programın genel çalışma mantığı
- Read-Parse-Execute döngüsü
- Kullanıcıdan komut okuma
- Built-in komutlar

**Kod Üzerinden Gösterilecek Yerler:**
- `main()` fonksiyonu
- `readline()` ile komut okuma
- `parse_line()` fonksiyonu
- `execute_command()` içindeki built-in komut kontrolleri

**Demo Komutları:**

```bash
./shell
help
pwd
mkdir test
cd test
cd ..
rmdir test
```

**Ekran Görüntüsü Önerileri:**

[Görüntü 1: Shell ilk açılış ekranı]

Alt yazı: Program `./shell` komutu ile başlatılmıştır. Açılışta Mini Unix Shell başlığı, komut listesi ve örnek kullanımlar kullanıcıya gösterilir.

[Görüntü 2: Help komutu ve örnekli komut listesi]

Alt yazı: `help` komutu ile desteklenen komutlar, açıklamaları ve örnek kullanımları tablo halinde listelenir.

[Görüntü 3: `pwd`, `mkdir`, `cd`, `rmdir` komutlarının çalışması]

Alt yazı: Temel dizin işlemleri gösterilmektedir. `pwd` mevcut dizini gösterir, `mkdir` dizin oluşturur, `cd` dizin değiştirir ve `rmdir` boş dizini siler.

**Konuşma Notu:**

Bu bölümde proje tanıtılır. Shell'in kullanıcıdan komut aldığı, komutu parçaladığı ve komut türüne göre built-in veya dış komut olarak çalıştırdığı anlatılır. `help` ekranındaki komut listesi üzerinden projenin özellikleri hızlıca gösterilir.

## Üye 2 - Process Management, Pipe ve Background Süreçler

**Anlatılacak Başlıklar:**
- `fork()` ile çocuk süreç oluşturma
- `execvp()` ile dış komut çalıştırma
- `waitpid()` ile foreground komut bekleme
- Background process desteği
- `jobs`, `fg`, `killjob` komutları
- Tek seviyeli pipe desteği
- `pipe()` ve `dup2()` kullanımı

**Kod Üzerinden Gösterilecek Yerler:**
- `execute_command()` içinde normal komut çalıştırma
- `execute_pipeline()` fonksiyonu
- `add_job()`, `remove_job()`, `show_jobs()`
- `reap_background_jobs()`

**Demo Komutları:**

```bash
touch deneme.txt
ls | grep txt
sleep 10 &
jobs
killjob 1
sleep 1 &
fg 1
```

**Ekran Görüntüsü Önerileri:**

[Görüntü 4: Pipe kullanımı - `ls | grep txt`]

Alt yazı: Pipe örneğinde `ls` komutunun çıktısı `grep txt` komutuna aktarılır. Böylece sadece `.txt` içeren dosya adları filtrelenir.

[Görüntü 5: Background process - `sleep 10 &`]

Alt yazı: `sleep 10 &` komutu arka planda başlatılır. Shell bu komutun bitmesini beklemeden yeni komut almaya devam eder.

[Görüntü 6: `jobs`, `killjob`, `fg` komutları]

Alt yazı: `jobs` aktif ve geçmiş arka plan süreçlerini gösterir. `killjob 1` seçilen süreci sonlandırır, `fg 1` ise arka plan sürecini foreground olarak bekler.

**Konuşma Notu:**

Bu bölümde projenin en önemli sistem programlama kısmı anlatılır. Her dış komutun ayrı bir süreçte çalıştırıldığı, foreground komutlarda shell'in beklediği, background komutlarda ise beklemeden yeni komut aldığı gösterilir. Pipe örneği ile iki süreç arasında veri aktarımı açıklanır.

## Üye 3 - Senkronizasyon, Loglama, Hata Yönetimi ve Testler

**Anlatılacak Başlıklar:**
- `pthread_mutex_t` ile senkronizasyon
- Arka plan süreç listesinin korunması
- `shell.log` dosyasına loglama
- Komut çalışma süresi ölçümü
- `gettimeofday()` ile performans ölçümü
- Hata yönetimi
- History yapısı
- Yönlendirme desteği
- README ve rapor yapısı

**Kod Üzerinden Gösterilecek Yerler:**
- `pthread_mutex_lock()` ve `pthread_mutex_unlock()` kullanılan bölümler
- `log_command()` fonksiyonu
- `show_log()` ve `clear_log()`
- `add_to_history_arr()`, `show_history()`, `clear_history_arr()`
- `setup_redirection()`

**Demo Komutları:**

```bash
history
history clear
echo merhaba > out.txt
cat < out.txt
olmayan_komut
ls |
log 5
```

**Ekran Görüntüsü Önerileri:**

[Görüntü 7: History komutu ve son 10 komut]

Alt yazı: `history` komutu son 10 komutu listeler. Komut sayısı 10'u geçerse en eski kayıt silinir ve yeni komut sona eklenir.

[Görüntü 8: Yönlendirme - `echo merhaba > out.txt`, `cat < out.txt`]

Alt yazı: `>` operatörü komut çıktısını dosyaya yazar. `<` operatörü ise komut girdisini dosyadan okur.

[Görüntü 9: Hatalı komut ve hatalı pipe mesajı]

Alt yazı: Hatalı komut veya eksik pipe kullanımında shell kapanmaz. Kullanıcıya uygun hata mesajı verilir ve yeni komut alınmaya devam edilir.

[Görüntü 10: `log 5` ile log kayıtları]

Alt yazı: `log 5` komutu son 5 log kaydını gösterir. Loglarda komut adı, çalışma süresi, komut tipi ve durum bilgisi yer alır.

**Konuşma Notu:**

Bu bölümde projenin güvenilirlik ve takip tarafı anlatılır. Background süreç listesi üzerinde mutex kullanıldığı, komutların log dosyasına yazıldığı ve hatalı komutlarda shell'in kapanmadığı gösterilir. Ayrıca süre ölçümü üzerinden kısa performans değerlendirmesi yapılır.

## Kod Üzerinden Anlatılabilecek Önemli Fonksiyonlar

### `main()`

Shell döngüsünü yönetir. Kullanıcıdan komut alır, komutu ayrıştırır ve çalıştırma fonksiyonlarına yönlendirir.

### `parse_line()`

Kullanıcının yazdığı komutu boşluklara göre parçalar. Komut sonunda `&` varsa background olarak işaretler.

### `execute_command()`

Built-in komutları kontrol eder. Dış komutlar için `fork()` ve `execvp()` kullanır.

### `execute_pipeline()`

`|` içeren komutlarda iki çocuk süreç oluşturur. `pipe()` ve `dup2()` ile süreçler arası veri aktarımı sağlar.

### `add_job()`, `remove_job()`, `show_jobs()`

Background süreçleri yönetir. Aktif ve geçmiş background süreçlerini listeler.

### `log_command()`

Komutları `shell.log` dosyasına süre, tip ve durum bilgisiyle yazar.

### `setup_redirection()`

`>` ve `<` yönlendirme operatörlerini işler.

## Kısa Demo Sırası

Sunumda hızlı ve anlaşılır bir demo için şu sıra kullanılabilir:

```bash
make
./shell
help
touch deneme.txt
ls | grep txt
sleep 10 &
jobs
killjob 1
echo merhaba > out.txt
cat < out.txt
history
olmayan_komut
ls |
log 5
exit
```

## Zorunlu Şartların Sunumda Gösterimi

| Zorunlu Şart | Projede Karşılığı | Sunacak Üye |
| --- | --- | --- |
| C ve POSIX/Linux API | `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `chdir`, `getcwd` | Üye 1, Üye 2 |
| Process kullanımı | `fork()` ile çocuk süreçler | Üye 2 |
| Senkronizasyon | `pthread_mutex_t jobs_mutex` | Üye 3 |
| Loglama | `shell.log`, `log_command()` | Üye 3 |
| Hata yönetimi | `perror`, özel hata mesajları | Üye 3 |
| Performans değerlendirmesi | `gettimeofday()` ile süre ölçümü | Üye 3 |
| Komut okuma | `readline()` | Üye 1 |
| Pipe desteği | `execute_pipeline()` | Üye 2 |
| Built-in komutlar | `cd`, `exit`, `history`, `jobs`, `log`, `help` | Üye 1 |
| Background process | `&`, `jobs`, `fg`, `killjob` | Üye 2 |
| History | Son 10 komut listesi | Üye 3 |

## Sunum Sonu Kapanış

Bu projede C dili ve POSIX/Linux API kullanılarak temel bir Unix shell geliştirilmiştir. Proje zorunlu şartları karşılamaktadır. Ek olarak `jobs`, `killjob`, `fg`, `log`, `history clear`, `help <komut>` ve yönlendirme desteği gibi özellikler eklenmiştir.

Sunum sonunda şu mesaj verilebilir:

> Projede süreç yönetimi, pipe, background çalışma, senkronizasyon, hata yönetimi, loglama ve performans ölçümü gibi temel sistem programlama konuları uygulamalı olarak gösterilmiştir.
