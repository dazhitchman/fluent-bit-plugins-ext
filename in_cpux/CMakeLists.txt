set(src cpux.c ../src/osutils.c ../src/fluent_kvp.c  ../src/kvp.c ../src/strutils.c ../src/dynlist.c)

FLB_PLUGIN(in_cpux "${src}" "")
