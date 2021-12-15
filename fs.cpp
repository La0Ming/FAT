#include <iostream>
#include "fs.h"
#include "cstring"

FS::FS()
{
    std::cout << "FS::FS()... Creating file system\n";

    // Root block
    //disk.write(0, (uint8_t*)root);
    root = new Node();
    root->cwd = "";
    disk.read(0, (uint8_t*)files);
    for(auto file : files)
    {
        Node *entry = new Node();
        entry->key = file;
        entry->cwd = root->cwd + "/" + file.file_name;
        root->children.push_back(entry);
    }
    root->cwd = "/";

    // Fat Block
    disk.read(1, (uint8_t*)fat);
}

FS::~FS()
{
    // cd back to root
    while(root->cwd != "/")
    {
        cd("..");
    }

    // Sort the root block
    file_pos = 0;
    sort(root->children);
}

void FS::build_tree()
{
    for(auto file : files)
    {
        Node *entry = new Node();
        entry->key = file;
        entry->cwd = root->cwd + "/" + file.file_name;
        root->children.push_back(entry);
    }
}

void FS::sort(std::vector<Node*> children)
{
    std::vector<Node*> dirs;
    for(auto child : children)
    {
        if(!child->key.type)
        {
            files[file_pos++] = child->key;
        }
        else
        {
            dirs.push_back(child);
        }
    }
    for(auto dir : dirs)
    {
        files[file_pos++] = dir->key;
        sort(dir->children);
    }
}

void FS::allocator(Node *&n)
{
    // for(auto child : n->children)
    // {
    //     child = new Node();
    //     allocator(child);
    // }
}

void FS::garbage_collector(Node *&n)
{
    // for(auto child : n->children)
    // {
    //     garbage_collector(child);
    // }

    delete n;
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
    // for(int i = 0; i < root->children.size(); i++)
    // {
    //     // if(strcmp(root->children[i]->key.file_name, path.c_str()) == 0)
    //     // {
    //     //     return i;
    //     // }
    // }
    
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
    disk.write(1, (uint8_t*)fat);

    // Root block
    garbage_collector(root);
    root = new Node();
    //strcpy(root->cwd, "/");
    dir_entry root_dir;
    root_dir.size = 0;
    root_dir.type = 1;
    root_dir.access_rights = READ;
    root->key = root_dir;
    disk.write(0, (uint8_t*)root);

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
    // else
    // {
    //     if(no_files < MAX_NO_FILES) // TODO: Fix no_files
    //     {
    //         int16_t pos = 2;
    //         std::string data = "";

    //         // Create entry
    //         dir_entry file;
    //         std::getline(std::cin, data);
    //         file.size = data.size();
    //         std::strcpy(file.file_name, filepath.c_str());
    //         file.type = TYPE_FILE;
    //         file.access_rights = WRITE;
    //         find_free(pos);
    //         file.first_blk = pos;
    //         disk.write(file.first_blk, (uint8_t*)data.substr(0, BLOCK_SIZE).c_str());
    //         Node *file_node;
    //         file_node->key = file;
    //         file_node->cwd = root->cwd + "/" + filepath;
    //         root->children.push_back(file_node);

    //         if(file.size > BLOCK_SIZE && file.size < BLOCK_SIZE * (MAX_NO_FILES - no_files - 1))
    //         {
    //             unsigned int res = file.size / BLOCK_SIZE - 1;

    //             if(file.size % BLOCK_SIZE != 0)
    //             {
    //                 res++;            
    //             }

    //             unsigned int next = 2;

    //             for(res; res > 0; res--)
    //             {
    //                 int16_t prev_pos  = pos;
    //                 find_free(++pos);
    //                 fat[prev_pos] = pos;
    //                 disk.write(pos, (uint8_t*)data.substr(0, BLOCK_SIZE * next++).c_str());
    //             }
    //         }

    //         fat[pos] = FAT_EOF;
    //         disk.write(1, (uint8_t*)fat);
    //     }
    //     else
    //     {
    //         std::cout << "Disk has no more room for additional data" << std::endl;
    //     }
    // } 

    return 0;
}

// cat <filepath> reads the content of a file and prints it on the screen
int
FS::cat(std::string filepath)
{
    std::cout << "FS::cat(" << filepath << ")\n";
    // char buffer[BLOCK_SIZE] = {""};
    // unsigned int i = find_entry(filepath);

    // if(root->children[i]->key.type)
    // {
    //     std::cout << "cat: " << root->children[i]->key.file_name << ": Is a directory" << std::endl;
    // }
    // else
    // {
    //     if(i + 1)
    //     {
    //         int16_t blk_no = files[i].first_blk;
    //         while(fat[blk_no] != FAT_EOF)
    //         {
    //             disk.read(blk_no, (uint8_t*)buffer);
    //             std::cout << buffer;
    //             blk_no = fat[blk_no];
    //         }
    //         disk.read(blk_no, (uint8_t*)buffer);
    //         std::cout << buffer << std::endl;
    //     }
    //     else
    //     {
    //         std::cout << "cat: " << filepath << ": No such file or directory" << std::endl;
    //     }
    // }

    return 0;
}

