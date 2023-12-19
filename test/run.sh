#!/bin/sh

IN="${0%/*}"
OUT="$IN"/out
EXE="${1:?}"

rm -rf "$OUT"
mkdir "$OUT"

ck() {
	if "$@"; then
		echo PASS
	else
		echo "FAIL: $*" >&2
		exit 1
	fi
}

ck "$EXE" "$IN"/1.tgs  "$OUT"/1.png png
ck "$EXE" "$IN"/2.tgs  "$OUT"/2.png png  1000x1000
ck "$EXE" "$IN"/3.tgs  "$OUT"/3.png png  8000x8000 100
ck "$EXE" "$IN"/4.tgs  "$OUT"/4_    pngs 100x100   5
ck "$EXE" -            "$OUT"/5.gif gif  100x100   25  < "$IN"/5.json
ck "$EXE" "$IN"/6.json -            gif  200x200   50  > "$OUT"/6.gif
ck "$EXE" -            -            gif  500x500   100 > "$OUT"/7.gif < "$IN"/7.json
