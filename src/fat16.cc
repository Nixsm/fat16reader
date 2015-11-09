#include "root_block.h"
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <sys/stat.h>
#include <memory>

Fat16::Fat16(std::fstream& fs)
    :_fs(fs)
{
    fs.read((char*)&_rb, sizeof(RootBlock));
    
    uchar* nBytesBlock = _rb.nBytesBlock;
    unsigned short nbo = nBytesBlock[0] | (nBytesBlock[1] << 8);
    
    uchar* fatSize = _rb.nBlkOccpByFAT;
    unsigned short fas = fatSize[0] | (fatSize[1] << 8);
    
    _fats.resize((int) _rb.nFAT);
    for (int i = 0; i < (int)_rb.nFAT; ++i) {
        _fats.at(i).resize(fas * nbo);
        fs.read((char*) const_cast<uchar*>(_fats.at(i).data()), _fats.at(i).size());
    }

    uchar* rootEntries = _rb.nRtEntries;
    unsigned short rt = rootEntries[0] | (rootEntries[1] << 8);

    int nRtEntries = std::ceil((rt * 32) / (float)nbo);

    _rootEntries.resize(nRtEntries);
    
    fs.read((char*) const_cast<Directory*>(_rootEntries.data()), sizeof(Directory) * nRtEntries);

}

void Fat16::listRootDirFiles() {
    for (int i = 0; i < _rootEntries.size(); ++i) {
        Directory rtDir = _rootEntries.at(i);
        if (rtDir.isUsed()) {
            std::cout << rtDir.getName() << " ";
            std::cout << rtDir.getExtension() << std::endl;
        }
    }

    //for (int i = 0; i < _fats.at(0).size(); ++i){
    //    std::cout << (int)_fats.at(0).at(i) << " ";
    //}
}

int Fat16::_getFreeClusterNumber() {
    int startingCluster = 2;

    for (int i = 0; i < _rootEntries.size(); ++i) {
        if (_rootEntries.at(i).isUsed()) {
            if (_rootEntries.at(i).startingClusterNumber() > startingCluster) {
                startingCluster = _rootEntries.at(i).startingClusterNumber();
            }
        }
    }

    int idx;
    for (int i = 4; i < _fats.at(0).size(); i+=2) {
        if ((idx = util::toShort(_fats.at(0).at(i), _fats.at(0).at(i + 1))) > startingCluster && idx != 0xFFFF) {
            startingCluster = idx;
        }
    }
    return startingCluster + 1; // + 1 because the last is mapped
}
Fat16::RootBlock Fat16::getRootBlock() {
    return _rb;
}

std::vector<std::vector<uchar> > Fat16::getFATs() {
    return _fats;
}

std::vector<Fat16::Directory> Fat16::getRootDirectory() {
    return _rootEntries;
}

int Fat16::_dataRegionStart() {
    int rootStart = _rootStart();

    return rootStart + ((util::toShort(_rb.nRtEntries) * 32) / util::toShort(_rb.nBytesBlock));
}

int Fat16::_rootStart() {
    int reservedBlocks = util::toShort(_rb.nRsrvdBlks);

    int reservedRegion = 0x00;
    
    int FATStart = reservedBlocks + reservedRegion;

    int rootStart = FATStart + ((int) _rb.nFAT * util::toShort(_rb.nBlkOccpByFAT));

    return rootStart;
}

int Fat16::_firstSectorOfCluster(int n) {
    return _dataRegionStart() + ((n - 2) * _rb.nPerAU);
}

int Fat16::_fatSector(int n) {
    return _nFatCopyLocation(0) + ((n * 2) / util::toShort(_rb.nBytesBlock));
}

int Fat16::_nFatCopyLocation(int n) {
    int reservedRegion = 0;
    return reservedRegion + (n * util::toShort(_rb.nBlkOccpByFAT));
}


int Fat16::_getClusterNumber(int clusterNumber) {
    int fatIdx = _fatSector(clusterNumber) * _rb.blocksOccpByFat() + clusterNumber;
    fatIdx *= 2;
    return util::toShort(_fats.at(0).at(fatIdx), _fats.at(0).at(fatIdx + 1));
}

