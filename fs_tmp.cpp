#include <iostream>
#include <sstream>
#include "fs.h"
#include "cstring"
#include <vector>
#include <iterator>
#include <string>

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";
}

FS::~FS()
{

}

void FS::find_free(int16_t first)
{
    for(; first < BLOCK_SIZE/2; first++)
    {
        if(fat[first] == FAT_FREE)
        {
            break;
        }
    }
}

// formats the disk, i.e., creates an empty file system
int
FS::format()
{
    std::cout << "FS::format()\n";
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;
    disk.write(0, nullptr);
    for(unsigned int i = 2; i < BLOCK_SIZE/2; i++)
    {
        fat[i] = FAT_FREE;
        disk.write(i, nullptr);
    }
    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";
    int16_t pos = 2;
    std::string data = "";
    std::string test = "";
    dir_entry file;
    uint8_t blk[BLOCK_SIZE] = "";

    // Need to fix data
    std::cin >> data;
    std::strcpy(file.file_name, filepath.c_str());
    file.type = TYPE_FILE;
    file.access_rights = READ;
    find_free(pos);
    file.first_blk = pos;
    file.size = sizeof(file);
    if(file.size > BLOCK_SIZE)
    {
        //find_free(++pos);
        //fat[file.first_blk] = pos; // KOLLA SÅ RÄTT POS 
        unsigned int res = file.size / BLOCK_SIZE;

        if(file.size % BLOCK_SIZE != 0)
        {
            res++;           
        }

        for(res; res > 0; res--)
        {
            find_free(++pos);
            fat[pos] = pos;
        }
        find_free(++pos);
        fat[pos] = FAT_EOF;
    }
    else
    {
        fat[file.first_blk] = FAT_EOF;
    }
    uint16_t i=file.first_blk;
    int k=0;
    while(fat[i] != FAT_EOF && i < BLOCK_SIZE/2){
        test = data.substr(k, BLOCK_SIZE);
        std::copy(test.begin(), test.end(), std::begin(blk));
        disk.write(i, blk);
        i = fat[i];
        k += BLOCK_SIZE;
    }
    test = data.substr(k, BLOCK_SIZE);
    std::copy(test.begin(), test.end(), std::begin(blk));
    disk.write(i, blk);
    //
    // Nedanstående är fel
    //
    disk.read(0, blk);
    //std::string kalle((char*)blk);
    disk.write(0, (uint8_t*)&file); //måste göra så att vi inte skriver över innehållet 
    if(blk[BLOCK_SIZE] < BLOCK_SIZE){
        //std::string tmp(blk);
        //strcpy_s(file.)
        
    }
    else{
        int check = sizeof(blk) + sizeof(file);
        printf("%d \n", check);
        std::cout << "too many files" << std::endl;
    }
    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    uint8_t blk[BLOCK_SIZE] = "";
    disk.read(2, blk);
    std::string kalle((char*)blk);
    std::cout << kalle << std::endl;
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::cout << "FS::ls()\n";
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";
    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";
    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    std::cout << "FS::pwd()\n";
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    return 0;
}