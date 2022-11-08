#include <cstring>
#include <iostream>
#include <vector>

#define ERR_NO_FILE "No Such File!\n"
#define CANNOT_OPEN "Cannot Open Such File!\n"
#define PARAM_WRONG "Parameter Cannot Be Passed!\n"
#define COMMAND_ERR "Cannot Recognize Such Command!\n"

using namespace std;

typedef unsigned char uint8;      //1字节
typedef unsigned short uint16;    //2字节
typedef unsigned int uint32;      //4字节

//---------------------以下是FAT12 全局变量---------------------------
int BytsPerSec;			//每扇区字节数
int SecPerClus;			//每簇扇区数
int RsvdSecCnt;			//引导扇区数
int NumFATs;			//FAT表个数
int RootEntCnt;			//根目录最大文件数
int FATSz;				//FAT扇区数

//FAT12的偏移字节
int fatBase;            //FAT表偏移
int fileRootBase;       //根目录区偏移
int dataBase;           //数据区偏移
int BytsPerClus;        //每簇字节数



//---------------------以下是工具函数---------------------------
char* stringToChar(const string& str){
    char *strs = new char[str.length() + 1];
    strcpy(strs, str.c_str());
    return strs;
}

vector<string> split(const string& str, const string& delim) {
    vector <string> res;
    if ("" == str) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char *strs = stringToChar(str);
    char *d = stringToChar(delim);
    char *path = strtok(strs, d);
    while (path) {
        string s = path;           //分割得到的字符串转换为string类型
        res.push_back(s);       //存入结果数组
        path = strtok(NULL, d); 
    }
    return res;
}

bool isL(string &s) {
    return s[0]=='-' && s[1]=='l'; 
}

void formatPath(string &s) {
    if (s[0] != '/') {
        s = "/" + s;
    }
}

extern "C" {
void asm_print(const char *, const int);
}

void myPrint(const char *s) {
    asm_print(s, strlen(s));
}



//---------------------以下是Node节点类---------------------------
class Node{
    string name;            //名字
    vector<Node*> next;     //下一级目录的Node数组
    Node* father;           //父节点
    string path;            //记录path，便于打印操作
    uint32 fileSize;        //文件大小
    bool isFile = false;    //是文件还是目录
    bool isVal = true;      //用于标记.和..（true表示文件或者文件夹，false表示.和..）
    int dir_count = 0;      //记录下一级有多少目录
    int file_count = 0;     //记录下一级有多少文件
    char *content = new char[10000];    //存放文件内容
public:
    Node() = default;

    Node(string name, bool isVal);

    Node(string name, string path);

    Node(string name, uint32 fileSize, bool isFile, string path);

    void setPath(string path) {
        this->path = path;
    }

    void setName(string n) {
        this->name = n;
    }
    void addFather(Node *father);

    void addChild(Node *child);

    void addFileChild(Node *child);

    void addDirChild(Node *child);

    string getName() { return name; }

    string getPath() { return this->path; }

    char* getContent() { return content; }

    bool getIsFile() { return isFile; }

    Node* getFather() {return father; };

    vector<Node*> getNext() { return next; }

    bool getIsVal() { return isVal; }

    uint32 getFileSize() { return fileSize; }

    int getDirCount() { return dir_count; }

    int getFileCount() { return file_count; };
};

Node::Node(string name, string path) {
    this->name = name;
    this->path = path;
}

Node::Node(string name, bool isVal) {
    this->name = name;
    this->isVal = isVal;
}

Node::Node(string name, uint32 fileSize, bool isFile, string path) {
    this->name = name;
    this->fileSize = fileSize;
    this->isFile = isFile;
    this->path = path;
}

void Node::addFather(Node* father){
    this->father = father;
}

void Node::addChild(Node *child) {
    this->next.push_back(child);
}

void Node::addFileChild(Node *child) {
    this->next.push_back(child);
    this->file_count++;
}

void Node::addDirChild(Node *child) {
    child->addChild(new Node(".", false));
    child->addChild(new Node("..", false));
    this->next.push_back(child);
    this->dir_count++;
}

//---------------------以下是FAT12类---------------------------
#pragma pack(1) /*指定按1字节对齐*/

