//
// Created by omara on 8/6/2026.
//
#include "FileReader.h"
#include <iostream>
#include <fstream>
#include <sstream>


string FileReader::readFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cout << "Failed to open: " << filepath << endl;
        return {};
    }
    stringstream buffer;
    buffer << file.rdbuf();
    return (buffer.str());
}