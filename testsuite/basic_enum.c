int printf(char *fmt, ...);

int main()
{
    enum foobar {
        MEMA,
        MEMB,
        MEMC,
        MEMD
    };

    enum foobar foob;
    foob = MEMD;

    printf("%d %d %d %d\n", MEMA, MEMB, MEMC, MEMD);
    printf("%d\n", foob);
}
