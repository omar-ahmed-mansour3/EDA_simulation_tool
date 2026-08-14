//
// Created by omara on 8/6/2026.
//

#ifndef EDA_PROJECT_FILEREADER_H
#define EDA_PROJECT_FILEREADER_H
#include <string>
using namespace std;

class FileReader {
    public:
        static string readFile(const string& file_name);
};
#endif //EDA_PROJECT_FILEREADER_H