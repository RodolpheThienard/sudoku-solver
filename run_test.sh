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
# echo -e "  ${LEXIC}SYNTAX${NC} : Comparaison de la syntaxe générée avec celui du lab1"
# echo -e "  ${LEXIC}COMPIL${NC} : Vérification de la compilation en asm"
# echo -e "     ${LEXIC}RUN${NC} : Exécution et vérification du code de sortie (42)"
echo -e "   ${RED}ERROR${NC} : Le test échoue"
echo -e "      ${GREEN}OK${NC} : Le test réussi"
echo -e "  ${GRAY}ESCAPE${NC} : Le test n'est pas effectué \n"
printf "%17s\t%5s\n" "TEST FILE NAME" "TODO"
printf '_%.0s' {1..50}
printf '\n'

name(){
    name_file=$(echo "$name" | cut -f 2 -d "/")
    # printable_name=$(echo "${name_file}" |cut -c -10)
    printf "%17s\t" $name_file
    # echo -e -n "$printable_name \t"
}

output(){
    name
    test
    # error
}

test(){
    for sudoku in build/sudoku*
    do
        if [ -f sudoku ] 
        then
            build/sudoku $file_name
        else 
            no_comp
        fi
    done
}

ok(){
    echo -e -n "${GREEN}OK${NC}\t"
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


total(){
    printf "%17s\t%2s/%2s\t%2s/%2s\t%2s/%2d\n" "TOTAL" "$syntax" "$file_amount" "$compil" $(("$file_amount - $escape_compil")) "$run" $(("$file_amount - $escape_run"))
}

for file in data/*.data
do
    if [ -f $file ]
    then
        name=${file%.*}
        output
    else
        echo -e "${RED}No test${NC}"
    fi
    echo -e ""
    let file_amount++
done
# total

