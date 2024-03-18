#!/bin/bash
# -s [+][-]seek  start at <seek> bytes abs. (or +: rel.) infile offset.
# -u          use upper case hex letters.
# -a          toggle autoskip: A single '*' replaces nul-lines. Default off.
# -l len      stop after <len> octets.
# -g          number of octets per group in normal output. Default 2 (-e: 4).
xxd -u -a -g 1 -s $2 -l $3 $1
