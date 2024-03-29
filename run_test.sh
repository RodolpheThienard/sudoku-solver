#!/bin/bash

err_amount=0
file_amount=0
pass=0
syntax=0
compil=0
run=0
escape_run=0
escape_compil=0

RED='\033[0;91m\033[1m'
GREEN='\033[0;92m\033[1m'
LEXIC='\033[0;95m\033[1m'
GRAY='\033[0;90m\033[1m'
NC='\033[0m' # No Color

echo -e "${LEXIC}INFO :${NC}"
echo -e "   ${RED}ERROR${NC} : Le test échoue"
echo -e " ${GREEN}SUCCESS${NC} : Le test réussi"
echo -e "  ${GRAY}ESCAPE${NC} : Le test n'est pas effectué \n"
printf "%17s\t%7s\t%7s\t%7s\n" "TEST FILE NAME" "CPU" "OMP" "TARGET"
printf '_%.0s' {1..50}
printf '\n'

name(){
    name_file=$(echo "$name" | cut -f 2 -d "/")
    # printable_name=$(echo "${name_file}" |cut -c -10)
    printf "%17s\t" $name_file
    # echo -e -n "$printable_name \t"
}

pipeline(){
    name
    all_check
    # error
}

all_check(){
    cpu
    check_validity
    omp
    check_validity
    target
    check_validity
}

check_validity (){
    if [ "$?" -eq 0 ]; then
        ok
    else
        error
        return 1
    fi
}

cpu(){
        if [ -f build/wfc ] 
        then
            build/wfc -s0-100000 $file -lcpu > /tmp/result
        else 
            no_comp
        fi
}

omp(){
        if [ -f build/wfc ] 
        then
            build/wfc -s0-100000 $file -lomp > /tmp/result
        else 
            no_comp
        fi
}

target(){
        if [ -f build/wfc ] 
        then
            build/wfc -s0-100000 $file -ltarget > /tmp/result
        else 
            no_comp
        fi
}

ok(){
    echo -e -n "${GREEN}SUCCESS${NC}\t"
}
no_comp(){
    echo -e -n "${RED}No Comp${NC}\t"
}
error(){
    echo -e -n "${RED}ERROR${NC}\t"
}
escaper(){
    echo -e -n "${GRAY}ESCAPE${NC}\t"
}


for file in data/*.data
do
    if [ -f $file ]
    then
        name=${file%.*}
        pipeline
    else
        echo -e "${RED}No test${NC}"
    fi
    echo -e ""
    let file_amount++
done

