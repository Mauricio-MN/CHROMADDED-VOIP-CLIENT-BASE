#!/bin/bash
# Resolve dependências transitivas de DLLs (BFS).
# Uso: DLL_DIR=./bin/launcher/release32 bash copy_dlls.sh

DEST_DIR="${DLL_DIR:?'Defina DLL_DIR antes de rodar'}"

# ── Helpers ─────────────────────────────────────────────────────────────────

win_to_unix() {
    local p="${1//\\//}"
    if [[ "$p" =~ ^([A-Za-z]):(.*)$ ]]; then
        echo "/${BASH_REMATCH[1],,}${BASH_REMATCH[2]}"
    else
        echo "$p"
    fi
}

is_32bit() {
    objdump -f "$1" 2>/dev/null | grep -qiE "pei?-(i386|i686)"
}

is_system_dll() {
    [[ "$1" =~ ^/[a-z]/[Ww]indows/ ]]
}

find_32bit_dll() {
    local raw upath
    raw=$(where "$1" 2>/dev/null) || return 1
    while IFS= read -r wpath; do
        [[ -z "$wpath" ]] && continue
        upath=$(win_to_unix "$wpath")
        is_32bit "$upath" && echo "$upath" && return 0
    done <<< "$raw"
    return 1
}

# ── BFS ─────────────────────────────────────────────────────────────────────

declare -A SEEN
COPIED=false

mapfile -t QUEUE < <(find "$DEST_DIR" -maxdepth 1 \( -iname "*.dll" -o -iname "*.exe" \) -printf "%f\n")

while [[ ${#QUEUE[@]} -gt 0 ]]; do
    NEXT_QUEUE=()

    for name in "${QUEUE[@]}"; do
        key="${name,,}"
        [[ -n "${SEEN[$key]}" ]] && continue
        SEEN[$key]=1

        src="$DEST_DIR/$name"
        [[ -f "$src" ]] || continue

        echo "→ Escaneando: $name"

        while IFS= read -r dep; do
            dep_key="${dep,,}"
            [[ -n "${SEEN[$dep_key]}" ]] && continue

            # Já existe na pasta (case-insensitive)?
            found_local=$(find "$DEST_DIR" -maxdepth 1 -iname "$dep" -printf "%f\n" | head -1)
            if [[ -n "$found_local" ]]; then
                NEXT_QUEUE+=("$found_local")
                continue
            fi

            # Procura no PATH
            found_path=$(find_32bit_dll "$dep") \
                || { echo "  ✗ Não encontrada: $dep"; SEEN[$dep_key]=1; continue; }

            if is_system_dll "$found_path"; then
                echo "  ~ Sistema (ignorada): $dep"
                SEEN[$dep_key]=1
                continue
            fi

            cp "$found_path" "$DEST_DIR/$dep"
            echo "  ✔ Copiada: $dep  ←  $found_path"
            COPIED=true
            NEXT_QUEUE+=("$dep")

        done < <(objdump -p "$src" 2>/dev/null | awk '/DLL Name:/{print $3}')
    done

    QUEUE=("${NEXT_QUEUE[@]}")
done

echo ""
$COPIED && echo "Concluído — novas DLLs copiadas." || echo "Nada a copiar."