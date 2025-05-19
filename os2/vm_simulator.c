#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define PAGE_SIZE 2048
#define OFFSET_BITS 11
#define TLB_SIZE 4
#define FRAME_COUNT 1024

typedef struct {
    uint32_t page_number;
    uint32_t frame_number;
    uint32_t last_used;
    bool valid;
} TLBEntry;

TLBEntry tlb[TLB_SIZE];
uint32_t time_counter = 0;
uint32_t tlb_hits = 0, tlb_misses = 0;
FILE *output;


void print_both(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    vfprintf(output, format, args);
    va_end(args);
}


uint32_t get_frame_number(uint32_t page_number) {
    page_number ^= (page_number >> 16);
    return (page_number * 701 + 29) % FRAME_COUNT;
}

int search_TLB(uint32_t page_number, uint32_t *frame_number) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page_number == page_number) {
            *frame_number = tlb[i].frame_number;
            tlb[i].last_used = ++time_counter;
            return 1;
        }
    }
    return 0;
}

void insert_TLB(uint32_t page_number, uint32_t frame_number) {
    int lru_index = 0;
    uint32_t min_time = ~0U;

    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            lru_index = i;
            break;
        }
        if (tlb[i].last_used < min_time) {
            min_time = tlb[i].last_used;
            lru_index = i;
        }
    }

    tlb[lru_index].page_number = page_number;
    tlb[lru_index].frame_number = frame_number;
    tlb[lru_index].last_used = ++time_counter;
    tlb[lru_index].valid = true;
}

void translate_address(uint32_t virtual_address) {
    uint32_t offset = virtual_address & (PAGE_SIZE - 1);
    uint32_t page_number = virtual_address >> OFFSET_BITS;
    uint32_t frame_number;

    print_both("Virtual address: %u\n", virtual_address);
    print_both("Page number: %u\n", page_number);
    print_both("Offset: %u\n", offset);

    if (search_TLB(page_number, &frame_number)) {
        print_both("TLB: Hit\n");
        tlb_hits++;
    } else {
        print_both("TLB: Miss\n");
        tlb_misses++;
        frame_number = get_frame_number(page_number);
        insert_TLB(page_number, frame_number);
    }

    uint32_t physical_address = (frame_number << OFFSET_BITS) | offset;
    print_both("Frame number: %u\n", frame_number);
    print_both("Physical address: %u\n", physical_address);
    print_both("--------------------------\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s addresses.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }

    output = fopen("results.txt", "w");
    if (!output) {
        perror("Error creating output file");
        fclose(fp);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < TLB_SIZE; i++)
        tlb[i].valid = false;

    uint32_t address;
    while (fscanf(fp, "%u", &address) == 1) {
        translate_address(address);
    }

    print_both("=== TLB Summary ===\n");
    print_both("TLB Hits: %u\n", tlb_hits);
    print_both("TLB Misses: %u\n", tlb_misses);
    print_both("TLB Hit Rate: %.2f%%\n", 100.0 * tlb_hits / (tlb_hits + tlb_misses));

    fclose(fp);
    fclose(output);
    return EXIT_SUCCESS;
}
