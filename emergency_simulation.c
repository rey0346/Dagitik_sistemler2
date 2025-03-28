#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>

#define TOPLAM_OLAY 1000
#define EKIP_SAYISI 100
#define MUD_HIZI 40.0 

#define YANGIN 1
#define AMBULANS 2
#define POLIS 3

// Olay yapısı
typedef struct {
    int id;
    int olay_tipi;
    float x, y;
    int atandi;
    float mesafe;
    float sure;
    int rank;
} Olay;

// Ekip yapısı
typedef struct {
    float x, y;
    int tur;
    float musait_zaman;
} Ekip;

float rastgele_koordinat(float min, float max) {
    return min + (rand() / (float)RAND_MAX) * (max - min);
}

int rastgele_olay_tipi() {
    return 1 + rand() % 3;
}

void ekip_olustur(Ekip* ekipler, int sayi) {
    for (int i = 0; i < sayi; i++) {
        ekipler[i].x = rastgele_koordinat(0, 100);
        ekipler[i].y = rastgele_koordinat(0, 100);
        ekipler[i].tur = 1 + rand() % 3;
        ekipler[i].musait_zaman = 0.0;
    }
}

float mesafe(float x1, float y1, float x2, float y2) {
    return sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
}

int main(int argc, char** argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int olay_sayisi = TOPLAM_OLAY / size;
    Olay* local_olaylar = malloc(sizeof(Olay) * olay_sayisi);

    if (rank == 0) {
        srand(time(NULL));
        Olay* tum_olaylar = malloc(sizeof(Olay) * TOPLAM_OLAY);

        for (int i = 0; i < TOPLAM_OLAY; i++) {
            tum_olaylar[i].id = i + 1;
            tum_olaylar[i].olay_tipi = rastgele_olay_tipi();
            tum_olaylar[i].x = rastgele_koordinat(0, 100);
            tum_olaylar[i].y = rastgele_koordinat(0, 100);
            tum_olaylar[i].atandi = 0;
            tum_olaylar[i].mesafe = 0.0;
            tum_olaylar[i].sure = 0.0;
            tum_olaylar[i].rank = 0;
        }

        MPI_Scatter(tum_olaylar, olay_sayisi * sizeof(Olay), MPI_BYTE,
                    local_olaylar, olay_sayisi * sizeof(Olay), MPI_BYTE,
                    0, MPI_COMM_WORLD);
        free(tum_olaylar);
    } else {
        MPI_Scatter(NULL, olay_sayisi * sizeof(Olay), MPI_BYTE,
                    local_olaylar, olay_sayisi * sizeof(Olay), MPI_BYTE,
                    0, MPI_COMM_WORLD);
    }

    Ekip ekipler[EKIP_SAYISI];
    srand(time(NULL) + rank);
    ekip_olustur(ekipler, EKIP_SAYISI);

    float toplam_mesafe = 0.0;
    float toplam_sure = 0.0;
    int basarili = 0;

    #pragma omp parallel for schedule(dynamic) reduction(+:toplam_mesafe, toplam_sure, basarili)
    for (int i = 0; i < olay_sayisi; i++) {
        float olay_zamani = i * 0.5f;
        float en_kisa = 999999.0;
        int secilen = -1;
        float en_uygun_zaman = 999999.0;

        for (int j = 0; j < EKIP_SAYISI; j++) {
            if (ekipler[j].tur == local_olaylar[i].olay_tipi && ekipler[j].musait_zaman <= olay_zamani) {
                float uzaklik = mesafe(local_olaylar[i].x, local_olaylar[i].y, ekipler[j].x, ekipler[j].y);
                float tahmini_sure = uzaklik / MUD_HIZI;
                float yeni_zaman = olay_zamani + tahmini_sure;

                #pragma omp critical
                {
                    if (yeni_zaman < en_uygun_zaman) {
                        en_kisa = uzaklik;
                        secilen = j;
                        en_uygun_zaman = yeni_zaman;
                    }
                }
            }
        }

        if (secilen != -1) {
            float sure = en_kisa / MUD_HIZI;

            #pragma omp critical
            ekipler[secilen].musait_zaman = olay_zamani + sure * 500;

            local_olaylar[i].atandi = 1;
            local_olaylar[i].mesafe = en_kisa;
            local_olaylar[i].sure = sure;
            local_olaylar[i].rank = rank;

            toplam_mesafe += en_kisa;
            toplam_sure += sure;
            basarili++;

            printf("[Rank %d] Olay %d → Ekip %d (%.2f birim, %.2f saniye)\n",
                   rank, local_olaylar[i].id, secilen, en_kisa, sure);
        }
    }

    float genel_mesafe = 0.0;
    float genel_sure = 0.0;
    int toplam_basarili = 0;

    MPI_Reduce(&toplam_mesafe, &genel_mesafe, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&toplam_sure, &genel_sure, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&basarili, &toplam_basarili, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    Olay* tum_olaylar = NULL;
    if (rank == 0) {
        tum_olaylar = malloc(sizeof(Olay) * olay_sayisi * size);
    }

    MPI_Gather(local_olaylar, olay_sayisi * sizeof(Olay), MPI_BYTE,
               tum_olaylar, olay_sayisi * sizeof(Olay), MPI_BYTE,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        int toplam_olay = olay_sayisi * size;
        float basari_orani = 100.0 * toplam_basarili / toplam_olay;
        float ortalama_sure = genel_sure / toplam_basarili;

        printf("\nISTATISTIKLER\n");
        printf("Toplam olay sayisi      : %d\n", toplam_olay);
        printf("Basariyla mudahale      : %d\n", toplam_basarili);
        printf("Basari orani            : %.2f%%\n", basari_orani);
        printf("Toplam mudahale suresi  : %.2f saniye\n", genel_sure);
        printf("Ortalama mudahale suresi: %.2f saniye\n", ortalama_sure);
        printf("Toplam mesafe           : %.2f birim\n", genel_mesafe);

        FILE* dosya = fopen("results.csv", "w");
        if (dosya != NULL) {
            fprintf(dosya, "id,tur,x,y,atandi,mesafe,sure,rank\n");
            for (int i = 0; i < toplam_olay; i++) {
                const char* tur_str;
                if (tum_olaylar[i].olay_tipi == 1) tur_str = "Yangin";
                else if (tum_olaylar[i].olay_tipi == 2) tur_str = "Ambulans";
                else tur_str = "Polis";

                const char* atandi_str = tum_olaylar[i].atandi ? "atandi" : "atanmadi";

                fprintf(dosya, "%d,%s,%.2f,%.2f,%s,%.2f,%.2f,%d\n",
                        tum_olaylar[i].id,
                        tur_str,
                        tum_olaylar[i].x,
                        tum_olaylar[i].y,
                        atandi_str,
                        tum_olaylar[i].mesafe,
                        tum_olaylar[i].sure,
                        tum_olaylar[i].rank);
            }

            fprintf(dosya, "\nISTATISTIKLER\n");
            fprintf(dosya, "Toplam olay sayisi      : %d\n", toplam_olay);
            fprintf(dosya, "Basariyla mudahale      : %d\n", toplam_basarili);
            fprintf(dosya, "Basari orani            : %.2f%%\n", basari_orani);
            fprintf(dosya, "Toplam mudahale suresi  : %.2f saniye\n", genel_sure);
            fprintf(dosya, "Ortalama mudahale suresi: %.2f saniye\n", ortalama_sure);
            fprintf(dosya, "Toplam mesafe           : %.2f birim\n", genel_mesafe);

            fclose(dosya);
            printf("\nSonuclar 'results.csv' dosyasina yazildi.\n");
        }
        free(tum_olaylar);
    }

    free(local_olaylar);
    MPI_Finalize();
    return 0;
}