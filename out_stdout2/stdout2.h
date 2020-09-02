//
// Created by darren on 28/8/20.
//

#ifndef FLUENT_BIT_PLUGINS_EXT_STDOUT2_H
#define FLUENT_BIT_PLUGINS_EXT_STDOUT2_H

#include <fluent-bit/flb_output_plugin.h>
#include <fluent-bit/flb_sds.h>


struct flb_stdout2 {
    int out_format;
    int json_date_format;
    flb_sds_t json_date_key;
    flb_sds_t date_key;
    struct flb_output_instance *ins;
};

#endif //FLUENT_BIT_PLUGINS_EXT_STDOUT2_H
