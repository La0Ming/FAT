#include <iostream>
#include "fs.h"
#include "cstring"

FS::FS()
: file_pos(0)
{
    std::cout << "FS::FS()... Creating file system\n";

    // Fill fat from disk
    current_dir.blk_nmr =0;
    disk.read(0, (uint8_t*)current_dir.files); //måste hämta alla directories (hämtar bara root just nu)
    disk.read(1, (uint8_t*)fat);
    while(file_pos < MAX_NO_FILES && strcmp(current_dir.files[file_pos].file_name, "") != 0)
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

int FS::find_file(std::string path)
{
    for (unsigned int i = 0; i < MAX_NO_FILES; i++)
    {
        if (strcmp(current_dir.files[i].file_name, path.c_str()) == 0)
        {
            return i;
        }    
    }

    return -1;
}

int FS::find_free_current_dir()
{
    std::string empty_str = "";
    for (int i = 0; i < MAX_NO_FILES; i++)
    {
        if (strcmp(current_dir.files[i].file_name, empty_str.c_str()) == 0)
        {
            std::cout << i << std::endl;
            return i;
        }    
    }
    std::cout << "no free position" << std::endl;
    return -1;
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
        strcpy(current_dir.files[i].file_name, data);
    }

    file_pos = 0;
    disk.write(0, (uint8_t*)current_dir.files);
    disk.write(1, (uint8_t*)fat);

    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";
    if(find_file(filepath) + 1)
    {
        std::cout << "create: cannot create file '" << filepath << "': File exists" << std::endl;
    }
    else
    {
        if(file_pos < MAX_NO_FILES)
        {
            int16_t pos = 2;
            std::string data = "";
            dir_entry file;
            std::getline(std::cin, data);
            file.size = data.size();
            std::strcpy(file.file_name, filepath.c_str());
            file.type = TYPE_FILE;
            file.access_rights = READ;
            find_free(pos);
            file.first_blk = pos;
            int pos_file = find_free_current_dir();
            if(pos_file != -1){
                current_dir.files[pos_file] = file;
            }
            else{
                std::cout << "The directory is full" << std::endl;
                return -1;
            }
            disk.write(file.first_blk, (uint8_t*)data.substr(0, BLOCK_SIZE).c_str());

            if(file.size > BLOCK_SIZE && file.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
            {
                unsigned int res = file.size / BLOCK_SIZE - 1;

                if(file.size % BLOCK_SIZE != 0)
                {
                    res++;            
                }

                unsigned int next = 2;

                for(res; res > 0; res--)
                {
                    int16_t prev_pos  = pos;
                    find_free(++pos);
                    fat[prev_pos] = pos;
                    disk.write(pos, (uint8_t*)data.substr(0, BLOCK_SIZE * next++).c_str());
                }
            }

            fat[pos] = FAT_EOF;
            //std::cout << current_dir.files[0].file_name << std::endl;
            //std::cout << current_dir.files[1].file_name << std::endl;
            disk.write(current_dir.blk_nmr, (uint8_t*)current_dir.files);
            disk.write(1, (uint8_t*)fat);
        }
        else
        {
            std::cout << "Disk has no more room for additional data" << std::endl;
        }
    } 

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    char buffer[BLOCK_SIZE] = {""};
    unsigned int i = find_file(filepath);

    if(i + 1)
    {
        int16_t blk_no = current_dir.files[i].first_blk;
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
    std::cout << "name\ttype\tsize" << std::endl;
    std::string fnut_fnut = "";
    for (unsigned int i = 0; i < MAX_NO_FILES; i++)
    {
        if(strcmp(current_dir.files[i].file_name, fnut_fnut.c_str())){
            std::string type = "file";
            std::string size = "-";

            if(current_dir.files[i].type)
            {
                type = "dir";
            }

            if(current_dir.files[i].size)
            {
                size = std::to_string(current_dir.files[i].size);
            }

            std::cout << current_dir.files[i].file_name << "\t" << type << "\t" << size << std::endl;
        }
    }
    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";

    int src_pos = find_file(sourcepath);
    int dest_pos = find_file(destpath);
    
    if(dest_pos + 1)
    {
        if(file_pos < MAX_NO_FILES && current_dir.files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
        {
            current_dir.files[dest_pos].size = current_dir.files[src_pos].size;
            char buffer[BLOCK_SIZE] = {""};
            int16_t src_blk_no = current_dir.files[src_pos].first_blk;
            int16_t dest_blk_no = current_dir.files[dest_pos].first_blk;

            while(fat[src_blk_no] != FAT_EOF)
            {
                disk.read(src_blk_no, (uint8_t*)buffer);
                disk.write(dest_blk_no, (uint8_t*)buffer);
                src_blk_no = fat[src_blk_no];

                if(fat[dest_blk_no] == FAT_EOF)
                {
                    int16_t prev_pos = dest_blk_no;
                    find_free(++dest_blk_no);
                    fat[prev_pos] = dest_blk_no;
                    fat[dest_blk_no] = FAT_EOF;
                }
                else
                {
                    dest_blk_no = fat[dest_blk_no];
                }
            }
            disk.read(src_blk_no, (uint8_t*)buffer);
            disk.write(dest_blk_no, (uint8_t*)buffer);

            disk.write(0, (uint8_t*)current_dir.files);
            disk.write(1, (uint8_t*)fat);
        }
        else
        {
            std::cout << "Disk has no more room for additional data" << std::endl;
        }
    }
    else if(find_file(sourcepath) == -1)
    {
        std::cout << "cp: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    }
    else
    {
        if(file_pos < MAX_NO_FILES && current_dir.files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
        {
            dir_entry file;
            std::strcpy(file.file_name, destpath.c_str());
            file.size = current_dir.files[src_pos].size;
            file.type = TYPE_FILE;
            file.access_rights = READ;
            int16_t pos = 2;
            find_free(pos);
            file.first_blk = pos;
            current_dir.files[file_pos++] = file;
            fat[file.first_blk] = FAT_EOF;

            char buffer[BLOCK_SIZE] = {""};
            int16_t src_blk_no = current_dir.files[src_pos].first_blk;
            int16_t dest_blk_no = file.first_blk;

            while(fat[src_blk_no] != FAT_EOF)
            {
                disk.read(src_blk_no, (uint8_t*)buffer);
                disk.write(dest_blk_no, (uint8_t*)buffer);
                src_blk_no = fat[src_blk_no];

                if(fat[dest_blk_no] == FAT_EOF)
                {
                    std::cout << "EOF" << std::endl;
                    int16_t prev_pos = pos;
                    find_free(++dest_blk_no);
                    fat[prev_pos] = dest_blk_no;
                    fat[dest_blk_no] = FAT_EOF;
                }
                else
                {
                    dest_blk_no = fat[dest_blk_no];
                }
            }
            disk.read(src_blk_no, (uint8_t*)buffer);
            disk.write(dest_blk_no, (uint8_t*)buffer);

            disk.write(0, (uint8_t*)current_dir.files);
            disk.write(1, (uint8_t*)fat);
        }
        else
        {
            std::cout << "Disk has no more room for additional data" << std::endl;
        }
    }

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";

    int pos = find_file(sourcepath);

    if(pos + 1)
    {
        strcpy(current_dir.files[pos].file_name, destpath.c_str());
        disk.write(0, (uint8_t*)current_dir.files);
    }
    else
    {
        std::cout << "mv: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    }

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    std::cout << "FS::rm(" << filepath << ")\n";

    int pos = find_file(filepath);
    char buffer[1] = "";

    if(pos + 1)
    {
        int16_t blk_no = current_dir.files[pos].first_blk;

        
        for (; pos < file_pos; pos++)
        {
            current_dir.files[pos] = current_dir.files[pos + 1];
        }
        
        file_pos--;

        while(fat[blk_no] != FAT_EOF)
        {
            disk.write(blk_no, (uint8_t*)buffer);
            blk_no = fat[blk_no];
            fat[blk_no] = FAT_FREE;
        }
        disk.write(blk_no, (uint8_t*)buffer);
        fat[blk_no] = FAT_FREE;

        disk.write(0, (uint8_t*)current_dir.files);
        disk.write(1, (uint8_t*)fat);
    }
    else
    {
        std::cout << "rm: cannot remove '" << filepath << "': No such file or directory" << std::endl;
    }

    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";

    int src_pos = find_file(filepath1);
    int dest_pos = find_file(filepath2);

    if(src_pos == -1)
    {
        std::cout << "append: cannot find '" << filepath1 << "': No such file or directory" << std::endl;
    }
    else if(dest_pos == -1)
    {
        std::cout << "append: cannot find '" << filepath2 << "': No such file or directory" << std::endl;
    }
    else if(filepath1 == filepath2)
    {
        std::cout << "\"You do not need to cover the special case when appending a file to itself, i.e., ’append <filename1> <filename1>’.\"" << std::endl << std::endl << "- Håkan Grahn" << std::endl;
    }
    else
    {
        if(file_pos < MAX_NO_FILES && current_dir.files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
        {
            char buffer[BLOCK_SIZE] = {""};
            int16_t dest_blk_no = current_dir.files[dest_pos].first_blk;

            while(fat[dest_blk_no] != FAT_EOF)
            {
                dest_blk_no = fat[dest_blk_no];
            }

            int16_t src_blk_no = current_dir.files[find_file(filepath1)].first_blk;
            int16_t prev_pos = dest_blk_no;
            find_free(++dest_blk_no);
            fat[prev_pos] = dest_blk_no;

            while(fat[src_blk_no] != FAT_EOF)
            {
                disk.read(src_blk_no, (uint8_t*)buffer);
                disk.write(dest_blk_no, (uint8_t*)buffer);
                current_dir.files[dest_pos].size += strlen(buffer);
                src_blk_no = fat[src_blk_no];
                int16_t prev_pos = dest_blk_no;
                find_free(++dest_blk_no);
                fat[prev_pos] = dest_blk_no;
            }

            disk.read(src_blk_no, (uint8_t*)buffer);
            disk.write(dest_blk_no, (uint8_t*)buffer);
            current_dir.files[dest_pos].size += strlen(buffer);
            fat[dest_blk_no] = FAT_EOF;
            disk.write(0, (uint8_t*)current_dir.files);
            disk.write(1, (uint8_t*)fat);
        }
        else
        {
            std::cout << "Disk has no more room for additional data" << std::endl;
        }
    }
    

    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";
    //måste checka så att fat och nuvarande files inte är fullt. Om det är fullt i någon av dem, printa det och avsluta.
    int free_dir_pos = find_free_current_dir();
    if(free_dir_pos == -1){
        return -1;
    }
    dir_entry subdir;
    int16_t pos = 2;
    std::string back_dir_name = "..";
    std::string empty_dir_name = "";
    
    std::strcpy(subdir.file_name, dirpath.c_str());
    subdir.size = sizeof(dir_entry);
    subdir.type = 1;
    subdir.access_rights = 0x04;
    find_free(++pos);
    subdir.first_blk = pos;
    fat[pos] = FAT_EOF;
    current_dir.files[free_dir_pos] = subdir;
    sub_dir new_dir;
    dir_entry parent_dir;
    std::strcpy(parent_dir.file_name, back_dir_name.c_str());
    parent_dir.access_rights = 0x04;
    parent_dir.size = sizeof(current_dir);
    new_dir.files[0] = parent_dir;
    for(int i=1; i < MAX_NO_FILES; i++){
        std::strcpy(new_dir.files[i].file_name, empty_dir_name.c_str());
    }
    disk.write(current_dir.blk_nmr, (uint8_t*)current_dir.files);
    disk.write(subdir.first_blk, (uint8_t*)new_dir.files);
    disk.write(1, (uint8_t*)fat);
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";
    std::string tmp = "..";
    if(strcmp(tmp.c_str(), dirpath.c_str()) == 0){
        //std::cout << ".. path" << std::endl;
        std::cout << current_dir.files[0].first_blk << std::endl;
        //disk.write(current_dir.files[0].first_blk, (uint8_t*)current_dir.files);
        //std::cout << current_dir.files[0].file_name << std::endl;
        disk.read(current_dir.files[0].first_blk, (uint8_t*)current_dir.files);
        //std::cout << current_dir.files[0].file_name << std::endl;
        //std::cout << current_dir.files[1].file_name << std::endl;
        current_dir.blk_nmr = current_dir.files[0].first_blk;
    }
    else{
        std::cout << dirpath << std::endl;
        int dir_nmr = find_file(dirpath);
        if(dir_nmr != -1){
            disk.read(current_dir.files[dir_nmr].first_blk, (uint8_t*)current_dir.files);
        }
        current_dir.blk_nmr = current_dir.files[dir_nmr].first_blk;
        /*
        for(int k=0; k <MAX_NO_FILES; k++){
            if(current_dir.files[k].type == 1 && strcmp(current_dir.files[k].file_name, dirpath.c_str()) == 0){

            }
            /*
            if(current_dir.children[i].files[k].type == 1 && strcmp(current_dir.children[i].files[k].file_name, dirpath.c_str()) == 0){
                current_dir = current_dir.children[i];
                std::cout << "found it" << std::endl;
                return 0;
            }
        }*/
    }
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
