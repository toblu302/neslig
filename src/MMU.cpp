#include <assert.h>

#include "controller.h"


void dumpRAM() {
    uint16_t address = 0;
    uint16_t count = 0;
    printf("\nDump of RAM:\n");
    for( address=0x3F00; address<=0x3F20; ++address) {
            if( count % 16 == 0 ) {
                printf("\n%X: ", address);
            }
            //printf("%X, ", MMU.VRAM[address]);
            count += 1;
    }
    printf("\n");
}

void dumpSPRRAM() {
    uint16_t address = 0;
    uint16_t count = 0;
    printf("Dump of SPRRAM:\n");
    for( address=0; address<=0xFF; ++address) {
            if( count % 16 == 0 ) {
                printf("\n%X: ", address);
            }
            //printf("%X, ", MMU.OAM[address]);
            count += 1;
    }
    printf("\n");
}

void dumpVRAM() {
    printf("Dump of interesting VRAM:\n");
    uint16_t count = 0;
    uint16_t address = 0;
    for( address=0x2000; address<0x23FF; ++address) {
        if( count % 16 == 0 ) {
            printf("\n%X: ", address);
        }
        //printf("%X, ", MMU.VRAM[address]);
        count += 1;
    }
}
