function needs_recompile() {
  local OBJ="$1"
  local FILE="$2"
  local DEP="$3"

  if [[ ! -e $OBJ ]]; then
    return 0  # O arquivo de objeto não existe, precisa ser recompilado
  fi

  if [[ $OBJ -ot $FILE || $OBJ -ot $DEP ]]; then
    return 0  # O arquivo de objeto é mais antigo do que o arquivo .cpp ou .d, precisa ser recompilado
  fi

  # Verificar se algum arquivo de cabeçalho listado no arquivo .d foi modificado
  while read -r DEPFILE; do
    if [[ $OBJ -ot $DEPFILE ]]; then
      echo "$DEPFILE att"
      return 0  # O arquivo de objeto é mais antigo do que o arquivo de cabeçalho, precisa ser recompilado
    fi
  done < <(tr '\ ' '\n' < "$DEP")

  return 1  # O arquivo de objeto está atualizado e não precisa ser recompilado
}

EXTNAME=""
COMPILER=""

FOLDER=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
FAILMSG="Failed copy of file: "
OK="Successfull copy of file: "

win (){
    FOLDER=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd -W)
}

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        EXTNAME="so"
        COMPILER="g++"
elif [[ "$OSTYPE" == "darwin"* ]]; then
        EXTNAME="so"
        COMPILER="g++"
elif [[ "$OSTYPE" == "cygwin" ]]; then
        EXTNAME="dll"
        win
        COMPILER="g++"
elif [[ "$OSTYPE" == "msys" ]]; then
        EXTNAME="dll"
        win
        COMPILER="C:/msys64/mingw64/bin/g++.exe"
elif [[ "$OSTYPE" == "win32" ]]; then
        EXTNAME="dll"
        COMPILER="g++"
elif [[ "$OSTYPE" == "freebsd"* ]]; then
        EXTNAME="so"
        COMPILER="g++"
fi


# Lê o argumento passado no formato "NOME=VALOR"
for arg in "$@"
do
    # Verifica se o argumento é do formato "NOME=VALOR"
    if [[ $arg == *"="* ]]; then
        # Extrai o nome e o valor do argumento
        name="${arg%=*}"
        value="${arg#*=}"
        
        if [[ $name == "CLEAN" ]]; then
            if [[ $value == "true" ]]; then
                rm -r $FOLDER/obj/release/*
                rm -r $FOLDER/obj/debug/*
            fi
        fi

    fi
done

#BUILD GNU

TESTARG=""
TEST=$1
BUILDNAME=""
BUILDNAMENOEXT="crmd"
FPIC=""
SHARED=""
G=""
WINLIBS=""

if [[ "$EXTNAME" == "dll" ]]; then
    WINLIBS="-lwsock32
             -lWs2_32"
fi

if [[ "$TEST" == "test" ]]; then
    TESTARG=../test.cpp
    BUILDNAMENOEXT="test"
    if [[ "$EXTNAME" == "dll" ]]; then
        BUILDNAME="../$BUILDNAMENOEXT.exe"
    else
        BUILDNAME="../$BUILDNAMENOEXT"
    fi
    G="-g"
else
    if [[ "$TEST" == "android" ]]; then
        EXTNAME="so"
        COMPILER="g++"
    fi
    BUILDNAME="crmd."$EXTNAME
    #FPIC="-fPIC"
    #SHARED="-shared"
    G=""
fi

BUILDSUCCESS="true"

ENDOBJ="$FOLDER/bin/release/$BUILDNAME"
ENDOBJFOLDER="$FOLDER/bin/release"

LIBS="-lssl -lcrypto -lsfml-system -lsfml-audio -lsfml-window -llibopus -lboost_system-mt"
ARGS="$ARGS -static-libstdc++ -static-libgcc -DSFML_STATIC"

CPPS="connection.cpp
				cript.cpp
				protocol.cpp
				soundmanager.cpp
				player.cpp
				SoundCustomBufferRecorder.cpp
				smbPitchShift.cpp
				opusmanager.cpp
                socketUdp.cpp
                protoParse.cpp
				crmd.cpp
                ./proto/protocol.pb.cc
                $TESTARG"
OBJS=""
# Compilar cada arquivo .cpp/.cc individualmente
COUNT=0
for FILE in $CPPS; do
  OBJ="${FILE%.cpp}.o"
  OBJ+="${FILE%.cc}.o"
  DEP="${FILE%.cpp}.d"
  DEP+="${FILE%.cc}.d"
  OBJ_DIR=$(dirname "$FILE")
  mkdir -p "$FOLDER/obj/release/$OBJ_DIR"
  if needs_recompile "$FOLDER/obj/release/$OBJ" "$FOLDER/$FILE" "$FOLDER/obj/release/$DEP"; then
    echo "Compile $OBJ"
    #{
    $COMPILER -c $G -Wall -std=c++20 -MMD -MP $ARGS $FPIC $WINLIBS $LIBS $FOLDER/$FILE -o $FOLDER/obj/release/$OBJ
    if [ $? -ne 0 ]; then
        BUILDSUCCESS="false"
        echo "BUILD FAILED"
        exit;
    fi
    #} &

  else
    echo "$OBJ OK"
  fi
  OBJS="$OBJS $FOLDER/obj/release/$OBJ"

  COUNT+=1
  if [[ $COUNT == 10 ]]; then
    #wait
    $COUNT=0
  fi
done

wait

# Construir a biblioteca compartilhada final
if [[ "$BUILDSUCCESS" == "false" ]]; then
    echo "BUILD FAILED"
    exit
fi

CMDBUILD="$COMPILER $G -std=c++20 $ARGS $OBJS $SHARED $FPIC $WINLIBS $LIBS -o $ENDOBJ"

echo $CMDBUILD
eval $CMDBUILD
if [ $? -eq 0 ]; then
    echo ""
else
    echo "BUILD FAILED"
    BUILDSUCCESS="false"
fi

#INCLUDE


check () {
    if [ $? -eq 0 ]; then
        echo $OK $1
    else
        echo $FAILMSG $1
    fi
}

darwincheck () {
    if [ $? -eq 0 ]; then
        echo $OK $1
    else
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo "Tryng dynamic lib copy"
            cp -R $ENDOBJFOLDER/$BUILDNAMENOEXT.dylib $FOLDER/include/
            check "crmd.dylib"
        else
            echo $FAILMSG $1
        fi
    fi
}

includefiles() {
    cp -R $ENDOBJ $FOLDER/include/
    darwincheck "crmd."$EXTNAME

    cp -R $FOLDER/crmd.h       $FOLDER/include/
    check "crmd.h"

    cp -R $FOLDER/globaldefs.h $FOLDER/include/
    check "globaldefs.h"

    cp -R $FOLDER/osSolver.h $FOLDER/include/
    check "osSolver.h"
}

if [[ "$TEST" != "test" ]]; then
    if [[ "$BUILDSUCCESS" == "true" ]]; then
        includefiles
    fi
fi