//引导扇区的BPB，25字节
class BPB{
    uint16 BPB_BytsPerSec;    //每扇区字节数
    uint8 BPB_SecPerClus;     //每簇扇区数
    uint16 BPB_RsvdSecCnt;    //Boot记录占用的扇区数
    uint8 BPB_NumFATs;		//FAT表个数
    uint16 BPB_RootEntCnt;    //根目录最大文件数
    uint16 BPB_TotSec16;      //扇区总数
    uint8 BPB_Media;          //介质描述符
    uint16 BPB_FATSz16;       //FAT扇区数
    uint16 BPB_SecPerTrk;     //每磁道扇区数（Sector/track）
    uint16 BPB_NumHeads;      //磁头数（面数）
    uint32 BPB_HiddSec;       //隐藏扇区数
    uint32 BPB_TotSec32;      //如果BPB_FATSz16为0，该值为FAT扇区数
public:
    BPB() {};

    void init(FILE *fat12);
};

void BPB::init(FILE *fat12){
    fseek(fat12, 11, SEEK_SET); //BPB从偏移11个字节处开始
    fread(this, 1, 25, fat12);

    //初始化各个全局变量
    BytsPerSec = this->BPB_BytsPerSec; 
    SecPerClus = this->BPB_SecPerClus;
    RsvdSecCnt = this->BPB_RsvdSecCnt;
    NumFATs = this->BPB_NumFATs;
    RootEntCnt = this->BPB_RootEntCnt;

    if (this->BPB_FATSz16 != 0){
        FATSz = this->BPB_FATSz16;
    }
    else{
        FATSz = this->BPB_TotSec32;
    }

    fatBase = RsvdSecCnt * BytsPerSec;
    fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
    dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    BytsPerClus = SecPerClus * BytsPerSec;
}

//根目录条目
class RootEntry
{
    char DIR_Name[11];          //文件名8字节，扩展名3字节
    uint8 DIR_Attr;             //文件属性
    char reserved[10];          //保留位
    uint16 DIR_WrtTime;         //文件最后一次写入时间
    uint16 DIR_WrtDate;         //文件最后一次写入日期
    uint16 DIR_FstClus;         //此条目对应的开始簇数
    uint32 DIR_FileSize;        //文件大小
public:
    RootEntry() {};

    void initRootEntry(FILE *fat12, Node *root);

    bool isValidNameAt(int i);

    bool isEmptyName();

    bool isInvalidName();

    bool isFile();

    void generateFileName(char name[12]);

    void generateDirName(char name[12]);

    uint32 getFileSize();

    uint16 getFstClus() { return DIR_FstClus; }
};

/*
* 获取FAT表的值
*/
int getFATValue(FILE *fat12, int num) {
    int base = RsvdSecCnt * BytsPerSec; //FAT表的偏移量
    int pos = base + num * 3 / 2;       //该项的偏移量
    int type = num % 2;

    uint16 bytes;
    uint16 *bytesPtr = &bytes;
    fseek(fat12, pos, SEEK_SET);
    fread(bytesPtr, 1, 2, fat12);

    if (type == 0) {
        bytes = bytes << 4;
    }
    return bytes >> 4;
}

/*
* 查FAT表，从数据区获取内容
*/
void RetrieveContent(FILE *fat12, int startClus, Node *child) {
    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);   //数据区偏移量
    int currentClus = startClus;    
    int value = 0;
    char *pointer = child->getContent();

    if (startClus == 0) return;

    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("File Broken!\n");
            break;
        }

        int size = SecPerClus * BytsPerSec;     //每簇字节数
        char *str = (char*)malloc(size);
        char *content = str;
        int startByte = base + (currentClus - 2)*SecPerClus*BytsPerSec;   //该簇的起始字节，这里-2是因为数据区第一个簇号是2

        fseek(fat12, startByte, SEEK_SET);
        fread(content, 1, size, fat12);

        for (int i = 0; i < size; ++i) {
            *pointer = content[i];
            pointer++;
        }
        free(str);
        currentClus = value;
    }
}

/*
* 递归查找子节点
*/
void readChildren(FILE *fat12, int startClus, Node *root) {

    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);   //数据区偏移量
    int currentClus = startClus;
    int value = 0;
    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("File Broken!\n");
            break;
        }

        int startByte = base + (currentClus - 2) * SecPerClus * BytsPerSec;     //该簇的起始字节，这里-2是因为数据区第一个簇号是2
        int size = SecPerClus * BytsPerSec;     //每簇字节数
        int loop = 0;
        while (loop < size) {
            //数据区每项开头的内容和 RootEntry 相同
            RootEntry *rootEntry = new RootEntry();
            fseek(fat12, startByte + loop, SEEK_SET);
            fread(rootEntry, 1, 32, fat12);

            loop += 32;
            
            if (rootEntry->isEmptyName() || rootEntry->isInvalidName()) {
                continue;
            }

            char tmpName[12];
            if ((rootEntry->isFile())) {
                rootEntry->generateFileName(tmpName);
                Node *child = new Node(tmpName, rootEntry->getFileSize(), true, root->getPath());
                root->addFileChild(child);
                child->addFather(root);
                RetrieveContent(fat12, rootEntry->getFstClus(), child);
            } else {
                rootEntry->generateDirName(tmpName);
                Node *child = new Node();
                child->setName(tmpName);
                child->setPath(root->getPath() + tmpName + "/");
                root->addDirChild(child);
                child->addFather(root);
                readChildren(fat12, rootEntry->getFstClus(), child);
            }
        }
    }
}

