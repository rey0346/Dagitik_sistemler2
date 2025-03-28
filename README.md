# 🆘 Emergency Simulation: MPI + OpenMP Tabanlı Acil Durum Simülasyonu

Bu projede `emergency_simulation.c` dosyasında **MPI (Message Passing Interface)** ve **OpenMP (Open Multi-Processing)** kullanılarak çok iş parçacıklı ve çok işlemcili bir acil durum simülasyonu gerçekleştirilmiştir.

---

## 🔥 Simülasyon Özeti

- Şehirde zaman içinde gerçekleşen 1000 adet acil durum olayı (olay tipi ve mesafe) simüle edilir.
- Olay tipleri: **İtfaiye**, **Ambulans**, **Polis**
- Müdahale edebilecek toplam **100 ekip** mevcuttur.
- Bu ekipler, **4 MPI işlemcisine** dağıtılır.
- Her olay geldiğinde, OpenMP ile:
  - 1. Görev: Müsait ekipleri kontrol etme
  - 2. Görev: En yakın uygun ekibi seçip görevlendirme  
  işlemleri **paralel** olarak gerçekleştirilir.

---

## ⚙️ Paralelleştirme Yapısı

### ✅ MPI
- 100 ekip, 4 MPI **rank**'ine eşit şekilde bölünür.
- Her işlemci, kendi olay kümesini işler.

### ✅ OpenMP
- Her MPI işlemi, olaylara müdahaleyi **çok çekirdekli** olarak işler.
- Kullanılan thread'ler:
  - 🧵 Thread 1: Uygun ekip bul
  - 🧵 Thread 2: Ekip görevlendir & müdahale süresini güncelle

---

## 🔧 Derleme ve Çalıştırma

### Derleme

```bash
mpicc -fopenmp -o emergency_simulation emergency_simulation.c -lm
```

### Sadece MPI ile çalıştırma

```bash
mpirun -np 4 ./emergency_simulation
```

### MPI + OpenMP ile çalıştırma

```bash
export OMP_NUM_THREADS=4
mpirun -np 4 ./emergency_simulation
```

---

## 📈 Örnek Çıktı

```
ISTATISTIKLER
Toplam olay sayısı: 1000
Başarıyla müdahale: 573
Başarı oranı: 57.30%
Toplam müdahale süresi: 372.60 saniye
Ortalama müdahale süresi: 0.65 saniye
Toplam mesafe: 14903.98 birim

Toplam yürütme süresi (MPI_Wtime() ile): 0.007153 saniye
```

---

## 🐳 Docker Desteği

### Dockerfile

Bu dosya:

- MPI ve OpenMP kütüphanelerini kurar.
- Simülasyon kodunu derler.
- Konteyner çalıştığında otomatik olarak simülasyonu başlatır.

### docker-compose.yml

Bu dosya:

- Simülasyonu tek komutla başlatmanı sağlar.
- `results.csv` dosyasının dışarı aktarılmasını sağlar.
- Thread sayısını ayarlamak için ortam değişkeni tanımlar.

---

## ⚖️ MPI vs MPI + OpenMP Karşılaştırması

| Yapı                    | Süre (saniye) |
|-------------------------|---------------|
| Sadece MPI (`-np 4`)    | 0.007153      |
| MPI + OpenMP (`-np 4 + 4 threads`) | 0.003361      |

> **Açıklama:**  
MPI yalnızken her işlem tek çekirdek kullanır.  
OpenMP ile birlikte her MPI işlemi birden fazla çekirdek kullanabildiği için aynı anda daha çok işlem yapılır ve simülasyon süresi ciddi oranda azalır.