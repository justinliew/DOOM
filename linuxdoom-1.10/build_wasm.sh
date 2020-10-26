#!/bin/sh

emcc -DHEADLESS *.c -o index.html --preload-file data