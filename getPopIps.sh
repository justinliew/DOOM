#!/usr/bin/env bash
exitOnError() { # $1=ErrorMessage $2=PrintUsageFlag
    local TMP="$(echo "$0" | sed -E 's#^.*/##')"
    [ "$1" != "" ] && >&2 echo "ERROR: $1"
    [[ "$1" != "" && "$2" != "" ]] && >&2 echo ""
    [ "$2" != "" ] && {
        >&2 echo -e "USAGE: $TMP [OPTIONS]"
        >&2 echo -e "\nOPTIONS:"
        >&2 echo -e "$(grep -e "#OPTION[=]" $0 | sed -E -e 's/^[^-]*/  /g' -e 's/\)//' -e 's/#[^=]*=/: /' -e 's/\|/ | /')"
        >&2 echo -e "\nEXAMPLES:"
        >&2 echo -e "  $((++I))) Get TLS offset number from specified domain name:"
        >&2 echo -e "       ${TMP} -d www.fastly.com"
        >&2 echo -e "  $((++I))) Get POP names only:"
        >&2 echo -e "       ${TMP} -a FASTLY_API_KEY"
        >&2 echo -e "  $((++I))) Get POP IPs from specified TLS offset number:"
        >&2 echo -e "       ${TMP} -a FASTLY_API_KEY -o 313"
        >&2 echo -e "  $((++I))) Get POP IPs from TLS offset number derived from specified domain name:"
        >&2 echo -e "       ${TMP} -a FASTLY_API_KEY -d www.fastly.com"
    }
    exit 1
}

debugPrint() {
    >&2 echo -e "DEBUG: $1"
}

toolCheck() { # $1=TOOL_NAME
    type -P "$1" >/dev/null 2>&1 || exitOnError "Dependency failure - \"$1\" required and not found"
}

getArgs() {
    POSITIONAL=()
    # Defaults:
    #FILTER=".+"
    #
    while [[ $# -gt 0 ]]
    do
        key="$(echo "[$1]" | tr '[:upper:]' '[:lower:]' | sed -E 's/^.(.*).$/\1/')"
        case "$key" in
            -a|--api_key) #OPTION=Fastly API Key
                API_KEY="$2"
                shift; shift
                ;;
            -d|--domain_name) #OPTION=Domain name
                DOMAIN="$2"
                shift; shift
                ;;
            -o|--tls_offset) #OPTION=TLS offset number
                OFFSET="$2"
                [[ "${OFFSET}" =~ ^[1-9][0-9]*$ ]] || exitOnError "TLS offset number must be a non-zero integer"
                shift; shift
                ;;
            -f|--pop_filter) #OPTION=POP regex filter
                FILTER="$2"
                shift; shift
                ;;
            --debug) #HIDDEN_OPTION
                DEBUG="1"
                shift
                ;;
            *)
                [[ "$1" =~ ^- ]] && exitOnError "Unexpected argument - \"$1\"" "1"
                POSITIONAL+=("$1")
                shift
                ;;
        esac
    done
    # Checks:
    [[ "${#POSITIONAL[@]}" -ne 0 ]] &&  exitOnError "No arguments expected, ${#POSITIONAL[@]} found" "1"
}

extraArgCheck() { # $1=ARG_NAME / $2=ARG_VALUE
    [[ "$2" != "" ]] && >&2 echo "INFO: \"$1\" argument not applicable and ignored"
}

identifyMode() {
    [[ "${DOMAIN}" != "" && "${API_KEY}" == "" ]] && {
        MODE="GET_OFFSET_FROM_DOMAIN"
        extraArgCheck "-o" "${OFFSET}"; OFFSET=""
        extraArgCheck "-f" "${FILTER}"; FILTER=""
        return
    }
    [[ "${API_KEY}" != "" && "${DOMAIN}${OFFSET}" == "" ]] && {
        MODE="GET_POP_NAMES_ONLY"
        return
    }
    [[ "${API_KEY}" != "" && "${DOMAIN}" != "" && "${OFFSET}" == "" ]] && {
        MODE="GET_POP_IPS_FROM_DOMAIN"
        return
    }
    [[ "${API_KEY}" != "" && "${OFFSET}" != ""  && "${DOMAIN}" == "" ]] && {
        MODE="GET_POP_IPS_FROM_OFFSET"
        return
    }
    [[ "${API_KEY}" != "" && "${DOMAIN}${OFFSET}" != "" ]] && {
        exitOnError "To get POP IPs, specify either the \"-d\" or \"-o\" argument, but not both"
    }
    exitOnError "" 1 # Exit and print USAGE
}

getOffset() {
    local IP="$(nslookup "${DOMAIN}" | grep -m1 -E "^Address: [^ ]" | grep -oE "\d+\.\d+\.\d+\.\d+")"
    [[ "${IP}" == "" ]] && exitOnError "Domain \"${DOMAIN}\" could not be resolved"
    [[ "${DEBUG}" != "" ]] && debugPrint "IP = ${IP}"
    OFFSET="$(nslookup "${IP}" ns1.fastly.cloud | grep -m1 -oE "in-addr\.arpa\s+name = \d+" | grep -oE "\d+")"
    [[ "${OFFSET}" == "" ]] && exitOnError "Offset not found"
    [[ "${DEBUG}" != "" ]] && debugPrint "OFFSET = ${OFFSET}"
}

getPopInfo() {
    POP_INFO="$(curl -s -H "fastly-key: $API_KEY" https://api.fastly.com/datacenters)"
    [[ "${POP_INFO}" =~ ^\[.*\]$ ]] || exitOnError "Unexpected result returned from curl\n${POP_INFO}"
}

processPops() {
    [[ "$MODE" == "GET_POP_NAMES_ONLY" ]] && local TMP="\"\\(.code) = \\(.name)\"" || local TMP=".code"
    while IFS= read -r POP; do
        [[ "$(echo "${POP}" | grep -E "${FILTER}")" == "" ]] && continue
        [[ "$MODE" == "GET_POP_NAMES_ONLY" ]] && {
            echo "$POP"
            continue
        }
        DOMAIN="${OFFSET}.${POP}.global.fastly.cloud"
        IP="$(nslookup ${DOMAIN} | grep -oE "Address: \d+\.\d+\.\d+\.\d+" | sed -e 's/.* //')"
        [[ "${IP}" == "" ]] && IP="Domain \"${DOMAIN}\" did not resolve"
        echo "${POP} = ${IP}"
    done <<< "$(echo "${POP_INFO}" | jq -r ".[]|select(.private != true)|$TMP")"
}
#
#
#
toolCheck "curl"
getArgs "$@"
identifyMode
[[ "$DEBUG" != "" ]] && debugPrint "MODE = $MODE"
[[ "$MODE" == "GET_OFFSET_FROM_DOMAIN" || "$MODE" == "GET_POP_IPS_FROM_DOMAIN" ]] && getOffset
[[ "$MODE" != "GET_OFFSET_FROM_DOMAIN" ]] && getPopInfo
case "${MODE}" in
    GET_OFFSET_FROM_DOMAIN)
        echo "${OFFSET}"
        ;;
    GET_POP_NAMES_ONLY|GET_POP_IPS_FROM_DOMAIN|GET_POP_IPS_FROM_OFFSET)
        processPops
        ;;
    *)
        exitOnError "Unknown MODE \"${MODE}\""
esac