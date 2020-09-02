
set(src metadata.c ../src/kvp.c ../src/fluent_kvp.c ../src/strutils.c)

FLB_PLUGIN(in_metadata "${src}" "")
