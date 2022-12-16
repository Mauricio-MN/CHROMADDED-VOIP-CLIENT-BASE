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

#BUILD GNU

TESTARG=""
TEST=$1
BUILDNAME=""
FPIC=""
SHARED=""
G=""
WINLIBS=""

if [[ "$EXTNAME" == "dll" ]]; then
    WINLIBS="-lwsock32
             -lWs2_32"
fi

if [[ "$TEST" == "test" ]]; then
    TESTARG=$FOLDER/../test.cpp
    if [[ "$EXTNAME" == "dll" ]]; then
        BUILDNAME="../test.exe"
    else
        BUILDNAME="../test"
    fi
    G="-g"
else
    BUILDNAME="crmd."$EXTNAME
    FPIC="-fPIC"
    SHARED="-shared"
    G=""
fi

CMDBUILD="$COMPILER -fdiagnostics-color=always -Wall -std=c++20
                $G
				$FOLDER/connection.cpp
				$FOLDER/cript.cpp
				$FOLDER/protocol.cpp
				$FOLDER/soundmanager.cpp
				$FOLDER/bufferParser.cpp
				$FOLDER/player.cpp
				$FOLDER/SoundCustomBufferRecorder.cpp
				$FOLDER/smbPitchShift.cpp
				$FOLDER/opusmanager.cpp
				$FOLDER/crmd.cpp
				$TESTARG
				-o
				$FOLDER/$BUILDNAME
                $FPIC
                $SHARED
				$WINLIBS
				-lssl
				-lcrypto
				-lsfml-system
				-lsfml-audio
				-llibopus"

echo $CMDBUILD
eval $CMDBUILD
if [ $? -eq 0 ]; then
    echo ""
else
    echo "BUILD FAILED"
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
            cp -R $FOLDER/crmd.dylib $FOLDER/include/
            check "crmd.dylib"
        else
            echo $FAILMSG $1
        fi
    fi
}

includefiles() {
    cp -R $FOLDER/crmd.$EXTNAME $FOLDER/include/
    darwincheck "crmd."$EXTNAME

    cp -R $FOLDER/crmd.h       $FOLDER/include/
    check "crmd.h"

    cp -R $FOLDER/globaldefs.h $FOLDER/include/
    check "globaldefs.h"

    cp -R $FOLDER/osSolver.h $FOLDER/include/
    check "osSolver.h"
}

if [[ "$TEST" != "test" ]]; then
    includefiles
fi