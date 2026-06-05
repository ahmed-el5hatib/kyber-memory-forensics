#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <oqs/oqs.h>

/* Helper function to print a label, memory address, and partial hex representation (first N & last M bytes) of a buffer */
void print_hex_partial(const char *label, const uint8_t *data, size_t first_n, size_t last_m, size_t total_len) {
    printf("%s:\n", label);
    printf("  Memory Address:          %p\n", (void*)data);
    printf("  Total Size:              %zu bytes\n", total_len);
    
    printf("  First %zu bytes (Hex):    ", first_n);
    for (size_t i = 0; i < first_n && i < total_len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");

    printf("  Last %zu bytes (Hex):     ", last_m);
    if (total_len >= last_m) {
        for (size_t i = total_len - last_m; i < total_len; i++) {
            printf("%02X", data[i]);
        }
    } else {
        for (size_t i = 0; i < total_len; i++) {
            printf("%02X", data[i]);
        }
    }
    printf("\n\n");
}

/* Helper function to write a label, memory address, and partial hex representation to a text file */
void write_hex_to_file(const char *filename, const char *label, const uint8_t *data, size_t first_n, size_t last_m, size_t total_len) {
    FILE *f = fopen(filename, "a");
    if (!f) {
        fprintf(stderr, "[!] Warning: Could not open file %s for writing.\n", filename);
        return;
    }
    fprintf(f, "%s:\n", label);
    fprintf(f, "  Memory Address:          %p\n", (void*)data);
    fprintf(f, "  Total Size:              %zu bytes\n", total_len);
    
    fprintf(f, "  First %zu bytes (Hex):    ", first_n);
    for (size_t i = 0; i < first_n && i < total_len; i++) {
        fprintf(f, "%02X", data[i]);
    }
    fprintf(f, "\n");

    fprintf(f, "  Last %zu bytes (Hex):     ", last_m);
    if (total_len >= last_m) {
        for (size_t i = total_len - last_m; i < total_len; i++) {
            fprintf(f, "%02X", data[i]);
        }
    } else {
        for (size_t i = 0; i < total_len; i++) {
            fprintf(f, "%02X", data[i]);
        }
    }
    fprintf(f, "\n\n");
    fclose(f);
}

int main(void) {
    // 1. Initialize liboqs
    OQS_init();
    printf("[+] liboqs initialized. Version: %s\n", OQS_version());

    // 2. Create a Kyber-512 KEM object
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_kyber_512);
    if (kem == NULL) {
        fprintf(stderr, "[!] Error: Failed to initialize Kyber-512 KEM object. Check if Kyber-512 algorithm is supported by your liboqs build.\n");
        return EXIT_FAILURE;
    }
    printf("[+] Kyber-512 KEM object created successfully.\n");

    // 3. Allocate arrays for public key (800 bytes) and secret key (1632 bytes)
    // We use the macro constants if available; otherwise, dynamic sizes from KEM struct.
#ifdef OQS_KEM_kyber_512_length_public_key
    size_t public_key_len = OQS_KEM_kyber_512_length_public_key;
#else
    size_t public_key_len = kem->length_public_key;
#endif

#ifdef OQS_KEM_kyber_512_length_secret_key
    size_t secret_key_len = OQS_KEM_kyber_512_length_secret_key;
#else
    size_t secret_key_len = kem->length_secret_key;
#endif

    printf("[+] Key sizes: Public Key = %zu bytes, Secret Key = %zu bytes\n", public_key_len, secret_key_len);

    uint8_t *public_key = malloc(public_key_len);
    uint8_t *secret_key = malloc(secret_key_len);

    if (public_key == NULL || secret_key == NULL) {
        fprintf(stderr, "[!] Error: Memory allocation failed for keys.\n");
        if (public_key) free(public_key);
        if (secret_key) free(secret_key);
        OQS_KEM_free(kem);
        return EXIT_FAILURE;
    }

    // Zero out memory before keygen
    memset(public_key, 0, public_key_len);
    memset(secret_key, 0, secret_key_len);

    // 4. Generate Kyber-512 key pair
    OQS_STATUS rc = OQS_KEM_keypair(kem, public_key, secret_key);
    if (rc != OQS_SUCCESS) {
        fprintf(stderr, "[!] Error: Key pair generation failed.\n");
        free(public_key);
        free(secret_key);
        OQS_KEM_free(kem);
        return EXIT_FAILURE;
    }
    printf("[+] Kyber-512 key pair generated and loaded in heap.\n\n");

    // 5. Perform encapsulation and decapsulation to simulate real usage
    size_t ciphertext_len = kem->length_ciphertext;
    size_t shared_secret_len = kem->length_shared_secret;

    uint8_t *ciphertext = malloc(ciphertext_len);
    uint8_t *shared_secret_enc = malloc(shared_secret_len);
    uint8_t *shared_secret_dec = malloc(shared_secret_len);

    if (ciphertext == NULL || shared_secret_enc == NULL || shared_secret_dec == NULL) {
        fprintf(stderr, "[!] Error: Memory allocation failed for ephemeral encryption variables.\n");
        if (ciphertext) free(ciphertext);
        if (shared_secret_enc) free(shared_secret_enc);
        if (shared_secret_dec) free(shared_secret_dec);
        OQS_MEM_cleanse(public_key, public_key_len);
        OQS_MEM_cleanse(secret_key, secret_key_len);
        free(public_key);
        free(secret_key);
        OQS_KEM_free(kem);
        return EXIT_FAILURE;
    }

    // Encapsulate
    rc = OQS_KEM_encaps(kem, ciphertext, shared_secret_enc, public_key);
    if (rc != OQS_SUCCESS) {
        fprintf(stderr, "[!] Error: Kyber encapsulation failed.\n");
    } else {
        printf("[+] Encapsulation successful. Ciphertext generated (%zu bytes).\n", ciphertext_len);

        // Decapsulate
        rc = OQS_KEM_decaps(kem, shared_secret_dec, ciphertext, secret_key);
        if (rc != OQS_SUCCESS) {
            fprintf(stderr, "[!] Error: Kyber decapsulation failed.\n");
        } else {
            printf("[+] Decapsulation successful.\n");
            // Verify shared secrets match
            if (memcmp(shared_secret_enc, shared_secret_dec, shared_secret_len) == 0) {
                printf("[+] Verification: Success! Shared secrets match.\n");
            } else {
                printf("[!] Verification: Failed! Shared secrets do not match.\n");
            }
        }
    }
    printf("\n");

    // Cleanse ephemeral variables to ensure memory forensic target is primarily the long-term keys
    OQS_MEM_cleanse(shared_secret_enc, shared_secret_len);
    OQS_MEM_cleanse(shared_secret_dec, shared_secret_len);
    free(shared_secret_enc);
    free(shared_secret_dec);
    free(ciphertext);

    // 6. Print current process ID (PID)
    DWORD pid = GetCurrentProcessId();
    printf("=========================================================\n");
    printf("                     PROCESS INFO                        \n");
    printf("=========================================================\n");
    printf("Process ID (PID): %lu\n\n", pid);

    // 7. Print the memory locations and partial contents of the keys
    printf("=========================================================\n");
    printf("                     KEY DETAILS                         \n");
    printf("=========================================================\n");
    print_hex_partial("Kyber-512 Public Key", public_key, 32, 32, public_key_len);
    print_hex_partial("Kyber-512 Secret Key", secret_key, 32, 32, secret_key_len);

    // 8. Write the same data to 'kyber_keys.txt'
    const char *filename = "kyber_keys.txt";
    FILE *f_init = fopen(filename, "w"); // overwrite/create new
    if (f_init) {
        fprintf(f_init, "Process ID (PID): %lu\n\n", pid);
        fclose(f_init);
    }
    write_hex_to_file(filename, "Kyber-512 Public Key", public_key, 32, 32, public_key_len);
    write_hex_to_file(filename, "Kyber-512 Secret Key", secret_key, 32, 32, secret_key_len);
    printf("[+] Key details written to '%s' in current directory.\n\n", filename);

    // 9. Keep keys in scope and wait for user interaction to allow memory dump
    printf("=========================================================\n");
    printf("Memory dump ready - press ENTER to cleanse keys and exit.\n");
    printf("=========================================================\n");
    
    // Read input to keep program alive
    getchar();

    // 10. Clean up keys and liboqs structures safely
    printf("[+] Cleansing memory containing key material...\n");
    OQS_MEM_cleanse(public_key, public_key_len);
    OQS_MEM_cleanse(secret_key, secret_key_len);

    printf("[+] Freeing memory allocations...\n");
    free(public_key);
    free(secret_key);

    OQS_KEM_free(kem);
    OQS_destroy();

    printf("[+] Cleanup complete. Exiting.\n");
    return EXIT_SUCCESS;
}
