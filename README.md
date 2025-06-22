# lemonos-prerelease
limited LemonOS prerelease (cleaned up LemonOS occasionally receiving updates from private dev repo)

# how to use
type `make`

# how to compile your own binaries
get `https://github.com/lemonorangelime/lemonos-toolchain` then do `i386-gcc-lemonos FILE.c -o MY_BINARY`
to get these on LemonOS you must make a ram disk `tar -cf 1.tar MY_BINARY terminal` (you should throw in the terminal binary from `https://github.com/lemonorangelime/lemonos-coreutils/tree/main/terminal` so you can, yknow, run it)
