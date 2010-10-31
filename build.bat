flex proj4.l
hyacc -dlt proj4.y
gcc -w *.c -o compiler -DINFO
compiler source
link source,,,util.lib
source