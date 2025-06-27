# lemonos-prerelease
limited LemonOS prerelease

# warning
a lot of things are broken or unusable (such as gpu drivers which you cant actually use to do anything at the moment), so dont expect anything amazing

# how to use
type `make`

## sysrq
LemonOS uses super + sysrq + (character) to call a sysrq, an example is super + sysrq + i (print CPU info)

## keyboard layouts
you can press the meta key to switch between keyboard layers (currently layer 1 is qwerty english and layer 2 is qwerty greek)

# how to compile your own binaries
get `https://github.com/lemonorangelime/lemonos-toolchain` then do `i386-gcc-lemonos FILE.c -o MY_BINARY`

to get these on LemonOS you must make a ram disk `tar -cf 1.tar MY_BINARY terminal` (you should throw in the terminal binary from `https://github.com/lemonorangelime/lemonos-coreutils/tree/main/terminal` so you can, yknow, run it)
