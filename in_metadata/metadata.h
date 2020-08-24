#ifndef CAPM_METADATA_H
#define CAPM_METADATA_H


#define IS_POWER  1
#define IS_AMD64 2
#define IS_ARM 4
#define IS_POWER_VM  8

/* CPU Input configuration & context */
typedef struct flb_metadata {
    /* setup */
    int coll_fd;        /* collector id/fd            */
    struct flb_input_instance *ins;
    int counter;
}flb_metadata;
#endif
