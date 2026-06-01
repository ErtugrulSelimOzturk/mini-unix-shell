# Mini Unix Shell - Sistem Programlama Projesi

Bu proje, C dili ve POSIX/Linux API'leri kullanılarak geliştirilmiş, temel Unix kabuk (shell) özelliklerine sahip bir komut satırı arayüzüdür.

## Amaç
Bu projenin amacı, işletim sistemi seviyesinde süreç yönetimi, sinyaller, boru hatları (pipes) ve dosya sistemi etkileşimlerini anlamak ve uygulamaktır.

## Tasarım
Kabuk, klasik bir "Read-Parse-Execute" döngüsü üzerine kurulmuştur:
1.  **Read**: `readline` kütüphanesi kullanılarak kullanıcıdan girdi alınır (Üst ok desteği mevcuttur).
2.  **Parse**: `strtok` ile komutlar ve argümanlar parçalanır.
3.  **Execute**: Komutlar, sürece (fork) ayrılarak `execvp` ile çalıştırılır.

## Kullanılan Sistem Programlama Kavramları
*   **Process Management**: `fork()`, `execvp()`, `waitpid()` ile süreçlerin oluşturulması ve yönetilmesi.
*   **Signals**: `SIGCHLD` sinyali ve `sigaction` kullanılarak zombi süreçlerin önlenmesi.
*   **Inter-Process Communication (IPC)**: `pipe()` ve `dup2()` ile süreçler arası veri aktarımı.
*   **Synchronization**: Arka plan süreç listesindeki ortak veriyi korumak için `pthread_mutex_t` kullanılır.
*   **Filesystem**: `chdir()`, `getcwd()`, `mkdir()` ile dizin yönetimi ve izolasyon.
*   **Timing**: `gettimeofday()` ile performans ölçümü.

## Özellikler
*   **Dinamik ve Renkli Prompt**: `[MY-SHELL]:/base/` formatında, 256-renk paleti destekli arayüz.
*   **Sandbox (İzolasyon)**: Tüm işlemler `base` klasörü ile sınırlıdır; kullanıcı proje kök dizinine çıkamaz.
*   **Pipe (|)**: `ls | grep test` gibi komutların çıktıları birbirine bağlanabilir.
*   **Arka Plan Çalıştırma (&)**: Komutların sonuna `&` ekleyerek shell'i bloklamadan çalıştırma.
*   **Jobs Takibi**: `jobs` komutu aktif ve geçmiş arka plan süreçlerini birlikte listeler.
*   **Geçmiş (History)**: Son 10 komutu saklar ve `history` komutuyla listeler. Üst ok tuşu desteği mevcuttur.
*   **Built-in Komutlar**: `cd`, `pwd`, `exit`, `history`, `history clear`, `jobs`, `killjob`, `fg`, `log`, `log clear`, `clear` ve `help` komutları shell içinde desteklenir.
*   **Yönlendirme**: `komut > dosya` ve `komut < dosya` ile basit çıktı/girdi yönlendirme yapılabilir.
*   **Loglama**: Tüm komutların çalışma süreleri ve detayları `shell.log` dosyasına kaydedilir.

## Çalıştırma Adımları
1.  **Gereksinimler**: `gcc`, `make` ve `libreadline-dev` kütüphanesi.
2.  **Derleme**:
    ```bash
    make
    ```
3.  **Çalıştırma**:
    ```bash
    ./shell
    ```

## Testler
*   `touch deneme.txt` ve `ls | grep txt`: Pipe özelliğinin testi.
*   `sleep 10 &`: Arka plan ve zombi süreç yönetiminin testi.
*   `killjob 1` ve `fg 1`: Arka plan süreç yönetimi testi.
*   `cd ..`: Sandbox/Erişim engelleme testi.
*   `history`: Komut geçmişi testi.
*   `history clear`: Komut geçmişini temizleme testi.
*   `log`, `log 5`, `log clear`: Log kayıtlarını shell içinden görüntüleme ve temizleme testi.
*   `echo merhaba > out.txt` ve `cat < out.txt`: Yönlendirme testi.
*   `clear`: Ekran temizleme komutunun testi.
*   `olmayan_komut`: Hatalı komutta shell'in kapanmadığının testi.

## Performans Değerlendirmesi
Her foreground komut için başlangıç ve bitiş zamanı `gettimeofday()` ile ölçülür ve süre milisaniye cinsinden `shell.log` dosyasına yazılır. Background komutlarda shell beklemeden yeni komut kabul ettiği için başlangıç kaydı oluşturulur ve kullanıcı arayüzünün bloklanmadığı gözlemlenir.

Örnek ölçümlerde basit komutların (`pwd`, `ls`) birkaç milisaniye içinde tamamlandığı, pipe kullanılan komutlarda ise iki ayrı süreç ve pipe kurulumu nedeniyle küçük bir ek maliyet oluştuğu görülür. `sleep 10 &` gibi background komutlarda shell anında prompt'a döndüğü için uzun süren komutların kullanıcı etkileşimini bekletmediği test edilmiştir.

## Karşılaşılan Problemler
*   **Zombi Süreçler**: Arka planda çalışan süreçlerin sistemde asılı kalması sorunu `SIGCHLD` handler ile çözüldü.
*   **Pipe Senkronizasyonu**: Dosya tanımlayıcılarının (file descriptors) doğru kapatılmaması süreçlerin kilitlenmesine neden oluyordu; `close()` çağrıları titizlikle düzenlendi.
*   **Girdi Okuma**: Standart `fgets` ile ok tuşlarını yakalamak mümkün değildi, bu yüzden `readline` kütüphanesine geçiş yapıldı.
