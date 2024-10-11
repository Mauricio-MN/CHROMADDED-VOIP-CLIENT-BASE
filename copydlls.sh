BINDIR_DEBUG="./bin/debug"

ldd $BINDIR_DEBUG/*.exe | sed 's,\\,/,g' | grep -oP "(?<= => )/c/.*(?=\()" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_DEBUG/ || true
ldd $BINDIR_DEBUG/*.dll | sed 's,\\,/,g' | grep -oP "(?<= => )/c/.*(?=\()" | grep -vE "/c/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_DEBUG/ || true

#ntldd $BINDIR_DEBUG/*.exe | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_DEBUG/ || true
#ntldd $BINDIR_DEBUG/*.dll | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_DEBUG/ || true
