#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define BYTES_PER_WORD 4

struct cache {
    unsigned char valid;
    unsigned char dirty;
    unsigned int content;
};

struct list_entry {
    unsigned int way;
    struct list_entry *prev;
    struct list_entry *next;
};

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize) {
    printf("Cache Configuration:\n");
    printf("-------------------------------------\n");
    printf("Capacity: %dB\n", capacity);
    printf("Associativity: %dway\n", assoc);
    printf("Block Size: %dB\n", blocksize);
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat                                 */
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
        int reads_hits, int write_hits, int reads_misses, int write_misses) {
    printf("Cache Stat:\n");
    printf("-------------------------------------\n");
    printf("Total reads: %d\n", total_reads);
    printf("Total writes: %d\n", total_writes);
    printf("Write-backs: %d\n", write_backs);
    printf("Read hits: %d\n", reads_hits);
    printf("Write hits: %d\n", write_hits);
    printf("Read misses: %d\n", reads_misses);
    printf("Write misses: %d\n", write_misses);
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/*                                                             */
/* Cache Design                                                */
/*                                                             */
/*          cache[set][assoc][word per block]                  */
/*                                                             */
/*                                                             */
/*       ----------------------------------------              */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*                                                             */
/*                                                             */
/***************************************************************/
void xdump(int set, int way, struct cache **cache) {
    int i, j, k = 0;

    printf("Cache Content:\n");
    printf("-------------------------------------\n");
    for (i = 0; i < way; i++) {
        if (i == 0) {
            printf("    ");
        }
        printf("      WAY[%d]", i);
    }
    printf("\n");

    for (i = 0; i < set; i++) {
        printf("SET[%d]:   ", i);
        for (j = 0; j < way; j++) {
            if (k != 0 && j == 0) {
                printf("          ");
            }
            printf("0x%08x  ", cache[i][j].content);
        }
        printf("\n");
    }
    printf("\n");
}

void usage(char *program) {
    printf("Usage: %s [-c cap:assoc:bsize] [-x] input_trace\n", program);
    exit(1);
}

int main(int argc, char *argv[]) {
    struct cache **cache;
    struct list_entry **access_list;
    char *filename;
    FILE *program;
    char buffer[33];
    char operation;
    unsigned int address;

    int i, j;
    int opt;
    int tmp;

    int capacity;
    int way;
    int blocksize;
    int set;
    int words;
    int index_size;

    int cache_config_set = 0;
    int cache_dump_set = 0;

    int total_reads = 0;
    int total_writes = 0;
    int write_backs = 0;
    int reads_hits = 0;
    int write_hits = 0;
    int reads_misses = 0;
    int write_misses = 0;

    // Argument check
    if (argc < 2) {
        usage(argv[0]);
    }

    while ((opt = getopt(argc, argv, "c:x")) != -1) {
        switch (opt) {
            case 'c':
                cache_config_set = 1;
                if (sscanf(optarg, "%d:%d:%d",
                            &capacity, &way, &blocksize) != 3) {
                    usage(argv[0]);
                }
                if (capacity < 4 || capacity > 8192) {
                    printf("Capacity should be between 4B and 8KB.\n");
                    exit(1);
                }
                if (way < 1 || way > 16) {
                    printf("Associativity should be between 1 and 16.\n");
                    exit(1);
                }
                if (blocksize < 4 || blocksize > 32) {
                    printf("Block size should be between 4B and 32B.\n");
                    exit(1);
                }
                set = capacity / way / blocksize;
                words = blocksize / BYTES_PER_WORD;

                tmp = set / 2;
                index_size = 0;
                while (tmp > 0) {
                    index_size++;
                    tmp >>= 1;
                }
                break;
            case 'x':
                cache_dump_set = 1;
                break;
            case '?':
                usage(argv[0]);
        }
    }
    if (!cache_config_set) {
        printf("The cache parameters are specified with \"-c\" option.\n");
        exit(1);
    }
    if (argc == optind) {
        usage(argv[0]);
    }
    filename = argv[optind];

    // allocate
    cache = (struct cache **) malloc(sizeof(struct cache *) * set);
    access_list = (struct list_entry **)
        malloc(sizeof(struct list_entry *) * set);
    for (i = 0; i < set; i++) {
        struct list_entry *entry;
        struct list_entry *prev_entry = NULL;

        cache[i] = (struct cache *) malloc(sizeof(struct cache) * way);
        for (j = 0; j < way; j++) {
            cache[i][j].valid = 0;
            cache[i][j].dirty = 0;
            cache[i][j].content = 0x0;
        }

        entry = (struct list_entry *) malloc(sizeof(struct list_entry));
        entry->way = 0;
        entry->prev = prev_entry;
        access_list[i] = entry;
        for (j = 1; j < way; j++) {
            entry->next =
                (struct list_entry *) malloc(sizeof(struct list_entry));
            prev_entry = entry;
            entry = entry->next;
            entry->way = j;
            entry->prev = prev_entry;
        }
        entry->next = NULL;
    }

    // Open program file.
    program = fopen(filename, "r");
    if (program == NULL) {
        printf("Error: Can't open program file \"%s\"\n", filename);
        exit(-1);
    }

    while (fgets(buffer, 33, program) != NULL) {
        unsigned int entry;
        unsigned int index;
        int _way;
        struct cache *result_cache;
        struct list_entry *list_item;
        int hit;
        int invalid_exist;

        sscanf(buffer, "%c %x", &operation, &address);

        entry = address & ~(blocksize - 1);
        index = (address / blocksize) & ((1 << index_size) - 1);
        hit = 0;
        for (i = 0; i < way; i++) {
            if (cache[index][i].valid && cache[index][i].content == entry) {
                hit = 1;
                _way = i;
                result_cache = &cache[index][i];

                list_item = access_list[index];
                while (list_item->way != i) {
                    list_item = list_item->next;
                }
                if (list_item->prev != NULL) {
                    list_item->prev->next = list_item->next;
                    if (list_item->next != NULL) {
                        list_item->next->prev = list_item->prev;
                    }
                    access_list[index]->prev = list_item;
                    list_item->prev = NULL;
                    list_item->next = access_list[index];
                    access_list[index] = list_item;
                }
                break;
            }
        }
        if (!hit) {
            struct cache *evict_cache;

            list_item = access_list[index];
            invalid_exist = 0;
            for (i = 0; i < way; i++) {
                if (!cache[index][i].valid) {
                    invalid_exist = 1;
                    evict_cache = &cache[index][i];
                    _way = i;
                    while (list_item->way != i) {
                        list_item = list_item->next;
                    }
                    break;
                }
            }
            if (!invalid_exist) {
                for (i = 1; i < way; i++) {
                    list_item = list_item->next;
                }
                evict_cache = &cache[index][list_item->way];
                _way = list_item->way;
            }
            if (evict_cache->dirty) {
                write_backs++;
            }
            evict_cache->valid = 1;
            evict_cache->dirty = 0;
            evict_cache->content = entry;
            result_cache = evict_cache;

            if (list_item->prev != NULL) {
                list_item->prev->next = list_item->next;
                if (list_item->next != NULL) {
                    list_item->next->prev = list_item->prev;
                }
                access_list[index]->prev = list_item;
                list_item->prev = NULL;
                list_item->next = access_list[index];
                access_list[index] = list_item;
            }
        }

        switch (operation) {
            case 'R':
                total_reads++;
                hit ? reads_hits++ : reads_misses++;
                break;
            case 'W':
                result_cache->dirty = 1;
                total_writes++;
                hit ? write_hits++ : write_misses++;
                break;
        }
    }

    cdump(capacity, way, blocksize);
    sdump(total_reads, total_writes, write_backs,
            reads_hits, write_hits, reads_misses, write_misses);
    if (cache_dump_set) {
        xdump(set, way, cache);
    }

    for (i = 0; i < set; i++) {
        free(cache[i]);
    }
    free(cache);

    return 0;
}
