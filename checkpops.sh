#!/usr/bin/env bash
[[ $# -ne 1 ]] && { echo "Domain name, please"; exit 1; }
DOMAIN="$1"
RE="^([A-Z]{3}) = ([1-9]+\.[0-9]+\.[0-9]+\.[0-9]+)$"
while read -r LINE
do
  [[ "$LINE" =~ $RE ]] && {
    POP="${BASH_REMATCH[1]}"
    IP="${BASH_REMATCH[2]}"
    STATUS="$(curl -ski "https://$IP" -H "host: $DOMAIN" | tr -d "\r" | sed -e 's/ $//' | head -1)"
    echo "${POP} = ${STATUS}"
  } || {
    echo "ERROR: Bad data \"$LINE\""
  }
done < /dev/stdin