#!/bin/sh

# Diretório contendo os arquivos de entrada (por exemplo, EXEs ou DLLs)
DLL_DIR="./bin/release32"
# Diretório de destino onde as DLLs serão copiadas
DEST_DIR="./bin/release32"

# Cria o diretório de destino, se não existir
mkdir -p "$DEST_DIR"

# Extrai todas as DLLs necessárias e copia para o DEST_DIR
objdump -p "$DLL_DIR"/*.* 2>/dev/null | \
grep "DLL Name" | awk '{print $3}' | sort -u | while read -r dllname; do
    # Encontra o caminho absoluto da DLL no sistema
    dllpath=$(which "$dllname" 2>/dev/null)
    
    if [ -z "$dllpath" ]; then
        # Tenta localizar a DLL manualmente (caso esteja no System32 ou diretórios conhecidos)
        for dir in /usr/bin /mingw*/bin /c/Windows/System32 /c/Windows/SysWOW64; do
            if [ -f "$dir/$dllname" ]; then
                dllpath="$dir/$dllname"
                break
            fi
        done
    fi

    if [ -n "$dllpath" ]; then
        echo "Copying $dllname from $dllpath"
        cp "$dllpath" "$DEST_DIR/"
    else
        echo "Warning: DLL not found: $dllname"
    fi
done
