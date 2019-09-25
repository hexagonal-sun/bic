#include <stdio.h>

int main()
{
    enum foobar {
        MEMA,
        MEMB,
        MEMC,
        MEMD,
        MEME = 1024,
        MEMF,
        MAX = MEMF
    };

    enum barbar {
        BARMEMA,
        BARMEMB,
        BARMEMC,
        BARMEMD,
    };

    enum foobar foob;
    foob = MAX;

    printf("%d %d %d %d\n", MEMA, MEMB, MEMC, MEMD);
    printf("%d %d %d\n", MEME, MEMF, MAX);
    printf("%d\n", foob);

    return 0;
}