void RootEntry::initRootEntry(FILE *fat12, Node *root) {
    int base = fileRootBase;
    char realName[12];

    for (int i = 0; i < RootEntCnt; ++i) {
        fseek(fat12, base, SEEK_SET);
        fread(this, 1, 32, fat12);

        base += 32;

        if (isEmptyName() || isInvalidName()) continue;

        if (this->isFile()) {
            generateFileName(realName);
            Node *child = new Node(realName, this->DIR_FileSize, true, root->getPath());
            root->addFileChild(child);
            child->addFather(root);
            RetrieveContent(fat12, this->DIR_FstClus, child);
        } else {
            generateDirName(realName);
            Node *child = new Node();
            child->setName(realName);
            child->setPath(root->getPath() + realName + "/");
            root->addDirChild(child);
            child->addFather(root);
            readChildren(fat12, this->getFstClus(), child);
        }
    }
}

bool RootEntry::isValidNameAt(int i) {
    return ((this->DIR_Name[i] >= 'a') && (this->DIR_Name[i] <= 'z'))
            ||((this->DIR_Name[i] >= 'A') && (this->DIR_Name[i] <= 'Z'))
            ||((this->DIR_Name[i] >= '0') && (this->DIR_Name[i] <= '9'))
            ||((this->DIR_Name[i] == ' '));
}

bool RootEntry::isEmptyName() {
    return this->DIR_Name[0] == '\0';
}

bool RootEntry::isInvalidName() {
    int invalid = false;
    for (int k = 0; k < 11; ++k) {
        if (!this->isValidNameAt(k)) {
            invalid = true;
            break;
        }
    }
    return invalid;
}

bool RootEntry::isFile() {
    return (this->DIR_Attr & 0x10) == 0;
}

void RootEntry::generateFileName(char name[12]) {
    int tmp = -1;
    for (int j = 0; j < 11; ++j) {
        if (this->DIR_Name[j] != ' ') {
            name[++tmp] = this->DIR_Name[j];
        } else {
            name[++tmp] = '.';
            while (this->DIR_Name[j] == ' ') j++;
            j--;
        }
    }
    ++tmp;
    name[tmp] = '\0';
}

void RootEntry::generateDirName(char *name) {
    int tmp = -1;
    for (int k = 0; k < 11; ++k) {
        if (this->DIR_Name[k] != ' ') {
            name[++tmp] = this->DIR_Name[k];
        } else {
            tmp++;
            name[tmp] = '\0';
            break;
        }
    }
}

uint32 RootEntry::getFileSize() {
    return DIR_FileSize;
}



//---------------------以下是解析并完成指令的函数---------------------------

Node* findByName(Node *root, vector<string> dirs) {
    if(dirs.empty()) return root;
    string name = dirs[dirs.size()-1];
    if(name == ".."){
        dirs.pop_back();
        return findByName(root->getFather(), dirs);
    }else if(name == "."){
        dirs.pop_back();
        return findByName(root, dirs);
    }else{
        for(int i = 0; i < root->getNext().size(); i++){
            if(name == root->getNext()[i]->getName()){
                dirs.pop_back();
                return findByName(root->getNext()[i], dirs);
            }
        }
    }
    return nullptr;
}

//不是目录则返回空指针
Node* isDir(string &s, Node *root) {
    vector<string> tmp = split(s, "/");
    vector<string> dirs;
    for(int i = tmp.size(); i > 0; i--){
        dirs.push_back(tmp[i-1]);
    }
    return findByName(root, dirs);
}

void outputCat(Node *root, string path, int &exist) {
    formatPath(path);
    //找到匹配文件
    if (path == root->getPath() + root->getName()) {
        if (root->getIsFile()) {
            exist = 1;
            if (root->getContent()[0] != 0) {
                myPrint(root->getContent());
                myPrint("\n");
            }
            return;
        } else {
            exist = 2;
            return;
        }
    }
    //当前目录下匹配失败
    if (path.length() <= root->getPath().length()) {
        return;
    }
    //在子目录和文件中匹配
    string tmp = path.substr(0, root->getPath().length());
    if (tmp == root->getPath()) {
        for (Node *q : root->getNext()) {
            outputCat(q, path, exist);
        }
    }
}

