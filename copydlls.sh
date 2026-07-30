#!/bin/bash

# Diretório onde estão localizados os arquivos DLL
#DLL_DIR="./bin/launcher/release32"

# Diretório de destino para as DLLs filtradas
#DEST_DIR="./bin/launcher/release32"

DEST_DIR=$DLL_DIR

COPIED="false"

convert_windows_path_to_gitbash() {
    # Substituir barras invertidas por barras normais
    gitbash_path=$(echo "$1" | sed 's/\\/\//g')
    
    # Adicionar a unidade do disco (letra da unidade) se não estiver presente
    if [[ ! $gitbash_path =~ ^[A-Za-z]:.*$ ]]; then
        gitbash_path="/$gitbash_path"
    fi
    
    echo "$gitbash_path"
}

# Lista das DLLs geradas pelo comando objdump
DLL_LIST=$(objdump -p "$DLL_DIR"/*.* | grep "DLL Name" | awk '{print $3}')

# Obter lista de DLLs na pasta
DLLS_IN_FOLDER=$(basename -a "$DLL_DIR"/*.*)

# Remover DLLs da lista que estão na pasta
DLL_LIST=$(comm -23 <(echo "$DLL_LIST" | sort) <(echo "$DLLS_IN_FOLDER" | sort))

echo "$DLL_LIST"


# Iterar sobre a lista de DLLs geradas e copiar para o diretório de destino
for DLL_NAME in $DLL_LIST; do
    # Remover o prefixo "DLL Name: " de cada linha
    DLL_NAME=${DLL_NAME#"DLL Name: "}

    #DLL_PATH=$(which "$DLL_NAME")

    DLL_PATH=""
    DLL_TYPE=""
    DLL_PATH_CHECKS=$(where $DLL_NAME)
    DLL_PATH_CHECK=$(convert_windows_path_to_gitbash "$DLL_PATH_CHECKS")

    for DLL_PATH_TEST in $DLL_PATH_CHECK; do

        # Use objdump to get file format info and grep for PE32 (32-bit)
        DLL_TYPE=$(objdump -p -f "$DLL_PATH_TEST" | grep "file format")
        DLL_TYPE=$(echo "$DLL_TYPE" | sed 's/.*file format//')

        if [[ "$DLL_TYPE" == *"32"* ]]; then
            DLL_PATH=$DLL_PATH_TEST
            echo "32"
            break
        fi
        if [[ "$DLL_TYPE" == *"386"* ]]; then
            DLL_PATH=$DLL_PATH_TEST
            echo "386"
            break
        fi
        if [[ "$DLL_TYPE" == *"686"* ]]; then
            DLL_PATH=$DLL_PATH_TEST
            echo "686"
            break
        fi
    done

    if [ "$DLL_PATH" = "" ]; then
        echo $DLL_PATH_CHECKS
        echo $DLL_PATH_CHECK
        echo $DLL_NAME
        echo $DLL_TYPE
        continue
    fi
    
    # DLL do sistema que não deve ser copiada?
    SYSTEM_DLL=$(echo "$DLL_PATH" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $NF}')

    # Verificar se a DLL não é uma DLL do sistema
    if [[ " ${SYSTEM_DLL} " == " ${DLL_PATH} " ]]; then
        if [ -n "$DLL_PATH" ]; then
            if [ -f "$DEST_DIR/$DLL_NAME" ]; then
                echo "Aviso: $DLL_NAME já existe no diretório de destino."
            else
                cp "$DLL_PATH" "$DEST_DIR"
                echo "Copiado: $DLL_NAME"
                COPIED="true"
            fi
        else
            echo "DLL não encontrada: $DLL_NAME"
        fi
    else
        echo "Ignorado (DLL do sistema): $DLL_NAME"
    fi
done

echo $COPIED

if [ "$COPIED" = "false" ]; then
    echo "Concluído!"
else
    echo "Again!"
fi