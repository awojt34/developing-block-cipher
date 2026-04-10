#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ab_lab.h"
#include <time.h>

#define N 16
#define NUM_TESTS 34
#define NUM_ITERATIONS 5

uint32_t P[18];
uint32_t S[4][256];

uint32_t F(uint32_t x) {
    uint8_t a, b, c, d;
    uint32_t y;

    d = x & 0xFF;
    x >>= 8;
    c = x & 0xFF;
    x >>= 8;
    b = x & 0xFF;
    x >>= 8;
    a = x & 0xFF;

    y = S[0][a] + S[1][b];
    y = y ^ S[2][c];
    y = y + S[3][d];

    return y;
}

void encryptBlock(uint32_t *xl, uint32_t *xr) {
    uint32_t L = *xl;
    uint32_t R = *xr;
    uint32_t temp;
    int i;

    for (i = 0; i < N; i++) {
        L = L ^ P[i];
        R = F(L) ^ R;

        temp = L;
        L = R;
        R = temp;
    }

    temp = L;
    L = R;
    R = temp;

    R = R ^ P[N];
    L = L ^ P[N + 1];

    *xl = L;
    *xr = R;
}

void decryptBlock(uint32_t *xl, uint32_t *xr) {
    uint32_t L = *xl;
    uint32_t R = *xr;
    uint32_t temp;
    int i;

    for (i = N + 1; i > 1; i--) {
        L = L ^ P[i];
        R = F(L) ^ R;

        temp = L;
        L = R;
        R = temp;
    }

    temp = L;
    L = R;
    R = temp;

    R = R ^ P[1];
    L = L ^ P[0];

    *xl = L;
    *xr = R;
}

void blowfishInitialize(uint8_t *key, int keyLen) {
    int i, j, k;
    uint32_t data;
    uint32_t datal = 0x00000000;
    uint32_t datar = 0x00000000;

    for (i = 0; i < N + 2; ++i) {
        uint32_t target = 0;
        uint32_t source = P_INIT[i];
        for (int bit = 0; bit < 32; ++bit) {
            volatile uint32_t mask = (1 << bit);
            if (source & mask) {
                target |= mask;
            }
        }
        P[i] = target;
    }

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 256; ++j) {
            uint32_t target = 0;
            uint32_t source = S_INIT[i][j];
            for (int bit = 0; bit < 32; ++bit) {
                volatile uint32_t mask = (1 << bit);
                if (source & mask) {
                    target |= mask;
                }
            }
            S[i][j] = target;
        }
    }

    j = 0;
    for (i = 0; i < N + 2; ++i) {
        data = 0x00000000;

        for (k = 0; k < 4; ++k) {
            data = (data << 8) | key[j];
            j++;
            if (j >= keyLen) j = 0;
        }
        
        P[i] = P[i] ^ data;
    }

    datal = 0x00000000;
    datar = 0x00000000;

    for (i = 0; i < N + 2; i += 2) {
        encryptBlock(&datal, &datar);
        P[i] = datal;
        P[i + 1] = datar;
    }

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 256; j += 2) {
            encryptBlock(&datal, &datar);
            S[i][j] = datal;
            S[i][j + 1] = datar;
        }
    }
}

void printKeyShort(const uint8_t *key, int len) {
    for(int i=0; i<len; i++) printf("%02X", key[i]);
}