// ls lists the content in the currect directory (files and sub-directories)
int
FS::ls()
{
    std::cout << "FS::ls()\n";

    // std::cout << "name\ttype\tsize" << std::endl;

    // std::string type = "dir";
    // std::string size = "-";
    // for(auto child : root->children)
    // {
    //     if(!child->key.type)
    //     {
    //         type = "file";
    //         size = std::to_string(child->key.size);
    //     }

    //     std::cout << child->key.file_name << "\t" << type << "\t" << size << std::endl;
    // }

    return 0;
}

// cp <sourcepath> <destpath> makes an exact copy of the file
// <sourcepath> to a new file <destpath>
int
FS::cp(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::cp(" << sourcepath << "," << destpath << ")\n";

    // int src_pos = find_entry(sourcepath);
    // int dest_pos = find_file(destpath);
    
    // if(dest_pos + 1)
    // {
    //     if(file_pos < MAX_NO_FILES && files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
    //     {
    //         files[dest_pos].size = files[src_pos].size;
    //         char buffer[BLOCK_SIZE] = {""};
    //         int16_t src_blk_no = files[src_pos].first_blk;
    //         int16_t dest_blk_no = files[dest_pos].first_blk;

    //         while(fat[src_blk_no] != FAT_EOF)
    //         {
    //             disk.read(src_blk_no, (uint8_t*)buffer);
    //             disk.write(dest_blk_no, (uint8_t*)buffer);
    //             src_blk_no = fat[src_blk_no];

    //             if(fat[dest_blk_no] == FAT_EOF)
    //             {
    //                 int16_t prev_pos = dest_blk_no;
    //                 find_free(++dest_blk_no);
    //                 fat[prev_pos] = dest_blk_no;
    //                 fat[dest_blk_no] = FAT_EOF;
    //             }
    //             else
    //             {
    //                 dest_blk_no = fat[dest_blk_no];
    //             }
    //         }
    //         disk.read(src_blk_no, (uint8_t*)buffer);
    //         disk.write(dest_blk_no, (uint8_t*)buffer);

    //         disk.write(0, (uint8_t*)files);
    //         disk.write(1, (uint8_t*)fat);
    //     }
    //     else
    //     {
    //         std::cout << "Disk has no more room for additional data" << std::endl;
    //     }
    // }
    // else if(find_file(sourcepath) == -1)
    // {
    //     std::cout << "cp: cannot find '" << sourcepath << "': No such file or directory" << std::endl;
    // }
    // else
    // {
    //     if(file_pos < MAX_NO_FILES && files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
    //     {
    //         dir_entry file;
    //         std::strcpy(file.file_name, destpath.c_str());
    //         file.size = files[src_pos].size;
    //         file.type = TYPE_FILE;
    //         file.access_rights = WRITE;
    //         int16_t pos = 2;
    //         find_free(pos);
    //         file.first_blk = pos;
    //         files[file_pos++] = file;
    //         fat[file.first_blk] = FAT_EOF;

    //         char buffer[BLOCK_SIZE] = {""};
    //         int16_t src_blk_no = files[src_pos].first_blk;
    //         int16_t dest_blk_no = file.first_blk;

    //         while(fat[src_blk_no] != FAT_EOF)
    //         {
    //             disk.read(src_blk_no, (uint8_t*)buffer);
    //             disk.write(dest_blk_no, (uint8_t*)buffer);
    //             src_blk_no = fat[src_blk_no];

    //             if(fat[dest_blk_no] == FAT_EOF)
    //             {
    //                 std::cout << "EOF" << std::endl;
    //                 int16_t prev_pos = pos;
    //                 find_free(++dest_blk_no);
    //                 fat[prev_pos] = dest_blk_no;
    //                 fat[dest_blk_no] = FAT_EOF;
    //             }
    //             else
    //             {
    //                 dest_blk_no = fat[dest_blk_no];
    //             }
    //         }
    //         disk.read(src_blk_no, (uint8_t*)buffer);
    //         disk.write(dest_blk_no, (uint8_t*)buffer);

    //         disk.write(0, (uint8_t*)files);
    //         disk.write(1, (uint8_t*)fat);
    //     }
    //     else
    //     {
    //         std::cout << "Disk has no more room for additional data" << std::endl;
    //     }
    // }

    return 0;
}

