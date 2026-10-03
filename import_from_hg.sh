#!/bin/bash

if [ -z "${PYTHON:-}" ]; then
	for python in python3 python /usr/bin/python3; do
		if command -v "$python" >/dev/null 2>&1 && \
			"$python" -c 'from mercurial.scmutil import revsymbol' >/dev/null 2>&1; then
			PYTHON="$python"
			break
		fi
	done
fi

if [ -z "${PYTHON:-}" ]; then
	echo "Could not find a Python interpreter with the Mercurial module available." >&2
	exit 1
fi

export PYTHON
exec ../fast-export/hg-fast-export.sh -r ~/hg/ostc4/ -A ./authors.txt
