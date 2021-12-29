#include <iostream>
#include "fs.h"
#include "cstring"
#include <regex>
#include <vector>

// om vi kör cd .. i root ska vi inte få error
// cp byter map t.ex cp ../file1 file2
//något fel med namn i cp

FS::FS()
{
    // Root block
    cwd = "/";
    current_blk = ROOT_BLOCK;
    change_dir();

    // Fat Block
    disk.read(FAT_BLOCK, (uint8_t *)fat);
}

FS::~FS()
{
}

// Looks for first available position in FAT
void FS::find_free(int16_t &first)
{
    while (first < BLOCK_SIZE / 2 && fat[first] != FAT_FREE)
    {
        first++;
    }

    if (fat[first] != FAT_FREE)
    {
        first = -1;
    }
}

// Looks for entry in current block
int FS::find_entry( std::string path)
{
    uint16_t start_blk = current_blk;
    std::string tmp_path = path;
    for (int i = 0; i < file_pos; i++)
    {
        if(strcmp(files[i].file_name, path.c_str()) == 0){
            return i;
        }
    }
    return -1;
}

// Changes directory
void FS::change_dir()
{
    disk.read(current_blk, (uint8_t *)files);
    file_pos = 0;
    while (file_pos < MAX_NO_FILES && strcmp(files[file_pos].file_name, "") != 0) // TODO: Add better comparison
    {
        file_pos++;
    }
}

// formats the disk, i.e., creates an empty file system
int FS::format()
{
    // Root block
    char empty[1] = "";
    file_pos = 0;
    cwd = "/";
    disk.write(ROOT_BLOCK, (uint8_t *)empty);
    disk.read(ROOT_BLOCK, (uint8_t *)files);

    // FAT block
    fat[ROOT_BLOCK] = FAT_EOF;
    fat[FAT_BLOCK] = FAT_EOF;
    disk.write(FAT_BLOCK, (uint8_t *)fat);

    // Rest of disk
    for (unsigned int i = 2; i < BLOCK_SIZE / 2; i++)
    {
        disk.write(i, (uint8_t *)empty);
        fat[i] = FAT_FREE;
    }
    return 0;
}

