#include <iostream>
#include "fs.h"
#include "cstring"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";

    // Root block
    cwd = "";
    current_blk = ROOT_BLOCK;
    disk.read(current_blk, (uint8_t*)files);
    file_pos = 0;
    while(file_pos < MAX_NO_FILES && strcmp(files[file_pos].file_name, "") != 0)
    {
        file_pos++;
    }

    // Fat Block
    disk.read(FAT_BLOCK, (uint8_t*)fat);
}

FS::~FS()
{

}

void FS::find_free(int16_t &first)
{
    while(first < BLOCK_SIZE/2 && fat[first] != FAT_FREE)
    {
        first++;
    }

    if(fat[first] != FAT_FREE)
    {
        first = -1;
    }
}

int FS::find_entry(const std::string path)
{
    for(int i = 0; i < file_pos; i++)
    {
        if(strcmp(files[i].file_name, path.c_str()) == 0)
        {
            return i;
        }
    }
    
    return -1;
}

// formats the disk, i.e., creates an empty file system
int
FS::format() // TODO: FIX
{
    std::cout << "FS::format()\n";

    // FAT block
    char empty[1] = "";
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;
    disk.write(FAT_BLOCK, (uint8_t*)fat);

    // Root block
    file_pos = 0;
    cwd = "";
    disk.write(ROOT_BLOCK, (uint8_t*)empty);
    disk.read(ROOT_BLOCK, (uint8_t*)files);

    // Rest of disk
    for(unsigned int i = 2; i < BLOCK_SIZE/2; i++)
    {
        disk.write(i, (uint8_t*)empty);
        fat[i] = FAT_FREE;
    }

    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int
FS::create(std::string filepath)
{
    std::cout << "FS::create(" << filepath << ")\n";

    if(find_entry(filepath) + 1)
    {
        std::cout << "create: cannot create file '" << filepath << "': File exists" << std::endl;
    }
    else
    {
        if(file_pos < MAX_NO_FILES)
        {
            int16_t pos = 2;
            std::string data = "";

            // Create entry
            dir_entry file;
            std::getline(std::cin, data);
            file.size = data.size();
            std::strcpy(file.file_name, filepath.c_str());
            file.type = TYPE_FILE;
            file.access_rights = READ;
            find_free(pos);
            file.first_blk = pos;
            disk.write(file.first_blk, (uint8_t*)data.substr(0, BLOCK_SIZE - 1).c_str());
            files[file_pos++] = file;
            disk.write(current_blk, (uint8_t*)files);

            if(file.size > BLOCK_SIZE && file.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
            {
                unsigned int res = file.size / BLOCK_SIZE - 1;

                if(file.size % BLOCK_SIZE != 0)
                {
                    res++;            
                }

                unsigned int next = 1;

                for(res; res > 0; res--)
                {
                    int16_t prev_pos  = pos;
                    find_free(++pos);
                    fat[prev_pos] = pos;
                    disk.write(pos, (uint8_t*)data.substr(BLOCK_SIZE * next, BLOCK_SIZE * (next + 1) - 1).c_str());
                    next++;
                }
            }

            fat[pos] = FAT_EOF;
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
    unsigned int i = find_entry(filepath);

    if(i + 1)
    {
        if(files[i].type)
        {
            std::cout << "cat: " << files[i].file_name << ": Is a directory" << std::endl;
        }
        else
        {
            int16_t blk_no = files[i].first_blk;
            while(fat[blk_no] != FAT_EOF)
            {
                disk.read(blk_no, (uint8_t*)buffer);
                std::cout << buffer;
                blk_no = fat[blk_no];
            }
            disk.read(blk_no, (uint8_t*)buffer);
            std::cout << buffer << std::endl;
        }
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

    unsigned int i = 1;

    if(current_blk == ROOT_BLOCK)
    {
        i = 0;
    }

    for(; i < file_pos; i++)
    {
        std::string type = "dir";
        std::string size = "-";
        
        if(files[i].type == TYPE_FILE)
        {
            type = "file";
            size = std::to_string(files[i].size);
        }

        std::cout << files[i].file_name << "\t" << type << "\t" << size << std::endl;
    }

    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";

    int src_pos = find_entry(sourcepath);
    int dest_pos = find_entry(destpath);
    

    if(src_pos == -1)
    {
        std::cout << "cp: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    }
    else
    {
        if(dest_pos + 1)
        {
            if(files[src_pos].type)
            {
                std::cout << "cp: " << files[src_pos].file_name << ": Is a directory" << std::endl;
            }
            else if(files[dest_pos].type)
            {
                std::cout << "cp: " << files[dest_pos].file_name << ": Is a directory" << std::endl;
            }
            else
            {
                if(file_pos < MAX_NO_FILES && files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
                {
                    files[dest_pos].size = files[src_pos].size;
                    char buffer[BLOCK_SIZE] = {""};
                    int16_t src_blk_no = files[src_pos].first_blk;
                    int16_t dest_blk_no = files[dest_pos].first_blk;

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

                    disk.write(current_blk, (uint8_t*)files);
                    disk.write(FAT_BLOCK, (uint8_t*)fat);
                }
                else
                {
                    std::cout << "Disk has no more room for additional data" << std::endl;
                }
            }
        }
        else
        {
            if(file_pos < MAX_NO_FILES && files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
            {
                dir_entry file;
                std::strcpy(file.file_name, destpath.c_str());
                file.size = files[src_pos].size;
                file.type = TYPE_FILE;
                file.access_rights = READ;
                int16_t pos = 2;
                find_free(pos);
                file.first_blk = pos;
                files[file_pos++] = file;
                fat[file.first_blk] = FAT_EOF;

                char buffer[BLOCK_SIZE] = {""};
                int16_t src_blk_no = files[src_pos].first_blk;
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

                disk.write(0, (uint8_t*)files);
                disk.write(1, (uint8_t*)fat);
            }
            else
            {
                std::cout << "Disk has no more room for additional data" << std::endl;
            }
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

    int src_pos = find_entry(sourcepath);
    int dest_pos = find_entry(destpath);

    if(src_pos + 1)
    {
        if(dest_pos + 1)
        {
            if(files[dest_pos].type == TYPE_DIR)
            {
                // Add file to sub-directory
                dir_entry file;
                file = files[src_pos];
                cd(files[dest_pos].file_name);
                files[file_pos++] = file;
                disk.write(current_blk, (uint8_t*)files);

                // Remove file from current directory
                cd("..");
                for(; src_pos < file_pos; src_pos++)
                {
                    files[src_pos] = files[src_pos + 1];
                }
                file_pos--;
                disk.write(current_blk, (uint8_t*)files);
            }
            else
            {
                std::cout << "mv: target '" << destpath << "' is not a directory" << std::endl;
            }
        }
        else
        {
            strcpy(files[src_pos].file_name, destpath.c_str());
            disk.write(0, (uint8_t*)files);
        }
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

    int pos = find_entry(filepath);
    char buffer[1] = "";

    if(pos + 1)
    {
        if(files[pos].type == TYPE_DIR)
        {
            cd(files[pos].file_name);
            if(file_pos > 1)
            {
                std::cout << "rm: cannot remove '" << filepath << "': Directory is not empty" << std::endl;
            }
            else
            {
                cd("..");
                char empty[1] = "";
                disk.write(files[pos].first_blk, (uint8_t*)empty);
                // reorganize
                for (; pos < file_pos; pos++)
                {
                    files[pos] = files[pos + 1];
                }
                
                file_pos--;
            }
        }
        else
        {
            int16_t blk_no = files[pos].first_blk;
            
            for (; pos < file_pos; pos++)
            {
                files[pos] = files[pos + 1];
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

            disk.write(0, (uint8_t*)files);
            disk.write(1, (uint8_t*)fat);
        }
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

    int src_pos = find_entry(filepath1);
    int dest_pos = find_entry(filepath2);

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
        if(files[src_pos].type == TYPE_DIR)
        {
            std::cout << "append: " << filepath1 << ": Is a directory" << std::endl;
        }
        else if(files[dest_pos].type == TYPE_DIR)
        {
            std::cout << "append: " << filepath2 << ": Is a directory" << std::endl;
        }
        else
        {
            if(file_pos < MAX_NO_FILES && files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
            {
                char buffer[BLOCK_SIZE] = {""};
                int16_t dest_blk_no = files[dest_pos].first_blk;

                while(fat[dest_blk_no] != FAT_EOF)
                {
                    dest_blk_no = fat[dest_blk_no];
                }

                int16_t src_blk_no = files[src_pos].first_blk;
                int16_t prev_pos = dest_blk_no;
                find_free(++dest_blk_no);
                fat[prev_pos] = dest_blk_no;

                while(fat[src_blk_no] != FAT_EOF)
                {
                    disk.read(src_blk_no, (uint8_t*)buffer);
                    disk.write(dest_blk_no, (uint8_t*)buffer);
                    files[dest_pos].size += strlen(buffer);
                    src_blk_no = fat[src_blk_no];
                    int16_t prev_pos = dest_blk_no;
                    find_free(++dest_blk_no);
                    fat[prev_pos] = dest_blk_no;
                }

                disk.read(src_blk_no, (uint8_t*)buffer);
                disk.write(dest_blk_no, (uint8_t*)buffer);
                files[dest_pos].size += strlen(buffer);
                fat[dest_blk_no] = FAT_EOF;
                disk.write(0, (uint8_t*)files);
                disk.write(1, (uint8_t*)fat);
            }
            else
            {
                std::cout << "Disk has no more room for additional data" << std::endl;
            }
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

    if(find_entry(dirpath) == -1)
    {
        dir_entry sub_dir;
        int16_t pos = 2;

        // Add name
        strcpy(sub_dir.file_name, dirpath.c_str());

        sub_dir.size = 0;

        // Find available first block
        find_free(pos);
        sub_dir.first_blk = pos;
        fat[pos] = FAT_EOF;

        sub_dir.type = TYPE_DIR;
        sub_dir.access_rights = READ;
        files[file_pos++] = sub_dir;

        dir_entry sub_dir_files[MAX_NO_FILES];
        for(unsigned int i = 1; i < MAX_NO_FILES; i++)
        {
            strcpy(sub_dir_files[i].file_name, "");
        }
        strcpy(sub_dir_files[0].file_name, "..");
        sub_dir_files[0].first_blk = current_blk;
        sub_dir_files[0].type = TYPE_DIR;
        sub_dir_files[0].access_rights = READ;

        disk.write(sub_dir.first_blk, (uint8_t*)sub_dir_files);
        disk.write(current_blk, (uint8_t*)files);
    }
    else
    {
        std::cout << "mkdir: cannot create directory '" << dirpath << "': File exists" << std::endl;
    }

    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    std::cout << "FS::cd(" << dirpath << ")\n";

    int pos = find_entry(dirpath);

    if(pos + 1)
    {
        if(files[pos].type)
        {
            if(dirpath == "..")
            {
                cwd.erase(cwd.find_last_of("/"));
            }
            else
            {
                cwd += "/" + dirpath;
            }
            current_blk = files[pos].first_blk;
            disk.read(current_blk, (uint8_t*)files);
            file_pos = 0;
            while(file_pos < MAX_NO_FILES && strcmp(files[file_pos].file_name, "") != 0)
            {
                file_pos++;
            }
        }
        else
        {
            std::cout << "cd: " << dirpath << ": Not a directory" << std::endl;
        }
    }
    else
    {
        std::cout << "cd: " << dirpath << ": No such file or directory" << std::endl;
    }
        
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
    std::cout << "FS::pwd()\n";

    if(cwd == "")
    {
        std::cout << "/" << std::endl;
    }
    else
    {
        std::cout << cwd << std::endl;
    }
    
    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int
FS::chmod(std::string accessrights, std::string filepath)
{
    std::cout << "FS::chmod(" << accessrights << "," << filepath << ")\n";
    int entry = find_entry(filepath);
    if(entry == -1){
        std::cout << "something" << std::endl;
    }
    else{
        uint8_t access_int = std::stoi(accessrights);
        if(access_int >=0 && access_int <= READ+WRITE+EXECUTE){
            files[entry].access_rights = access_int;
        }
        else{
            std::cout << "unavailable accessright" << std::endl;
        }
    }
    
    return 0;
}