std::string Fat16::extractContents(Directory dir, std::string path) {
    path += dir.getName();
    path += ".";
    path += dir.getExtension();

    int fileSize = dir.getFileSize();
    std::vector<uchar> file;

    int readed = 0;
    
    int dataStart = _rootStart() + _rootEntries.size() - 2;
    
    int bytesPerBlock = _rb.bytesPerBlock();

    int clusterNumber = dir.startingClusterNumber();
    
    int blockNumber = dataStart + clusterNumber;

    while (clusterNumber != 0xFFFF) {
        _fs.seekg(blockNumber * bytesPerBlock, std::ios::beg);

        if (fileSize < bytesPerBlock) {
            uchar* temp = new uchar[fileSize];
            _fs.read((char*) temp, fileSize);

            for (int i = 0; i < fileSize; ++i) {
                file.push_back(temp[i]);
            }
            delete [] temp;
        } else {
            uchar* temp = new uchar[bytesPerBlock];
            _fs.read((char*) temp, bytesPerBlock);

            for (int i = 0; i < bytesPerBlock; ++i) {
                file.push_back(temp[i]);
            }

            fileSize -= bytesPerBlock;
            delete [] temp;
        }
        
        int fatIdx = clusterNumber;
        fatIdx *= 2;
        clusterNumber = util::toShort(_fats.at(0).at(fatIdx), _fats.at(0).at(fatIdx + 1));
        blockNumber = dataStart + clusterNumber;
    }

    std::ofstream os(path.c_str(), std::ios::out | std::ios::binary);
    os.write((char*) file.data(), file.size());
    os.close();
    return path;
}

void Fat16::writeFile(std::string path) {
    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary);

    std::streampos fsize = 0;

    struct stat stats;

    if (stat(path.c_str(), &stats) == 0) {
        fsize = stats.st_size;
    }
    
    int size = fsize;

    int dataStart = _rootStart() + _rootEntries.size() - 2;

    int freeCluster = _getFreeClusterNumber();
    
    unsigned short bpb = _rb.bytesPerBlock();

    int blockNumber = dataStart + freeCluster;

    Directory dir;
    util::toByte((unsigned int) size, dir.fileSize);
    util::toByte((unsigned short) freeCluster, dir.startingCluster);

    std::string::size_type t = path.find(".");
    std::string ext;
    ext.resize(8);

    if (t != std::string::npos) {
        path.copy(const_cast<char*>(ext.c_str()), 3, t + 1);
        path.erase(t, 4);
    }
    
    strcpy((char*) dir.filename, path.c_str());
    strcpy((char*) dir.fileExt, ext.c_str());

    // TODO datas and other stuff


    for (int i = 0; i < _rootEntries.size(); ++i) {
        if (!_rootEntries.at(i).isUsed()) {
            _rootEntries.at(i) = dir;
            break;
        }
    }

    
    while (size != 0) {
        _fs.seekg(blockNumber * bpb);
        if (size < bpb) {
            uchar* bytes = new uchar[size];
            ifs.read((char*) bytes, size);

          
            _fs.write((char*) bytes, size);

            size -= size;
            int freeFatIdx = _getFreeFatIdx();
            for (int i = 0; i < _fats.size(); i++) {
                _fats.at(i).at(freeFatIdx) = 0xFF;
                _fats.at(i).at(freeFatIdx + 1) = 0xFF;
            }
            delete [] bytes;
        } else {
            uchar* bytes = new uchar[bpb];

            ifs.read((char*) bytes, bpb);

            _fs.write((char*) bytes, bpb);

            size -= bpb;
            
            freeCluster = _getFreeClusterNumber();
            int freeFatIdx = _getFreeFatIdx();

            uchar b[2];
            
            util::toByte((unsigned short) freeCluster, b);
            
            for (int i = 0; i < _fats.size(); i++) {
                _fats.at(i).at(freeFatIdx) = b[0];
                _fats.at(i).at(freeFatIdx + 1) = b[1];
            }
            blockNumber = dataStart + freeCluster;
            delete [] bytes;
        }
        
    }
    _saveDirs();
    _saveFats();

    
}

void Fat16::_saveDirs() {
    int rtBlkSize = sizeof(RootBlock);
    int fatSize = _fats.size() * _fats.at(0).size();

    _fs.seekg(fatSize + rtBlkSize);
    _fs.write((char*) const_cast<Directory*>(_rootEntries.data()), sizeof(Directory) * _rootEntries.size());
    
}

void Fat16::_saveFats() {
    int rtBlkSize = sizeof(RootBlock);

    
    _fs.seekg(rtBlkSize, std::ios::beg);

    for (int i = 0; i < _fats.size(); ++i) {
        _fs.write((char*) const_cast<uchar*>(_fats.at(i).data()), _fats.at(i).size());
    }    
}

int Fat16::_getFreeFatIdx() {
    int idx;
    for (int i = 4; i < _fats.at(0).size(); i+=2) {
        if ((idx = util::toShort(_fats.at(0).at(i), _fats.at(0).at(i + 1))) == 0x0000) {
            return i;
        }
    }
}