// create <filepath> creates a new file on the disk, the data content is
// written on the following rows (ended with an empty row)
int FS::create(std::string filepath)
{
    size_t last_slash = filepath.find_last_of("/");

    if (last_slash != filepath.size() - 1)
    {
        uint16_t start_blk = current_blk;
        std::string start_cwd = cwd;
        std::string name = filepath;

        // Check if cd is required
        if (last_slash != std::string::npos)
        {
            if (cd(filepath.substr(0, last_slash)) != 0)
            {
                return 0;
            }
            else
            {
                name = filepath.substr(last_slash+1);
            }
        }

        if (filepath.size() < 57)
        {
            if (find_entry(name) + 1)
            {
                std::cout << "create: cannot create file '" << filepath << "': File exists" << std::endl;
            }
            else
            {
                if (file_pos < MAX_NO_FILES)
                {
                    int16_t pos = 2;
                    std::string data = "";
                    std::string cmp = " ";

                    // Store input
                    while (cmp != "")
                    {
                        std::getline(std::cin, cmp);
                        data += cmp + "\n";
                    }
                    data.pop_back();

                    // Create entry
                    dir_entry file;
                    file.size = data.size(); // Newlines are included here
                    std::strcpy(file.file_name, name.c_str());
                    file.type = TYPE_FILE;
                    file.access_rights = (READ | WRITE);
                    find_free(pos);
                    file.first_blk = pos;
                    disk.write(file.first_blk, (uint8_t *)data.substr(0, BLOCK_SIZE - 1).c_str());
                    files[file_pos++] = file;
                    disk.write(current_blk, (uint8_t *)files);

                    if (file.size > BLOCK_SIZE && file.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
                    {
                        unsigned int res = file.size / BLOCK_SIZE - 1;

                        if (file.size % BLOCK_SIZE != 0)
                        {
                            res++;
                        }

                        unsigned int next = 1;

                        for (res; res > 0; res--)
                        {
                            int16_t prev_pos = pos;
                            find_free(++pos);
                            fat[prev_pos] = pos;
                            disk.write(pos, (uint8_t *)data.substr(BLOCK_SIZE * next, BLOCK_SIZE * (next + 1) - 1).c_str());
                            next++;
                        }
                    }

                    fat[pos] = FAT_EOF;
                    disk.write(1, (uint8_t *)fat);
                }
                else
                {
                    std::cout << "create: Disk has no more room for additional data" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "create: " << filepath << ": File name exceeds 56 characters" << std::endl;
        }

        // Go back to cwd TODO: Add if statement if necessary
        current_blk = start_blk;
        cwd = start_cwd;
        change_dir();
    }
    else
    {
        std::cout << "create: cannot create file '" << filepath << "', did you mean: " << std::endl
                  << std::endl
                  << "\tmkdir" << std::endl
                  << std::endl;
    }

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int FS::cat(std::string filepath)
{
    int last_slash = filepath.find_last_of("/");

    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    std::string name = filepath;
    char buffer[BLOCK_SIZE] = {""};

    // Check if cd is required
    if (last_slash != -1)
    {
        if (cd(filepath.substr(0, last_slash)) != 0)
        {
            return 0;
        }
        else
        {
            name = filepath.substr(last_slash+1);
        }
    }
    int i = find_entry(name);

    if (i != -1)
    {
        if ((files[i].access_rights & READ))
        {
            if (files[i].type)
            {
                std::cout << "cat: " << files[i].file_name << ": Is a directory" << std::endl;
            }
            else
            {
                int16_t blk_no = files[i].first_blk;

                while (fat[blk_no] != FAT_EOF)
                {
                    disk.read(blk_no, (uint8_t *)buffer);
                    std::cout << buffer;
                    blk_no = fat[blk_no];
                }
                disk.read(blk_no, (uint8_t *)buffer);
                std::cout << buffer;
            }
        }
        else
        {
            std::cout << "cat: " << filepath << ": Permission denied" << std::endl;
        }
    }
    else
    {
        std::cout << "cat: " << filepath << ": No such file or directory" << std::endl;
    }

    // Go back to cwd TODO: Add if statement if necessary
    current_blk = start_blk;
    cwd = start_cwd;
    change_dir();

    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int FS::ls()
{
    std::cout << "name\t\t\t\ttype\t\t\t\taccessrights\t\t\t\tsize" << std::endl;

    unsigned int i = 1;

    if (current_blk == ROOT_BLOCK)
    {
        i = 0;
    }

    for (; i < file_pos; i++)
    {
        std::string type = "dir";
        std::string size = "-";
        char rights[3] = {'-', '-', '-'};

        if (files[i].type == TYPE_FILE)
        {
            type = "file";
            size = std::to_string(files[i].size);
        }

        if (files[i].access_rights & READ)
        {
            rights[0] = 'r';
        }
        if (files[i].access_rights & WRITE)
        {
            rights[1] = 'w';
        }
        if (files[i].access_rights & EXECUTE)
        {
            rights[2] = 'x';
        }

        std::cout << files[i].file_name << "\t\t\t\t" << type << "\t\t\t\t" << rights << "\t\t\t\t\t" << size << std::endl;
    }

    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int FS::cp(std::string sourcepath, std::string destpath)
{
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    std::string name = sourcepath;
    std::string new_dest_path = destpath;
    int last_src_slash = sourcepath.find_last_of("/");
    if(last_src_slash != -1)
    {
        cd(sourcepath.substr(0, last_src_slash +1));
        name = sourcepath.substr(last_src_slash+1);
    }

    // Save data from current directory
    int src_pos = find_entry(name);
    dir_entry sourcefile = files[src_pos];

    // Go back to cwd
    current_blk = start_blk;
    cwd = start_cwd;
    change_dir();

    if (src_pos == -1)
    {
        std::cout << "cp: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    }
    else
    {
        if (sourcefile.access_rights & READ)
        {
            int last_dest_slash = new_dest_path.find_last_of("/");
            if(last_dest_slash != -1){
                cd(destpath.substr(0, last_dest_slash + 1));
                new_dest_path = destpath.substr(last_dest_slash + 1);
            }
            int dest_pos = find_entry(new_dest_path);

            if (dest_pos + 1)
            {
                std::cout << "cp: cannot copy to file '" << destpath << "': File exists" << std::endl;
            }
            else // Destination file does not exist
            {
                if (file_pos < MAX_NO_FILES && sourcefile.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
                {
                    dir_entry file;
                    std::strcpy(file.file_name, new_dest_path.c_str());
                    file.size = sourcefile.size;
                    file.type = TYPE_FILE;
                    file.access_rights = (READ | WRITE);
                    int16_t pos = 2;
                    find_free(pos);
                    file.first_blk = pos;
                    fat[file.first_blk] = FAT_EOF;

                    char buffer[BLOCK_SIZE] = {""};
                    int16_t src_blk_no = sourcefile.first_blk;
                    int16_t dest_blk_no = file.first_blk;

                    while (fat[src_blk_no] != FAT_EOF)
                    {
                        disk.read(src_blk_no, (uint8_t *)buffer);
                        disk.write(dest_blk_no, (uint8_t *)buffer);
                        src_blk_no = fat[src_blk_no];

                        if (fat[dest_blk_no] == FAT_EOF)
                        {
                            int16_t prev_blk = dest_blk_no;
                            find_free(++dest_blk_no);
                            fat[prev_blk] = dest_blk_no;
                            fat[dest_blk_no] = FAT_EOF;
                        }
                        else
                        {
                            dest_blk_no = fat[dest_blk_no];
                        }
                    }
                    disk.read(src_blk_no, (uint8_t *)buffer);
                    disk.write(dest_blk_no, (uint8_t *)buffer);

                    files[file_pos++] = file;
                    disk.write(current_blk, (uint8_t *)files);
                    disk.write(1, (uint8_t *)fat);

                    // Go back to cwd
                    current_blk = start_blk;
                    cwd = start_cwd;
                    change_dir();
                }
                else
                {
                    std::cout << "cp: Disk has no more room for additional data" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "cp: cannot open '" << sourcepath << "' for reading: Permission denied" << std::endl;
        }
    }
    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int FS::mv(std::string sourcepath, std::string destpath)
{
    std::string rm_path = sourcepath;
    std::string src_name = sourcepath;
    std::string dest_name = destpath;

    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;

    // cd down to second last source dir
    int last_src_slash = sourcepath.find_last_of("/");
    if (last_src_slash != -1)
    {
        cd(sourcepath.substr(0, last_src_slash + 1));
        src_name = sourcepath.substr(last_src_slash+1);
    }

    // Save information from the directory
    uint16_t src_blk = current_blk;
    int src_pos = find_entry(src_name);
    dir_entry src_file = files[src_pos];

    // Go back to cwd
    current_blk = start_blk;
    cwd = start_cwd;
    change_dir();

    if (src_pos + 1)
    {
        // cd down to second last dest directory
        int last_dest_slash = destpath.find_last_of("/");
        if (last_dest_slash != -1)
        {
            cd(destpath.substr(0, last_dest_slash + 1));
            dest_name = destpath.substr(last_dest_slash + 1); // TODO: Add handling for / in name
        }

        int dest_pos = find_entry(dest_name);

        if (dest_pos + 1)
        {
            if (files[dest_pos].type == TYPE_DIR)
            {
                if (files[dest_pos].access_rights & WRITE)
                {
                    cd(dest_name);
                    // Add file to sub-directory
                    files[file_pos++] = src_file;
                    disk.write(current_blk, (uint8_t *)files);

                    // Go back to cwd
                    current_blk = start_blk;
                    cwd = start_cwd;
                    change_dir();
                    rm(rm_path); // Remove file from source directory
                }
                else
                {
                    std::cout << "mv: cannot open '" << destpath << "' for writing: Permission denied" << std::endl;
                }
            }
            else
            {
                std::cout << "mv: cannot move file '" << dest_name << "': File exists" << std::endl;
            }
        }
        else
        {            
            // Change file name and save to source block
            strcpy(src_file.file_name, dest_name.c_str());

            // Add file to sub-directory
            files[file_pos++] = src_file;
            disk.write(current_blk, (uint8_t *)files);

            // Go back to cwd
            current_blk = start_blk;
            cwd = start_cwd;
            change_dir();
            rm(rm_path); // Remove file from source directory
        }

        // Go back to cwd
        current_blk = start_blk;
        cwd = start_cwd;
        change_dir();
    }
    else
    {
        std::cout << "mv: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    }

    return 0;
}

// rm <filepath> removes / deletes the file <filepath>
int FS::rm(std::string filepath)
{
    std::string name = filepath;
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    int last_slash = filepath.find_last_of("/");

    if (last_slash != -1)
    {
        cd(filepath.substr(0, last_slash + 1));
        name = filepath.substr(last_slash + 1);
    }

    int pos = find_entry(name);

    if (pos + 1)
    {
        if (files[pos].type == TYPE_DIR)
        {
            char empty[1] = "";
            cd(files[pos].file_name);
            if (file_pos > 1) // First entry is ".."
            {
                std::cout << "rm: cannot remove '" << filepath << "': Directory is not empty" << std::endl;
                // Go back to cwd
                current_blk = start_blk;
                cwd = start_cwd;
                change_dir();
            }
            else // "rm <dirname> that removes the directory dirname if the directory is empty" - Håkan Grahn
            {
                cd("..");

                //files[pos].file_name = "";
                strcpy(files[pos].file_name, empty);
                disk.write(files[pos].first_blk, (uint8_t *)empty);
                for (; pos < file_pos; pos++)
                {
                    files[pos] = files[pos + 1];
                }

                file_pos--;
                disk.write(current_blk, (uint8_t *)files);

                // Go back to cwd
                current_blk = start_blk;
                cwd = start_cwd;
                change_dir();
            }
        }
        else
        {
            char empty[1] = "";
            int16_t blk_no = files[pos].first_blk;

            for (; pos < file_pos; pos++)
            {
                files[pos] = files[pos + 1];
            }

            file_pos--;
            
            while (fat[blk_no] != FAT_EOF)
            {
                uint16_t prev_blk = blk_no;
                disk.write(blk_no, (uint8_t *)empty);
                blk_no = fat[blk_no];
                fat[prev_blk] = FAT_FREE;
            }
            uint16_t prev_blk = blk_no;
            disk.write(blk_no, (uint8_t *)empty);
            blk_no = fat[blk_no];
            fat[prev_blk] = FAT_FREE;

            disk.write(current_blk, (uint8_t *)files);
            disk.write(1, (uint8_t *)fat);

            // Go back to cwd
            current_blk = start_blk;
            cwd = start_cwd;
            change_dir();
            disk.read(current_blk, (uint8_t *)files);
        }
    }
    else
    {
        std::cout << "rm: cannot remove '" << filepath << "': No such file or directory" << std::endl;

        // Go back to cwd
        current_blk = start_blk;
        cwd = start_cwd;
        change_dir();
        disk.read(current_blk, (uint8_t *)files);
    }

    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int FS::append(std::string filepath1, std::string filepath2)
{
    
    std::string rm_path = filepath1;
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    std::string path_1 = filepath1;
    std::string path_2 = filepath2;
    
    int last_src_slash = filepath1.find_last_of("/");

    if (last_src_slash != -1)
    {
        cd(filepath1.substr(0, last_src_slash + 1));
        path_1 = filepath1.substr(last_src_slash + 1);
    }
    int src_pos = find_entry(path_1);

    dir_entry file1 = files[src_pos];
    uint8_t src_rights = files[src_pos].access_rights;

    // Go back to cwd
    current_blk = start_blk;
    cwd = start_cwd;
    change_dir();

    if (src_pos == -1)
    {
        std::cout << "append: cannot find '" << filepath1 << "': No such file or directory" << std::endl;
    }
    else if (filepath1 == filepath2)
    {
        std::cout << "\"You do not need to cover the special case when appending a file to itself, i.e., ’append <filename1> <filename1>’.\"" << std::endl
                  << std::endl
                  << "- Håkan Grahn" << std::endl;
    }
    else
    {
        if (file1.type == TYPE_DIR)
        {
            std::cout << "append: " << filepath1 << ": Is a directory" << std::endl;
        }
        else
        {
            int last_dest_slash = filepath2.find_last_of("/");

            if (last_dest_slash != -1)
            {
                cd(filepath2.substr(0, last_dest_slash + 1));
                path_2 = filepath2.substr(last_dest_slash + 1);
            }

            int dest_pos = find_entry(path_2);
            if (dest_pos == -1)
            {
                std::cout << "append: cannot find '" << filepath2 << "': No such file or directory" << std::endl;
                return 0;
            }

            if (files[dest_pos].type == TYPE_DIR)
            {
                std::cout << "append: " << filepath2 << ": Is a directory" << std::endl;
            }
            else
            {
                if (src_rights & READ)
                {
                    if (files[dest_pos].access_rights & WRITE)
                    {
                        if (file_pos < MAX_NO_FILES && file1.size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
                        {
                            char buffer[BLOCK_SIZE] = {""};
                            int16_t dest_blk_no = files[dest_pos].first_blk;

                            while (fat[dest_blk_no] != FAT_EOF)
                            {
                                dest_blk_no = fat[dest_blk_no];
                            }

                            int16_t src_blk_no = file1.first_blk;
                            int16_t prev_blk = dest_blk_no;
                            find_free(++dest_blk_no);
                            fat[prev_blk] = dest_blk_no;

                            while (fat[src_blk_no] != FAT_EOF)
                            {
                                disk.read(src_blk_no, (uint8_t *)buffer);
                                disk.write(dest_blk_no, (uint8_t *)buffer);
                                files[dest_pos].size += strlen(buffer);
                                src_blk_no = fat[src_blk_no];
                                prev_blk = dest_blk_no;
                                find_free(++dest_blk_no);
                                fat[prev_blk] = dest_blk_no;
                            }
                            disk.read(src_blk_no, (uint8_t *)buffer);
                            disk.write(dest_blk_no, (uint8_t *)buffer);
                            files[dest_pos].size += strlen(buffer);
                            fat[dest_blk_no] = FAT_EOF;

                            disk.write(current_blk, (uint8_t *)files);
                            disk.write(1, (uint8_t *)fat);
                        }
                        else
                        {
                            std::cout << "append: Disk has no more room for additional data" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "append: cannot open '" << filepath2 << "' for writing: Permission denied" << std::endl;
                    }
                }
                else
                {
                    std::cout << "append: cannot open '" << filepath1 << "' for reading: Permission denied" << std::endl;
                }
            }

            // Go back to cwd
            current_blk = start_blk;
            cwd = start_cwd;
            change_dir();
        }
    }
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int FS::mkdir(std::string dirpath)
{
    std::string name = dirpath;
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    int last_slash = dirpath.find_last_of("/");

    if (last_slash != -1)
    {
        cd(dirpath.substr(0, last_slash));
        name = dirpath.substr(last_slash + 1);
    }

    if (name.size() < 57)
    {
        if (find_entry(name) == -1)
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
            sub_dir.access_rights = (READ | WRITE);
            files[file_pos++] = sub_dir;

            dir_entry sub_dir_files[MAX_NO_FILES];
            strcpy(sub_dir_files[0].file_name, "..");
            for (unsigned int i = 1; i < MAX_NO_FILES; i++)
            {
                strcpy(sub_dir_files[i].file_name, "");
            }
            sub_dir_files[0].first_blk = current_blk;
            sub_dir_files[0].type = TYPE_DIR;
            sub_dir_files[0].access_rights = (READ | WRITE);

            //files[sub_dir.first_blk] = sub_dir;
            disk.write(sub_dir.first_blk, (uint8_t *)sub_dir_files);
            disk.write(current_blk, (uint8_t *)files);
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

    return 0;
}

// cd <dirpath> changes the current (working) directory to the directory named <dirpath>
int FS::cd(std::string dirpath) // TODO: Add return support for file fault error message
{
    uint16_t start_blk = current_blk;
    std::string dir_name = dirpath;
    std::string start_cwd = cwd;
    while (dir_name[dir_name.size() - 1] == '/' && dir_name.size() > 1) // Remove all '/' at the end
    {
        dir_name.erase(dir_name.size()-1);
    }
    if (dir_name[0] == '/') // From root
    {
        current_blk = ROOT_BLOCK;
        cwd = "/";
        change_dir();

        if (dirpath == "/")
        {
            return 0;
        }
        else
        {
            dir_name = dir_name.substr(1);
        }
    }
    std::vector<std::string> new_path = {};
    std::string split_token = "/";
    std::string tmp = "..";
    int pos = 0;
    while ((pos = dir_name.find(split_token)) != -1) {
        new_path.push_back(dir_name.substr(0, pos));
        dir_name.erase(0, pos + split_token.length());
    }
    new_path.push_back(dir_name);
    for(int i=0; i < new_path.size(); i++){
        if(strcmp(tmp.c_str(), new_path[i].c_str()) == 0){
            if(current_blk != ROOT_BLOCK)
            {
                int last_slash = cwd.find_last_of("/");
                current_blk = files[0].first_blk;
                cwd.erase(last_slash, cwd.size());
                change_dir();
            }
            else
            {
                //do nothing
            }
        }
        else{
            int dir_nmr = find_entry(new_path[i]);
            if(dir_nmr != -1){
                if(files[dir_nmr].type == TYPE_DIR){
                    current_blk = files[dir_nmr].first_blk;
                    cwd += "/";
                    cwd += new_path[i];
                    change_dir();
                }
                else{
                    // Go back if fail
                    current_blk = start_blk;
                    cwd = start_cwd;
                    change_dir();
                    std::cout << "cd: " << dirpath << ": Not a directory" << std::endl;
                    return 0;
                }
            }
            else{
                // Go back if fail
                current_blk = start_blk;
                cwd = start_cwd;
                change_dir();
                std::cout << "cd: " << dirpath << ": Not a directory" << std::endl;
                return 0;
            }
        }
    }
    return 0;
}

// pwd prints the full path, i.e., from the root directory, to the current
// directory, including the currect directory name
int FS::pwd()
{
    if (current_blk == ROOT_BLOCK)
    {
        std::cout << cwd << std::endl;
    }
    else
    {
        std::cout << cwd.substr(1) << std::endl;
    }

    return 0;
}

// chmod <accessrights> <filepath> changes the access rights for the
// file <filepath> to <accessrights>.
int FS::chmod(std::string accessrights, std::string filepath)
{
    std::string name = filepath;
    uint16_t start_blk = current_blk;
    std::string start_cwd = cwd;
    int last_slash = filepath.find_last_of("/");

    if (last_slash != -1)
    {
        cd(filepath.substr(0, last_slash + 1));
        name = filepath.substr(last_slash + 1);
    }

    int entry = find_entry(name);

    if (entry == -1)
    {
        std::cout << "chmod: " << filepath << ": No such file or directory" << std::endl;
    }
    else
    {
        std::regex reg("^[0-7]$");

        if (std::regex_match(accessrights, reg))
        {
            uint8_t access_int = std::stoi(accessrights);

            files[entry].access_rights = access_int;
            disk.write(current_blk, (uint8_t *)files);
        }
        else
        {
            std::cout << "chmod: invalid access right: '" << accessrights << "'" << std::endl;
        }
    }

    current_blk = start_blk;
    cwd = start_cwd;
    change_dir();

    return 0;
}