// mv <sourcepath> <destpath> renames the file <sourcepath> to the name <destpath>,
// or moves the file <sourcepath> to the directory <destpath> (if dest is a directory)
int
FS::mv(std::string sourcepath, std::string destpath)
{
    std::cout << "FS::mv(" << sourcepath << "," << destpath << ")\n";

    // int pos = find_file(sourcepath);

    // if(pos + 1)
    // {
    //     strcpy(files[pos].file_name, destpath.c_str());
    //     disk.write(0, (uint8_t*)files);
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
    std::cout << "FS::rm(" << filepath << ")\n";

    // int pos = find_file(filepath);
    // char buffer[1] = "";

    // if(pos + 1)
    // {
    //     int16_t blk_no = files[pos].first_blk;

        
    //     for (; pos < file_pos; pos++)
    //     {
    //         files[pos] = files[pos + 1];
    //     }
        
    //     file_pos--;

    //     while(fat[blk_no] != FAT_EOF)
    //     {
    //         disk.write(blk_no, (uint8_t*)buffer);
    //         blk_no = fat[blk_no];
    //         fat[blk_no] = FAT_FREE;
    //     }
    //     disk.write(blk_no, (uint8_t*)buffer);
    //     fat[blk_no] = FAT_FREE;

    //     disk.write(0, (uint8_t*)files);
    //     disk.write(1, (uint8_t*)fat);
    // }
    // else
    // {
    //     std::cout << "rm: cannot remove '" << filepath << "': No such file or directory" << std::endl;
    // }

    return 0;
}

// append <filepath1> <filepath2> appends the contents of file <filepath1> to
// the end of file <filepath2>. The file <filepath1> is unchanged.
int
FS::append(std::string filepath1, std::string filepath2)
{
    std::cout << "FS::append(" << filepath1 << "," << filepath2 << ")\n";

    // int src_pos = find_file(filepath1);
    // int dest_pos = find_file(filepath2);

    // if(src_pos == -1)
    // {
    //     std::cout << "append: cannot find '" << filepath1 << "': No such file or directory" << std::endl;
    // }
    // else if(dest_pos == -1)
    // {
    //     std::cout << "append: cannot find '" << filepath2 << "': No such file or directory" << std::endl;
    // }
    // else if(filepath1 == filepath2)
    // {
    //     std::cout << "\"You do not need to cover the special case when appending a file to itself, i.e., ’append <filename1> <filename1>’.\"" << std::endl << std::endl << "- Håkan Grahn" << std::endl;
    // }
    // else
    // {
    //     if(file_pos < MAX_NO_FILES && files[src_pos].size < BLOCK_SIZE * (MAX_NO_FILES - file_pos - 1))
    //     {
    //         char buffer[BLOCK_SIZE] = {""};
    //         int16_t dest_blk_no = files[dest_pos].first_blk;

    //         while(fat[dest_blk_no] != FAT_EOF)
    //         {
    //             dest_blk_no = fat[dest_blk_no];
    //         }

    //         int16_t src_blk_no = files[find_file(filepath1)].first_blk;
    //         int16_t prev_pos = dest_blk_no;
    //         find_free(++dest_blk_no);
    //         fat[prev_pos] = dest_blk_no;

    //         while(fat[src_blk_no] != FAT_EOF)
    //         {
    //             disk.read(src_blk_no, (uint8_t*)buffer);
    //             disk.write(dest_blk_no, (uint8_t*)buffer);
    //             files[dest_pos].size += strlen(buffer);
    //             src_blk_no = fat[src_blk_no];
    //             int16_t prev_pos = dest_blk_no;
    //             find_free(++dest_blk_no);
    //             fat[prev_pos] = dest_blk_no;
    //         }

    //         disk.read(src_blk_no, (uint8_t*)buffer);
    //         disk.write(dest_blk_no, (uint8_t*)buffer);
    //         files[dest_pos].size += strlen(buffer);
    //         fat[dest_blk_no] = FAT_EOF;
    //         disk.write(0, (uint8_t*)files);
    //         disk.write(1, (uint8_t*)fat);
    //     }
    //     else
    //     {
    //         std::cout << "Disk has no more room for additional data" << std::endl;
    //     }
    // }
    
    return 0;
}

// mkdir <dirpath> creates a new sub-directory with the name <dirpath>
// in the current directory
int
FS::mkdir(std::string dirpath)
{
    std::cout << "FS::mkdir(" << dirpath << ")\n";

    dir_entry sub_dir;
    int16_t pos = 2;

    // Add name
    strcpy(sub_dir.file_name, dirpath.c_str());

    sub_dir.size = 0;

    // Find available first block
    find_free(pos); // TODO: Add support for multiple directories in one block
    sub_dir.first_blk = pos;
    fat[pos] = FAT_EOF;

    sub_dir.type = 1;
    sub_dir.access_rights = READ;

    Node* dir_node;
    // dir_node->key = sub_dir;
    // // std::string dir_cwd = root->cwd;
    // // dir_cwd += "/" + dirpath;
    // // strcpy(dir_node->cwd, dir_cwd.c_str());
    // Node *parent;
    // parent = root;
    // strcpy(parent->key.file_name, "..");
    // dir_node->children.push_back(parent);
    // root->children.push_back(dir_node);

    disk.write(sub_dir.first_blk, (uint8_t*)root);

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
        // if(root->children[pos]->key.type)
        // {
        //     unsigned int i;
            
        //     for(auto child : root->children)
        //     {
        //         if(strcmp(root->key.file_name, dirpath.c_str()) == 0)
        //         {
        //             root = child;

        //             return 0;
        //         }
        //     }
        // }
        // else
        // {
        //     std::cout << "cd: " << dirpath << ": Not a directory" << std::endl;
        // }
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

    std::cout << root->cwd << std::endl;

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
