#include <iostream>
#include "root_block.h"
#include <string>

int main(int argc, char** argv){
    std::fstream fs("../disco2.IMA", std::ios::in | std::ios::out | std::ios::binary);

    Fat16 fat(fs);
    
    fat.listRootDirFiles();

    std::vector<Fat16::Directory> dirs = fat.getRootDirectory();

    for (int i = 0; i < dirs.size(); ++i) {
        if (dirs.at(i).isUsed()) {
            std::cout << "i'm " << dirs.at(i).getName() << std::endl;
            fat.extractContents(dirs.at(i));
        }
        
    }


    return 0;
}
