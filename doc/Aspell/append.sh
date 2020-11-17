#!/bin/bash
if [[ $(cat "$1" | wc -l) -gt 1 ]]; then
	cat "$1" >> "$2"
fi
