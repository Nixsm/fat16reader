#ifndef ROOT_BLOCK_H
#define ROOT_BLOCK_H

#include "util.h"
#include <fstream>
#include <iostream>
#include <vector>

class Fat16 {
public:
    struct RootBlock {
        uchar bootstrapProgram[3];
        uchar manuDesc[8];
        uchar nBytesBlock[2];
        uchar nPerAU;
        uchar nRsrvdBlks[2];
        uchar nFAT;
        uchar nRtEntries[2];
        uchar nTtlBlks[2];
        uchar mdDesc;
        uchar nBlkOccpByFAT[2];
        uchar nBlkPerTrack[2];
        uchar nHeads[2];
        uchar nHiddenBlk[4];
        uchar nBlkEntireDsk[4];
        uchar physDrvNum[2];
        uchar extBootRec;
        uchar volSerNum[4];
        uchar volLabel[11];
        uchar fileSysId[8];
        uchar bootstrapProgramRem[0x1C0];
        uchar bootBlkSig[2];

        unsigned short bytesPerBlock() {
            return util::toShort(nBytesBlock);
        }

        unsigned short blocksOccpByFat() {
            return util::toShort(nBlkOccpByFAT);
        }
    };

    struct Directory {
        uchar filename[8];
        uchar fileExt[3];
        uchar fileAttr;
        uchar reserved[10];
        uchar timeCreUpt[2];
        uchar dtCreUpt[2];
        uchar startingCluster[2];
        uchar fileSize[4];

        bool isUsed() {
            return filename[0] != 0x00 && filename[0] != 0xE5;
        }

        bool isDirectory() {
            return filename[0] == 0x2E;
        }

        std::string getName() {
            std::string str((char*) filename);
            std::string sub = getExtension();

            std::string::size_type i = str.find(sub);

            if (i != std::string::npos) {
                str.erase(i, sub.length());
            }

            for (unsigned int i = 0; i < str.size(); i++) {
                if (std::isspace(str.at(i))) {
                    str.erase(i);
                    break;
                }
            }

            return str;
        }
        std::string getExtension() {
            return std::string((char*) fileExt);
        }

        unsigned short startingClusterNumber() {
            return startingCluster[0] | (startingCluster[1] << 8);
        }

        unsigned int getFileSize() {
            return util::toInt(fileSize);
        }
        
    };

public:
    Fat16(std::fstream& fs);

    RootBlock getRootBlock();
    std::vector<std::vector<uchar> > getFATs();
    std::vector<Fat16::Directory> getRootDirectory();

    void listRootDirFiles();

    std::string extractContents(Directory dir, std::string path = "");
    void writeFile(std::string path);
    
    

private:
    int _dataRegionStart();
    int _nFatCopyLocation(int n);
    int _fatSector(int n);
    int _firstSectorOfCluster(int n);
    int _rootStart();
    int _getClusterNumber(int n);

    int _getFreeFatIdx();
    int _getFreeClusterNumber();

    void _saveFats();
    void _saveDirs();
private:
    RootBlock _rb;
    std::vector<Directory> _rootEntries;
    std::vector<std::vector<uchar> > _fats;

    std::fstream& _fs;
    
};

#endif//ROOT_BLOCK
