#!/bin/bash

main() {
    if [[ $EUID -ne 0 ]]
    then
        echo "Must be root to flush cache"
        return
    fi

    local program=${1:?missing program name}
    local trace_dir=${2:?missing trace directory}
    local max_threads=${3:-8}
    local args=
    local out=
    local ms=

    local separator="--------------------------------------------------------------------------------"
    local blue='\033[0;34m'
    local red='\033[0;31m'
    local cyan='\033[0;36m'
    local NC='\033[0m'

    for analysis in count cpu io
    do
        out=${analysis}.csv
        echo "threads,cache_cold,time" > $out
        local t=
        for (( t=1; t<=max_threads; t=t*2 ))
        do
            echo -e "${cyan}Testing $analysis analysis with $t threads${NC}"
            args="--analysis $analysis --thread $t --type parallel --benchmark --balanced"
            echo -e "${blue}Cache cold${NC}"
            ms=$(./cache_cold.sh $program $args $trace_dir | awk '/Analysis time/{ print $NF; }')
            echo -e "$ms ms"
            echo "$t,1,$ms" >> $out
            echo -e "${red}Cache hot${NC}"
            ms=$($program $args $trace_dir | awk '/Analysis time/{ print $NF; }')
            echo -e "$ms ms"
            echo "$t,0,$ms" >> $out
            echo $separator
        done
    done
}

main $@

