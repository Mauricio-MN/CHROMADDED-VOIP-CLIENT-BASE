#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctime>
#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <queue>
#include <SFML/System/FileInputStream.hpp>

class FileReader {
public:
    FileReader(const std::string& filename) : filename(filename){
        file.open(filename, std::ios::in | std::ios::binary);
        if (!file) {
            std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        }
    }

    ~FileReader(){
        file.close();
    }

    char get(size_t position) {
        char content;
        file.seekg(position);
        file.get(content);
        return content;
    }

    size_t getSize_t(size_t position){
        size_t content;
        file.seekg(position);
        file.get((char*)&content, sizeof(size_t));

        return content;
    }

    size_t getFileSize() {
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0);
        return fileSize;
    }

    void close(){
        file.close();
    }

private:
    std::string filename;
    std::ifstream file;
};

class FileWriter {
public:
    FileWriter(const std::string& filename) : filename(filename) {
        file.open(filename, std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        }
        pos = 0;
    }

    ~FileWriter(){
        file.close();
    }

    void writeByte(char byte){
        write(pos, &byte, 1);
        pos++;
    }

    void writeSize_t(size_t size){
        write(pos, &size, sizeof(size_t));
        pos += sizeof(size_t); // Incrementar pos pelo tamanho de size_t
    }

    bool write(size_t position, const void* content, size_t size) {
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekp(0, std::ios::end);

        if (position > fileSize) {
            // Se a posição de escrita estiver além do tamanho atual, expandir o arquivo
            file.write("", position - fileSize);
        }
        
        file.seekp(position);
        file.write((const char*)content, size);
        return true;
    }

    void close(){
        file.close();
    }

private:
    std::string filename;
    std::fstream file;
    size_t pos;
};

// Função para obter o valor de um bit em uma posição específica
int getBit(std::vector<char>& buffer, int bitPos) {
    int byteIndex = bitPos / 8;   // Encontra o índice do byte que contém o bit
    int bitOffset = bitPos % 8;   // Calcula o deslocamento do bit dentro do byte

    char byte = buffer[byteIndex]; // Obtém o byte relevante do vetor

    // Use máscaras para isolar o bit desejado e retorne 0 ou 1
    return (byte >> (7 - bitOffset)) & 1;
}

// Função para definir o valor de um bit em uma posição específica
void setBit(std::vector<char>& header, int bitPos, int bitValue) {
    int byteIndex = (bitPos) / 8;   // Encontra o índice do byte que contém o bit
    int bitOffset = (bitPos) % 8;   // Calcula o deslocamento do bit dentro do byte

    char& byte = header[byteIndex]; // Obtém a referência ao byte relevante do vetor

    if (bitValue == 0) {
        // Limpa o bit (definindo-o como 0)
        byte &= ~(1 << (7 - bitOffset));
    } else if (bitValue == 1) {
        // Define o bit (definindo-o como 1)
        byte |= (1 << (7 - bitOffset));
    } else {
        // Lança uma exceção se o valor do bit não for 0 ou 1
        throw std::invalid_argument("O valor do bit deve ser 0 ou 1.");
    }
}

void compact(FileReader& data, FileWriter& bufferResult, int offset){
    size_t filesize = data.getFileSize();
    size_t headerSize = (filesize / 8); //1 bit por byte

    std::vector<char> header;
    header.resize(headerSize);
    std::vector<char> result;

    std::vector<size_t> regonizedOffsets;

    size_t bitPos = 0;

    for(size_t i = 0; i < filesize; i++){

        int itPos = 0;
        int toErease = -1;
        for(size_t& ofst : regonizedOffsets){
            if(ofst == i){
                toErease = itPos;
                setBit(header, i, 1);
                break;
            }
            itPos++;
        }
        if(toErease >= 0){
            regonizedOffsets.erase(regonizedOffsets.begin() + itPos);
            continue;
        }

        bool haveNext = false;
        size_t next = i+offset;
        if(i+offset < filesize){
            haveNext = true;
        }

        if(haveNext){
            if(data.get(i) == data.get(next)){
                regonizedOffsets.push_back(next);

                bufferResult.writeByte(data.get(i));
                setBit(header, i, 1);
                continue;
            }
        }
        bufferResult.writeByte(data.get(i));
        setBit(header, i, 0);
    }

    for(size_t i = 0; i < headerSize; i++){
        bufferResult.writeByte(header[i]);
    }

    bufferResult.writeSize_t(headerSize);
}

void descompact(FileReader& CompressedData, FileWriter& bufferResult, int offset) {
    size_t CDataFileSize = CompressedData.getFileSize();
    size_t headerSize = CompressedData.getSize_t(CDataFileSize - 8); // Tamanho do cabeçalho em bytes

    std::vector<char> header;
    header.resize(headerSize);

    size_t bitPos = 0;

    // Lê o cabeçalho do arquivo compactado
    for (size_t i = 0; i < headerSize; i++) {
        header[i] = CompressedData.get(CDataFileSize - headerSize + i - 8); // Lê o cabeçalho no final do arquivo
    }

    size_t dataIndex = 0; // Índice para ler os dados compactados

    // Lê e descompacta os dados com base no cabeçalho
    std::queue<std::pair<size_t, char>> regonized;
    size_t actualPos = 0;

    for (size_t i = 0; i < CDataFileSize; i++) {
        bool isRecognized = getBit(header, actualPos);

        if(isRecognized){
            if(!regonized.empty()){
                if(regonized.front().first != actualPos){
                    regonized.push(std::pair<size_t, char>(actualPos + offset, CompressedData.get(i)));
                }
            } else {
                bufferResult.writeByte(regonized.front().second);
                regonized.pop();
                actualPos++;
            }
        }

        bufferResult.writeByte(CompressedData.get(i));
        actualPos++;

    }
}

int main(){
    //5
    //7
    //8
    //1
    for(int i = 1; i < 11; i ++){
        std::string cmptnm = std::string("teste5-7-8-1-") + std::to_string(i) + std::string(".mmn");
        FileReader file("teste5-7-8-1.mmn");
        FileWriter cmpt(cmptnm);
        compact(file, cmpt, i);
    }


}