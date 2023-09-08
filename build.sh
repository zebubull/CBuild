mkdir -p obj/linux/debug/core
mkdir -p obj/linux/debug/mem
mkdir -p obj/linux/debug/os
mkdir -p obj/linux/debug/util

gcc -c src/main.c -DDebug -o obj/linux/debug/main.o
gcc -c src/util/cbstr.c -DDebug -o obj/linux/debug/util/cbstr.o
gcc -c src/util/cbsplit.c -DDebug -o obj/linux/debug/util/cbsplit.o
gcc -c src/util/cbtimetable.c -DDebug -o obj/linux/debug/util/cbtimetable.o
gcc -c src/mem/cbmem.c -DDebug -o obj/linux/debug/mem/cbmem.o
gcc -c src/os/dir.c -DDebug -o obj/linux/debug/os/dir.o
gcc -c src/core/cbconf.c -DDebug -o obj/linux/debug/core/cbconf.o
gcc -c src/core/cbcore.c -DDebug -o obj/linux/debug/core/cbcore.o

gcc -o cbuild.out obj/linux/debug/main.o obj/linux/debug/core/cbcore.o obj/linux/debug/core/cbconf.o obj/linux/debug/mem/cbmem.o obj/linux/debug/os/dir.o obj/linux/debug/util/cbsplit.o obj/linux/debug/util/cbstr.o obj/linux/debug/util/cbtimetable.o