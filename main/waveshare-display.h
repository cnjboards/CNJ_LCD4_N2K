// this is written in C++, so we have to add this stuff to make it callable from C:
#ifdef __cplusplus
extern "C" {
#endif

    void doLvglInit(void);
    void processDisplay(void);

#ifdef __cplusplus
}
#endif
