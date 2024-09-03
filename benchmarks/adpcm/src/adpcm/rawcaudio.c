/* testc - Test adpcm coder */

#include "adpcm.h"
#include "data_pcm.h"
#include <stdio.h>
#include <string.h>

struct adpcm_state state;

#define NSAMPLES 1000

char	abuf[NSAMPLES/2];
short	sbuf[NSAMPLES];

#define MIN(a,b) (((a)<(b))?(a):(b))

int main() {
	int num_bytes_processed = 0;
    int n;
	int process_bytes;

    while(1) {
		printf("Final valprev=%d, index=%d\n", state.valprev, state.index);
		n = PCM_DATA_LENGTH - num_bytes_processed;
		if ( n == 0 ) break;

		process_bytes = MIN(NSAMPLES*2, n);

		memcpy(sbuf, pcm_data + num_bytes_processed, process_bytes);
		num_bytes_processed += process_bytes;
		
		adpcm_coder(sbuf, abuf, process_bytes/2, &state);


		// n = read(0, sbuf, NSAMPLES*2);
		// if ( n < 0 ) {
		// 	perror("input file");
		// 	exit(1);
		// }
		// if ( n == 0 ) break;
		// adpcm_coder(sbuf, abuf, n/2, &state);
		// write(1, abuf, n/4);
    }

    printf("Final valprev=%d, index=%d\n", state.valprev, state.index);
    return 0;
}
