#make i686=true MODE=release -j 4

#make -f Makefile2 i686=true MODE=release -j 4

#COPY DLL's
BINDIR_VOIP="./bin/release32"
#BINDIR_GENERATOR="./bin/generator/release32"
#BINDIR_RESOURCEMNG="./bin/resourcemng/release32"

#ntldd $BINDIR_VOIP/play.exe
#ntldd $BINDIR_VOIP/play.exe | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_VOIP/ || true
ntldd $BINDIR_VOIP/*.dll | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_VOIP/ || true

#ntldd $BINDIR_GENERATOR/play.exe | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_GENERATOR/ || true
#ntldd $BINDIR_GENERATOR/*.dll | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_GENERATOR/ || true

#ntldd $BINDIR_RESOURCEMNG/play.exe | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_RESOURCEMNG/ || true
#ntldd $BINDIR_RESOURCEMNG/*.dll | sed 's,\\,/,g' | grep -oP "(?<= => )C:/.*(?=\()" | grep -vE "C:/(Windows|WINDOWS|Windows)/?|\.\.?$$" | awk '{print $$3}' | sed 's/[[:space:]]*$//' | xargs -I '{}' cp -uf '{}' $BINDIR_RESOURCEMNG/ || true

./copydlls.sh DLL_DIR=$BINDIR_VOIP
#./copydlls.sh DLL_DIR=$BINDIR_GENERATOR
#./copydlls.sh DLL_DIR=$BINDIR_RESOURCEMNG

upx $BINDIR_VOIP/crmd.dll