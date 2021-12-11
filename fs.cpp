#include <iostream>
#include "fs.h"
#include "cstring"

FS::FS()
: file_pos(0)
{
    std::cout << "FS::FS()... Creating file system\n";

    // Fill fat from disk
    disk.read(0, (uint8_t*)files);
    disk.read(1, (uint8_t*)fat);
    while(file_pos < MAX_NO_FILES && strcmp(files[file_pos].file_name, "") != 0)
    {
        file_pos++;
    }
}

FS::~FS()
{}

void FS::find_free(int16_t &first)
{
    while(first < BLOCK_SIZE/2 && fat[first] != FAT_FREE)
    {
        first++;
    }
}

// formats the disk, i.e., creates an empty file system
int
FS::format()
{
    std::cout << "FS::format()\n";
    char data[1] = "";
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;
    for(unsigned int i = 2; i < BLOCK_SIZE/2; i++)
    {
        disk.write(i, (uint8_t*)data);
        fat[i] = FAT_FREE;
    }

    for (unsigned int i = 0; i < MAX_NO_FILES; i++)
    {
        strcpy(files[i].file_name, data);
    }
    file_pos = 0;
    disk.write(1, (uint8_t*)fat);

    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    for (unsigned int i = 0; i < file_pos; i++)
    {
        // std::cout << "File_name: " << files[i].file_name << std::endl;
        // std::cout << "c_str: " << filepath.c_str() << std::endl;
        if (strcmp(files[i].file_name, filepath.c_str()) == 0)
        {
            std::cout << "File name already taken. Please pick another." << std::endl;

            return -1;
        }    
    }
    
    if(file_pos < MAX_NO_FILES)
    {
        std::cout << "FS::create(" << filepath << ")\n";
        int16_t pos = 2;
        std::string data;
        dir_entry file;

        // Need to fix data
        std::getline(std::cin, data);
        file.size = data.size();
        std::strcpy(file.file_name, filepath.c_str()); // TODO: Add condition for files with same name
        file.type = TYPE_FILE;
        file.access_rights = READ;
        find_free(pos);
        file.first_blk = pos;
        disk.write(file.first_blk, (uint8_t*)data.c_str());
        files[file_pos++] = file;
        if(file.size > BLOCK_SIZE && file.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
        {
            unsigned int res = file.size / BLOCK_SIZE - 1;

            if(file.size % BLOCK_SIZE != 0)
            {
                res++;            
            }

            for(res; res > 0; res--)
            {
                find_free(++pos);
                fat[pos] = pos;
                disk.write(pos, (uint8_t*)data.c_str());
            }

            find_free(++pos);
            fat[pos] = FAT_EOF;
            disk.write(pos, (uint8_t*)data.c_str());
        }
        else
        {
            fat[file.first_blk] = FAT_EOF;
        }

        disk.write(0, (uint8_t*)files);
        disk.write(1, (uint8_t*)fat);
    }
    else
    {
        std::cout << "Disk has no more room for additional data" << std::endl;

        return -1;
    }   

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    bool found = false;
    std::cout << "FS::cat(" << filepath << ")\n";
    char buffer[BLOCK_SIZE];
    unsigned int i = 0;

    while(i <= file_pos)
    {
        if(strcmp(files[i].file_name, filepath.c_str()) == 0)
        {
            found = true;
            break;
        }
        i++;
    }

    if(found)
    {
        uint16_t blk_no = files[i].first_blk;
        while(fat[blk_no] != FAT_EOF)
        {
            disk.read(blk_no, (uint8_t*)buffer);
            std::cout << buffer;
            blk_no = fat[blk_no];
        }
        disk.read(blk_no, (uint8_t*)buffer);
        std::cout << buffer << std::endl;
    }
    else
    {
        std::cout << "cat: " << filepath << ": No such file or directory" << std::endl;
    }

    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::cout << "FS::ls()\n";
    std::cout << "name\tsize" << std::endl;
    
    for (unsigned int i = 0; i < file_pos; i++)
    {
        std::cout << files[i].file_name << "\t" << files[i].size << std::endl;
    }

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