void handleCat(vector<string> commands, Node* root) {
    if (commands.size() == 2 && commands[1][0] != '-') {
        int exist = 0;
        outputCat(root, commands[1], exist);
        if (exist == 0) {
            myPrint(ERR_NO_FILE);
        } else if (exist == 2) {
            myPrint(CANNOT_OPEN);
        }
    } else {
        myPrint(PARAM_WRONG);
    }
}

void outputLS(Node *r) {
    //如果当前节点是文件，则不输出
    if(r->getIsFile()){
        return;
    }

    //输出当前文件夹及直接子文件夹、直接子文件
    string str = r->getPath() + ":\n";
    myPrint(str.c_str());

    Node *next;
    int size = r->getNext().size();
    for(int i = 0; i < size; i++){
        next = r->getNext()[i];
        if(!next->getIsFile()){
            string out = "\033[31m" + next->getName() + "\033[0m" + "  ";
            myPrint(out.c_str());
        }else{
            myPrint(next->getName().c_str());
            myPrint(" ");
        }
    }
    myPrint("\n");

    //递归输出子文件中的内容
    for(int i = 0; i < size; i++){
        if(r->getNext()[i]->getIsVal()){
            outputLS(r->getNext()[i]);
        }
    }
}

void outputLSL(Node *r) {
    //如果当前节点是文件，则不输出
    if(r->getIsFile()){
        return;
    }

    //输出当前文件夹及直接子文件夹、直接子文件
    int dirNum = r->getDirCount();
    int fileNum = r->getFileCount();
    string str = r->getPath() + " " + to_string(dirNum) + " " + to_string(fileNum) + "\n";
    myPrint(str.c_str());

    Node *next;
    int size = r->getNext().size();
    for(int i = 0; i < size; i++){
        next = r->getNext()[i];
        if(!next->getIsFile()){  //如果是文件夹
            if(next->getName() == "." || next->getName() == ".."){
                str = "\033[31m" + next->getName() + "\033[0m \n";
                myPrint(str.c_str());
            }else{
                dirNum = next->getDirCount();
                fileNum = next->getFileCount();
                str = "\033[31m" + next->getName() + "\033[0m " + to_string(dirNum) + " " +to_string(fileNum) + "\n";
                myPrint(str.c_str());
            }
        }else{              //如果是文件
            str = next->getName() + " " + to_string(next->getFileSize()) + "\n";
            myPrint(str.c_str());
        }
    }
    myPrint("\n");

    //递归输出子文件中的内容
    for(int i = 0; i < size; i++){
        if(r->getNext()[i]->getIsVal()){
            outputLSL(r->getNext()[i]);
        }
    }
}   

void handleLS(vector<string> commands, Node* root) {
    if(commands.size() == 1){
        outputLS(root);
        return;
    }
    bool hasL = false;
    bool hasDir = false;
    Node *toFind = root;

    //处理-l以及目录的问题
    for(int i=1; i<commands.size(); i++){
        Node *newRoot = isDir(commands[i], root);
        if(isL(commands[i])){
            hasL = true;
        }else if(!hasDir && newRoot != nullptr){
            hasDir = true;
            toFind = newRoot;
        }else{
            myPrint(PARAM_WRONG);
            return;
        }
    }
    
    //根据是否有-l调用不同的输出
    if(hasL){
        outputLSL(toFind);
    }else{
        outputLS(toFind);
    }
}

int main()
{
    //打开FAT12的映像文件
    FILE *fat12;
    fat12 = fopen("./a.img", "rb"); 
    //初始化BPB, RootEntry, Node节点
    BPB *bpb = new BPB();
    bpb->init(fat12);
    Node *root = new Node();
    root->addFather(root);
    root->setName("");
    root->setPath("/");
    RootEntry *rootEntry = new RootEntry();
    rootEntry->initRootEntry(fat12, root);
    //解析并执行命令
    while (true) {
        myPrint(">");
        string input;
        getline(cin, input);
        vector<string> commands = split(input, " ");
        if (commands[0] == "exit") {
            myPrint("Bye!\n");
            fclose(fat12);
            break;
        } else if (commands[0] == "cat") {
            handleCat(commands, root);
        } else if (commands[0] == "ls") {
            handleLS(commands, root);
        } else {
            myPrint(COMMAND_ERR);
            continue;
        }
    }

}