int main() {
    static const uint8_t variable_key[NUM_TESTS][8] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        { 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 },
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
        { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 },
        { 0x7C, 0xA1, 0x10, 0x45, 0x4A, 0x1A, 0x6E, 0x57 },
        { 0x01, 0x31, 0xD9, 0x61, 0x9D, 0xC1, 0x37, 0x6E },
        { 0x07, 0xA1, 0x13, 0x3E, 0x4A, 0x0B, 0x26, 0x86 },
        { 0x38, 0x49, 0x67, 0x4C, 0x26, 0x02, 0x31, 0x9E },
        { 0x04, 0xB9, 0x15, 0xBA, 0x43, 0xFE, 0xB5, 0xB6 },
        { 0x01, 0x13, 0xB9, 0x70, 0xFD, 0x34, 0xF2, 0xCE },
        { 0x01, 0x70, 0xF1, 0x75, 0x46, 0x8F, 0xB5, 0xE6 },
        { 0x43, 0x29, 0x7F, 0xAD, 0x38, 0xE3, 0x73, 0xFE },
        { 0x07, 0xA7, 0x13, 0x70, 0x45, 0xDA, 0x2A, 0x16 },
        { 0x04, 0x68, 0x91, 0x04, 0xC2, 0xFD, 0x3B, 0x2F },
        { 0x37, 0xD0, 0x6B, 0xB5, 0x16, 0xCB, 0x75, 0x46 },
        { 0x1F, 0x08, 0x26, 0x0D, 0x1A, 0xC2, 0x46, 0x5E },
        { 0x58, 0x40, 0x23, 0x64, 0x1A, 0xBA, 0x61, 0x76 },
        { 0x02, 0x58, 0x16, 0x16, 0x46, 0x29, 0xB0, 0x07 },
        { 0x49, 0x79, 0x3E, 0xBC, 0x79, 0xB3, 0x25, 0x8F },
        { 0x4F, 0xB0, 0x5E, 0x15, 0x15, 0xAB, 0x73, 0xA7 },
        { 0x49, 0xE9, 0x5D, 0x6D, 0x4C, 0xA2, 0x29, 0xBF },
        { 0x01, 0x83, 0x10, 0xDC, 0x40, 0x9B, 0x26, 0xD6 },
        { 0x1C, 0x58, 0x7F, 0x1C, 0x13, 0x92, 0x4F, 0xEF },
        { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
        { 0x1F, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E },
        { 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1, 0xFE },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF },
        { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 }
    };

    static const uint32_t plaintext_l[NUM_TESTS] = {
        0x00000000l, 0xFFFFFFFFl, 0x10000000l, 0x11111111l, 0x11111111l,
        0x01234567l, 0x00000000l, 0x01234567l, 0x01A1D6D0l, 0x5CD54CA8l,
        0x0248D438l, 0x51454B58l, 0x42FD4430l, 0x059B5E08l, 0x0756D8E0l,
        0x762514B8l, 0x3BDD1190l, 0x26955F68l, 0x164D5E40l, 0x6B056E18l,
        0x004BD6EFl, 0x480D3900l, 0x437540C8l, 0x072D43A0l, 0x02FE5577l,
        0x1D9D5C50l, 0x30553228l, 0x01234567l, 0x01234567l, 0x01234567l,
        0xFFFFFFFFl, 0x00000000l, 0x00000000l, 0xFFFFFFFFl
    };

    static const uint32_t plaintext_r[NUM_TESTS] = {
        0x00000000l, 0xFFFFFFFFl, 0x00000001l, 0x11111111l, 0x11111111l,
        0x89ABCDEFl, 0x00000000l, 0x89ABCDEFl, 0x39776742l, 0x3DEF57DAl,
        0x06F67172l, 0x2DDF440Al, 0x59577FA2l, 0x51CF143Al, 0x774761D2l,
        0x29BF486Al, 0x49372802l, 0x35AF609Al, 0x4F275232l, 0x759F5CCAl,
        0x09176062l, 0x6EE762F2l, 0x698F3CFAl, 0x77075292l, 0x8117F12Al,
        0x18F728C2l, 0x6D6F295Al, 0x89ABCDEFl, 0x89ABCDEFl, 0x89ABCDEFl,
        0xFFFFFFFFl, 0x00000000l, 0x00000000l, 0xFFFFFFFFl
    };

    static const uint32_t expected_l[NUM_TESTS] = {
        0x4EF99745l, 0x51866FD5l, 0x7D856F9Al, 0x2466DD87l, 0x61F9C380l,
        0x7D0CC630l, 0x4EF99745l, 0x0ACEAB0Fl, 0x59C68245l, 0xB1B8CC0Bl,
        0x1730E577l, 0xA25E7856l, 0x353882B1l, 0x48F4D088l, 0x432193B7l,
        0x13F04154l, 0x2EEDDA93l, 0xD887E039l, 0x5F99D04Fl, 0x4A057A3Bl,
        0x452031C1l, 0x7555AE39l, 0x53C55F9Cl, 0x7A8E7BFAl, 0xCF9C5D7Al,
        0xD1ABB290l, 0x55CB3774l, 0xFA34EC48l, 0xA7907951l, 0xC39E072Dl,
        0x014933E0l, 0xF21E9A77l, 0x24594688l, 0x6B5C5A9Cl
    };

    static const uint32_t expected_r[NUM_TESTS] = {
        0x6198DD78l, 0xB85ECB8Al, 0x613063F2l, 0x8B963C9Dl, 0x2281B096l,
        0xAFDA1EC7l, 0x6198DD78l, 0xC6A0A28Dl, 0xEB05282Bl, 0x250F09A0l,
        0x8BEA1DA4l, 0xCF2651EBl, 0x09CE8F1Al, 0x4C379918l, 0x8951FC98l,
        0xD69D1AE5l, 0xFFD39C79l, 0x3C2DA6E3l, 0x5B163969l, 0x24D3977Bl,
        0xE4FADA8El, 0xF59B87BDl, 0xB49FC019l, 0x937E89A3l, 0x4986ADB5l,
        0x658BC778l, 0xD13EF201l, 0x47B268B2l, 0x08EA3CAEl, 0x9FAC631Dl,
        0xCDAFF6E4l, 0xB71C49BCl, 0x5754369Al, 0x5D9E0A5Al
    };

    int keysize = 8;
    int total_passed = 0;
    double total_time_all_iterations = 0.0;


    for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
        printf("\n--- Iteracja %d/%d ---\n", iteration + 1, NUM_ITERATIONS);
        printf("| %-16s | %-17s | %-17s | %-17s | %-6s |\n", 
               "KEY", "PLAINTEXT", "CIPHER", "REFERENCE", "STATUS");

        int passed = 0;
        double iteration_time = 0.0;

        for (int i = 0; i < NUM_TESTS; i++) {
            clock_t start_time = clock();

            blowfishInitialize((uint8_t*)variable_key[i], keysize);

            uint32_t L = plaintext_l[i];
            uint32_t R = plaintext_r[i];
            
            uint32_t origL = L;
            uint32_t origR = R;

            encryptBlock(&L, &R);
            uint32_t cipherL = L;
            uint32_t cipherR = R;

            clock_t end_time = clock();

            int is_correct = (cipherL == expected_l[i] && cipherR == expected_r[i]);
            if(is_correct) passed++;

            printKeyShort(variable_key[i], keysize);
            printf(" | %08X %08X | %08X %08X | %08X %08X | %s  |\n", 
                   origL, origR,
                   cipherL, cipherR,
                   expected_l[i], expected_r[i],
                   is_correct ? "  OK  " : " FAIL "
            );

            decryptBlock(&L, &R);
            if (L != origL || R != origR) {
                printf("Błąd deszyfrowania.\n");
            }

            double test_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
            iteration_time += test_time;
        }

        total_passed += passed;
        total_time_all_iterations += iteration_time;

        printf("\nIteracja %d: Testow zaliczonych: %d/%d\n", iteration + 1, passed, NUM_TESTS);
        printf("Iteracja %d: Czas: %.6f sekund\n", iteration + 1, iteration_time);
    }

    printf("\n=== PODSUMOWANIE WSZYSTKICH ITERACJI ===\n");
    printf("Liczba iteracji: %d\n", NUM_ITERATIONS);
    printf("Testow na iteracje: %d\n", NUM_TESTS);
    printf("Calkowita liczba testow: %d\n", NUM_TESTS * NUM_ITERATIONS);
    printf("Laczna liczba zaliczonych testow: %d/%d\n", total_passed, NUM_TESTS * NUM_ITERATIONS);
    printf("\nCzasy:\n");
    printf("  Calkowity czas wszystkich iteracji: %.6f sekund\n", total_time_all_iterations);
    printf("  Sredni czas jednej iteracji: %.6f sekund\n", total_time_all_iterations / NUM_ITERATIONS);
    printf("  Sredni czas jednego testu: %.6f sekund\n", total_time_all_iterations / (NUM_TESTS * NUM_ITERATIONS));
    printf("  Szacowana predkosc: %.2f testow/sekunde\n", (NUM_TESTS * NUM_ITERATIONS) / total_time_all_iterations);

    return 0;
}
