#include <iostream>
#include "fs.h"
#include "cstring"

// om vi kör cd .. i root ska vi inte få error
// cp byter map t.ex cp ../file1 file2
//något fel med namn i cp

FS::FS()
{
    // Root block
    cwd = "";
    current_blk = ROOT_BLOCK;
    change_dir();

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

int FS::tmp_enter(std::string &path) // TODO: Handle if name contains '/'
{
    std::string delimitor = "/";
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    size_t end = 0;
    while((end = path.find(delimitor)) != std::string::npos)
    {
        std::string entry = path.substr(0, end);

        if(find_entry(entry) != -1)
        {
            cd(entry);
        }
        else
        {
            current_blk = start_blk;
            cwd = start_cwd;
            change_dir();

            return -1;
        }
        
        path.erase(0, end + delimitor.length());
    }

    return 0;
}

void FS::change_dir()
{
    disk.read(current_blk, (uint8_t*)files);
    file_pos = 0;
    while(file_pos < MAX_NO_FILES && strcmp(files[file_pos].file_name, "") != 0)
    {
        file_pos++;
    }
}

// formats the disk, i.e., creates an empty file system
int
FS::format() // TODO: FIX
{
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
    std::string name = filepath;
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    if(tmp_enter(name) == -1)
    {
        std::cout << "create: cannot create file '" << filepath << "': No such file or directory" << std::endl;
    }
    else
    {
        if(name.size() < 57)
        {
            if(find_entry(name) + 1)
            {
                std::cout << "create: cannot create file '" << filepath << "': File exists" << std::endl;
            }
            else
            {
                if(file_pos < MAX_NO_FILES)
                {
                    int16_t pos = 2;
                    std::string data = "";
                    std::string cmp = " ";

                    // Create entry
                    dir_entry file;
                    while(cmp != ""){
                        std::getline(std::cin, cmp);
                        data += cmp;
                    }
                    
                    file.size = data.size();
                    std::strcpy(file.file_name, name.c_str());
                    file.type = TYPE_FILE;
                    file.access_rights = (READ | WRITE);
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
                    std::cout << "create: Disk has no more room for additional data" << std::endl;
                }
            } 
        }
        else
        {
            std::cout << "create: " << name << ": file name too big" << std::endl;
        }
        
        current_blk = start_blk;
        cwd = start_cwd;
        change_dir();
    }
    

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    // if(number_of_sub_dir != -1)
    // {
    //     char buffer[BLOCK_SIZE] = {""};
    //     unsigned int i = find_entry(filepath);

    //     if(i + 1)
    //     {
    //         if((files[i].access_rights & READ))
    //         {
    //             if(files[i].type)
    //             {
    //                 std::cout << "cat: " << files[i].file_name << ": Is a directory" << std::endl;
    //             }
    //             else
    //             {
    //                 int16_t blk_no = files[i].first_blk;
    //                 while(fat[blk_no] != FAT_EOF)
    //                 {
    //                     disk.read(blk_no, (uint8_t*)buffer);
    //                     std::cout << buffer;
    //                     blk_no = fat[blk_no];
    //                 }
    //                 disk.read(blk_no, (uint8_t*)buffer);
    //                 std::cout << buffer << std::endl;
    //             }
    //         }
    //         else
    //         {
    //             std::cout << "cat: " << filepath << ": Permission denied" << std::endl;
    //         }
    //     }
    //     else
    //     {
    //         std::cout << "cat: " << filepath << ": No such file or directory" << std::endl;
    //     }
    //     for(int z = number_of_sub_dir; z > 0; z--){
    //         cd("..");
    //     }
    // }
    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::cout << "FS::ls()\n";

    std::cout << "name\t\t\t\ttype\t\t\t\taccessrights\t\t\t\tsize" << std::endl;

    std::string type = "dir";
    std::string size = "-";
    unsigned int i = 1;

    if(current_blk == ROOT_BLOCK)
    {
        i = 0;
    }

    for(; i < file_pos; i++)
    {
        std::string type = "dir";
        std::string size = "-";
        char rights[3] = {'-', '-', '-'};
        
        if(files[i].type == TYPE_FILE)
        {
            type = "file";
            size = std::to_string(files[i].size);
        }

        if(files[i].access_rights & READ)
        {
            rights[0] = 'r';
        }
        if(files[i].access_rights & WRITE)
        {
            rights[1] = 'w';
        }
        if(files[i].access_rights & EXECUTE)
        {
            rights[2] = 'x';
        }

        std::cout << files[i].file_name << "\t\t\t\t" << type << "\t\t\t\t" << rights << "\t\t\t\t\t" << size << std::endl;
    }

    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    // int src_pos;
    // int dest_pos;
    // dir_entry sourcefile;
    // int number_of_sub_dir = path_parser(sourcepath);

    // if(number_of_sub_dir != -1){
    //     src_pos = find_entry(sourcepath);
    //     sourcefile = files[src_pos];
    //     for(int z = number_of_sub_dir; z > 0; z--){
    //         cd("..");
    //     }
    // }
    // else{
    //     return -1;
    // } //////// FIXA OM FILEPATH INTE FINNS

    // number_of_sub_dir = path_parser(destpath);
    // if(number_of_sub_dir != -1){
    //     dest_pos = find_entry(destpath);
    // }
    // else{
    //     return -1;
    // } //////// FIXA OM FILEPATH INTE FINNS

    // if(src_pos == -1)
    // {
    //     std::cout << "cp: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    //     for(int z = number_of_sub_dir; z > 0; z--){
    //         cd("..");
    //     }
    // }
    // else
    // {
    //     if(sourcefile.access_rights & READ)
    //     {
    //         if(dest_pos + 1)
    //         {
    //             if(files[dest_pos].access_rights & WRITE)
    //             {
    //                 if(sourcefile.type)
    //                 {
    //                     std::cout << "cp: " << sourcefile.file_name << ": Is a directory" << std::endl;
    //                 }
    //                 else if(files[dest_pos].type)
    //                 {
    //                     std::cout << "cp: " << files[dest_pos].file_name << ": Is a directory" << std::endl;
    //                 }
    //                 else
    //                 {
    //                     if(file_pos < MAX_NO_FILES && sourcefile.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
    //                     {
    //                         files[dest_pos].size = sourcefile.size;
    //                         char buffer[BLOCK_SIZE] = {""};
    //                         int16_t src_blk_no = sourcefile.first_blk;
    //                         int16_t dest_blk_no = files[dest_pos].first_blk;

    //                         while(fat[src_blk_no] != FAT_EOF)
    //                         {
    //                             disk.read(src_blk_no, (uint8_t*)buffer);
    //                             disk.write(dest_blk_no, (uint8_t*)buffer);
    //                             src_blk_no = fat[src_blk_no];

    //                             if(fat[dest_blk_no] == FAT_EOF)
    //                             {
    //                                 int16_t prev_pos = dest_blk_no;
    //                                 find_free(++dest_blk_no);
    //                                 fat[prev_pos] = dest_blk_no;
    //                                 fat[dest_blk_no] = FAT_EOF;
    //                             }
    //                             else
    //                             {
    //                                 dest_blk_no = fat[dest_blk_no];
    //                             }
    //                         }
    //                         disk.read(src_blk_no, (uint8_t*)buffer);
    //                         disk.write(dest_blk_no, (uint8_t*)buffer);

    //                         disk.write(current_blk, (uint8_t*)files);
    //                         disk.write(FAT_BLOCK, (uint8_t*)fat);
    //                         for(int z = number_of_sub_dir; z > 0; z--){
    //                             cd("..");
    //                         }
    //                     }
    //                     else
    //                     {
    //                         std::cout << "cp: Disk has no more room for additional data" << std::endl;
    //                     }
    //                 }
    //             }
    //             else
    //             {
    //                 std::cout << "cp: cannot open '" << destpath << "' for writing: Permission denied" << std::endl;
    //             }
    //         }
    //         else
    //         {
    //             if(file_pos < MAX_NO_FILES && sourcefile.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
    //             {
    //                 dir_entry file;
    //                 std::strcpy(file.file_name, destpath.c_str());
    //                 file.size = sourcefile.size;
    //                 file.type = TYPE_FILE;
    //                 file.access_rights = (READ | WRITE);
    //                 int16_t pos = 2;
    //                 find_free(pos);
    //                 file.first_blk = pos;
    //                 files[file_pos++] = file;
    //                 fat[file.first_blk] = FAT_EOF;

    //                 char buffer[BLOCK_SIZE] = {""};
    //                 int16_t src_blk_no = sourcefile.first_blk;
    //                 int16_t dest_blk_no = file.first_blk;

    //                 while(fat[src_blk_no] != FAT_EOF)
    //                 {
    //                     disk.read(src_blk_no, (uint8_t*)buffer);
    //                     disk.write(dest_blk_no, (uint8_t*)buffer);
    //                     src_blk_no = fat[src_blk_no];

    //                     if(fat[dest_blk_no] == FAT_EOF)
    //                     {
    //                         std::cout << "EOF" << std::endl;
    //                         int16_t prev_pos = pos;
    //                         find_free(++dest_blk_no);
    //                         fat[prev_pos] = dest_blk_no;
    //                         fat[dest_blk_no] = FAT_EOF;
    //                     }
    //                     else
    //                     {
    //                         dest_blk_no = fat[dest_blk_no];
    //                     }
    //                 }
    //                 disk.read(src_blk_no, (uint8_t*)buffer);
    //                 disk.write(dest_blk_no, (uint8_t*)buffer);

    //                 files[file_pos++] = file;
    //                 disk.write(current_blk, (uint8_t*)files);
    //                 disk.write(1, (uint8_t*)fat);

    //                 for(int z = number_of_sub_dir; z > 0; z--){
    //                     cd("..");
    //                 }   
    //             }
    //             else
    //             {
    //                 std::cout << "cp: Disk has no more room for additional data" << std::endl;
    //             }
    //         }
    //     }
    //     else
    //     {
    //         std::cout << "cp: cannot open '" << sourcepath << "' for reading: Permission denied" << std::endl;
    //     }
    // }

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    // int src_pos;
    // int dest_pos;
    // std::string rm_path = sourcepath;
    // dir_entry file;
    // int number_of_sub_dir = path_parser(sourcepath);
    
    // if(number_of_sub_dir != -1){
    //     src_pos = find_entry(sourcepath);
    //     file = files[src_pos];
    //     for(int z = number_of_sub_dir; z > 0; z--){
    //         cd("..");
    //     }
    // }
    // else{
    //     return -1;
    // } //////// FIXA OM FILEPATH INTE FINNS

    // number_of_sub_dir = path_parser(destpath);
    // if(number_of_sub_dir != -1){
    //     dest_pos = find_entry(destpath);
    // }
    // else{
    //     return -1;
    // } //////// FIXA OM FILEPATH INTE FINNS

    // if(src_pos + 1)
    // {
    //     if(dest_pos + 1)
    //     {
    //         if(files[dest_pos].type == TYPE_DIR)
    //         {
    //             if(files[dest_pos].access_rights & WRITE)
    //             {
    //                 // Add file to sub-directory
    //                 files[file_pos++] = file;
    //                 disk.write(current_blk, (uint8_t*)files);
    //             }
    //         }
    //         else
    //         {
    //             std::cout << "mv: target '" << destpath << "' is not a directory" << std::endl;
    //         }
    //         for(int z = number_of_sub_dir; z > 0; z--){
    //             cd("..");
    //         }
    //         rm(rm_path);// Remove file from source directory
    //     }
    //     else
    //     {
    //         for(int z = number_of_sub_dir; z > 0; z--){
    //             cd("..");
    //         }
    //         strcpy(file.file_name, destpath.c_str());
    //         disk.write(current_blk, (uint8_t*)files);
    //     }
    // }
    // else
    // {
    //     std::cout << "mv: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    // }

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int
FS::rm(std::string filepath)
{
    // int number_of_sub_dir = path_parser(filepath);
    // if(number_of_sub_dir != -1){
    //     int pos = find_entry(filepath);
    //     char buffer[1] = "";

    //     if(pos + 1)
    //     {
    //         if(files[pos].type == TYPE_DIR)
    //         {
    //             cd(files[pos].file_name);
    //             if(file_pos > 1)
    //             {
    //                 std::cout << "rm: cannot remove '" << filepath << "': Directory is not empty" << std::endl;
    //             }
    //             else
    //             {
    //                 cd("..");
    //                 char empty[1] = "";
    //                 disk.write(files[pos].first_blk, (uint8_t*)empty);

    //                 for (; pos < file_pos; pos++)
    //                 {
    //                     files[pos] = files[pos + 1];
    //                 }
                    
    //                 file_pos--;
    //             }
    //         }
    //         else
    //         {
    //             int16_t blk_no = files[pos].first_blk;
                
    //             for (; pos < file_pos; pos++)
    //             {
    //                 files[pos] = files[pos + 1];
    //             }
                
    //             file_pos--;

    //             while(fat[blk_no] != FAT_EOF)
    //             {
    //                 disk.write(blk_no, (uint8_t*)buffer);
    //                 blk_no = fat[blk_no];
    //                 fat[blk_no] = FAT_FREE;
    //             }
    //             disk.write(blk_no, (uint8_t*)buffer);
    //             fat[blk_no] = FAT_FREE;

    //             disk.write(current_blk, (uint8_t*)files);
    //             disk.write(1, (uint8_t*)fat);
    //         }
    //     }
    //     else
    //     {
    //         std::cout << "rm: cannot remove '" << filepath << "': No such file or directory" << std::endl;
    //     }
    //     for(int z = number_of_sub_dir; z > 0; z--){
    //         cd("..");
    //     }
    // }
    // return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
//     int src_pos;
//     int dest_pos;
//     std::string rm_path = filepath1;
//     dir_entry file;
//     int number_of_sub_dir = path_parser(filepath1);
    
//     if(number_of_sub_dir != -1){
//         src_pos = find_entry(filepath1);
//         file = files[src_pos];
//         for(int z = number_of_sub_dir; z > 0; z--){
//             cd("..");
//         }
//     }
//     else{
//         return -1;
//     } //////// FIXA OM FILEPATH INTE FINNS

//     number_of_sub_dir = path_parser(filepath2);
//     if(number_of_sub_dir != -1){
//         dest_pos = find_entry(filepath2);
//     }
//     else{
//         return -1;
//     } //////// FIXA OM FILEPATH INTE FINNS

//     if(src_pos == -1)
//     {
//         std::cout << "append: cannot find '" << filepath1 << "': No such file or directory" << std::endl;
//         for(int z = number_of_sub_dir; z > 0; z--){
//             cd("..");
//         }
//     }
//     else if(dest_pos == -1)
//     {
//         std::cout << "append: cannot find '" << filepath2 << "': No such file or directory" << std::endl;
//         for(int z = number_of_sub_dir; z > 0; z--){
//             cd("..");
//         }
//     }
//     else if(filepath1 == filepath2)
//     {
//         std::cout << "\"You do not need to cover the special case when appending a file to itself, i.e., ’append <filename1> <filename1>’.\"" << std::endl << std::endl << "- Håkan Grahn" << std::endl;
//         for(int z = number_of_sub_dir; z > 0; z--){
//             cd("..");
//         }
//     }
//     else
//     {
//         if(file.type == TYPE_DIR)
//         {
//             std::cout << "append: " << filepath1 << ": Is a directory" << std::endl;
//         }
//         else
//         {
//             if(files[dest_pos].type == TYPE_DIR)
//             {
//                 std::cout << "append: " << filepath2 << ": Is a directory" << std::endl;
//             }
//             else
//             {
//                 if(file.access_rights & READ)
//                 {
//                     if(files[dest_pos].access_rights & WRITE)
//                     {
//                         if(file_pos < MAX_NO_FILES && file.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
//                         {
//                             char buffer[BLOCK_SIZE] = {""};
//                             int16_t dest_blk_no = files[dest_pos].first_blk;

//                             while(fat[dest_blk_no] != FAT_EOF)
//                             {
//                                 dest_blk_no = fat[dest_blk_no];
//                             }

//                             int16_t src_blk_no = file.first_blk;
//                             int16_t prev_pos = dest_blk_no;
//                             find_free(++dest_blk_no);
//                             fat[prev_pos] = dest_blk_no;

//                             while(fat[src_blk_no] != FAT_EOF)
//                             {
//                                 disk.read(src_blk_no, (uint8_t*)buffer);
//                                 disk.write(dest_blk_no, (uint8_t*)buffer);
//                                 files[dest_pos].size += strlen(buffer);
//                                 src_blk_no = fat[src_blk_no];
//                                 int16_t prev_pos = dest_blk_no;
//                                 find_free(++dest_blk_no);
//                                 fat[prev_pos] = dest_blk_no;
//                             }

//                             disk.read(src_blk_no, (uint8_t*)buffer);
//                             disk.write(dest_blk_no, (uint8_t*)buffer);
//                             files[dest_pos].size += strlen(buffer);
//                             fat[dest_blk_no] = FAT_EOF;
//                             disk.write(current_blk, (uint8_t*)files);
//                             disk.write(1, (uint8_t*)fat);
//                         }
//                         else
//                         {
//                             std::cout << "append: Disk has no more room for additional data" << std::endl;
//                         }
//                     }
//                     else
//                     {
//                         std::cout << "append: cannot open '" << filepath2 << "' for writing: Permission denied" << std::endl;
//                     }
//                 }
//                 else
//                 {
//                     std::cout << "append: cannot open '" << filepath1 << "' for reading: Permission denied" << std::endl;
//                 }
//             }
//         }
//     }
//     for(int z = number_of_sub_dir; z > 0; z--){
//         cd("..");
//     }
//     return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    std::string name = dirpath;
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;

    if(tmp_enter(name) == -1)
    {
        std::cout << "mkdir: cannot create directory '" << dirpath << "': No such file or directory" << std::endl;
    }
    else
    {
        if(name.size() < 57)
        {
            if(find_entry(name) == -1)
            {
                dir_entry sub_dir;
                int16_t pos = 2;

                // Add name
                strcpy(sub_dir.file_name, name.c_str());

                sub_dir.size = 0;

                // Find available first block
                find_free(pos);
                sub_dir.first_blk = pos;
                fat[pos] = FAT_EOF;

                sub_dir.type = TYPE_DIR;
                sub_dir.access_rights = (READ | WRITE | EXECUTE);
                files[file_pos++] = sub_dir;

                dir_entry sub_dir_files[MAX_NO_FILES];
                for(unsigned int i = 1; i < MAX_NO_FILES; i++)
                {
                    strcpy(sub_dir_files[i].file_name, "");
                }
                strcpy(sub_dir_files[0].file_name, "..");
                sub_dir_files[0].first_blk = current_blk;
                sub_dir_files[0].type = TYPE_DIR;
                sub_dir_files[0].access_rights = (READ | WRITE | EXECUTE);

                disk.write(sub_dir.first_blk, (uint8_t*)sub_dir_files);
                disk.write(current_blk, (uint8_t*)files);
            }
            else
            {
                std::cout << "mkdir: cannot create directory '" << dirpath << "': File exists" << std::endl;
            }
        }
        else
        {
            std::cout << "mkdir: " << name << ": file name too big" << std::endl;
        }
        
        current_blk = start_blk;
        cwd = start_cwd;
        change_dir();
    }
    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int
FS::cd(std::string dirpath)
{
    // int number_of_sub_dir = path_parser(dirpath);
    // if(number_of_sub_dir != -1){
    //     int pos = find_entry(dirpath);

    //     if(pos + 1)
    //     {
    //         if(files[pos].type == TYPE_DIR)
    //         {
    //             if(dirpath == "..")
    //             {
    //                 cwd.erase(cwd.find_last_of("/"));
    //             }
    //             else
    //             {
    //                 cwd += "/" + dirpath;
    //             }
    //             current_blk = files[pos].first_blk;
    //             change_dir();
    //         }
    //         else
    //         {
    //             std::cout << "cd: " << dirpath << ": Not a directory" << std::endl;
    //             return -1;
    //         }
    //     }
    //     else
    //     {
    //         std::cout << "cd: " << dirpath << ": No such file or directory" << std::endl;
    //         return -1;
    //     }
    // }
    // return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int
FS::pwd()
{
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
    // int number_of_sub_dir = path_parser(filepath);
    // if(number_of_sub_dir != -1){
    //     int entry = find_entry(filepath);

    //     if(entry == -1)
    //     {
    //         std::cout << "chmod: " << filepath << ": No such file or directory" << std::endl;
    //     }
    //     else
    //     {
    //         uint8_t access_int = std::stoi(accessrights);

    //         if(access_int >=0 && access_int <= (READ|WRITE|EXECUTE))
    //         {
    //             files[entry].access_rights = access_int;
    //             disk.write(current_blk, (uint8_t*)files);
    //         }
    //         else
    //         {
    //             std::cout << "chmod: invalid access right" << std::endl;
    //         }
    //     }
    //     for(int z = number_of_sub_dir; z > 0; z--){
    //         cd("..");
    //     }
    // }
    // return 0;
}