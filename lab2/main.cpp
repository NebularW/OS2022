#include <cstring>
#include <iostream>
#include <vector>

#define ERR_NO_FILE "No Such File!\n"
#define CANNOT_OPEN "Cannot Open Such File!\n"
#define PARAM_WRONG "Parameter Cannot Be Passed!\n"
#define COMMAND_ERR "Cannot Recognize Such Command!\n"

using namespace std;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

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
    char *p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(NULL, d);
    }
    return res;
}

int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int NumFATs;				//FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSz;					//FAT扇区数

extern "C" {
void asm_print(const char *, const int);
}

void myPrint(const char *s) {
    asm_print(s, strlen(s));
}

//FAT1的偏移字节
int fatBase;
int fileRootBase;
int dataBase;
int BytsPerClus;
#pragma pack(1) /*指定按1字节对齐*/

class Node {
    string name;
    vector<Node*> next;
    string path;
    uint32_t FileSize;
    bool isFile = false;
    bool isVal = true;
    int dir_count = 0;
    int file_count = 0;
    char *content = new char[10000];
public:
    Node() = default;

    Node(string name, bool isVal);

    Node(string name, string path);

    Node(string name, uint32_t fileSize, bool isFile, string path);

    void setPath(string p) {
        this->path = p;
    }

    void setName(string n) {
        this->name = n;
    }

    void addChild(Node *child);

    void addFileChild(Node *child);

    void addDirChild(Node *child);

    string getName() { return name; }

    string getPath() { return this->path; }

    char* getContent() { return content; }

    bool getIsFile() { return isFile;}

    vector<Node*> getNext() { return next; }

    bool getIsVal() { return isVal; }

    uint32_t getFileSize() { return FileSize; }
};

Node::Node(string name, string path) {
    this->name = name;
    this->path = path;
}

Node::Node(string name, bool isVal) {
    this->name = name;
    this->isVal = isVal;
}

void Node::addFileChild(Node *child) {
    this->next.push_back(child);
    this->file_count++;
}

Node::Node(string name, uint32_t fileSize, bool isFile, string path) {
    this->name = name;
    this->FileSize = fileSize;
    this->isFile = isFile;
    this->path = path;
}

void Node::addDirChild(Node *child) {
    child->addChild(new Node(".", false));
    child->addChild(new Node("..", false));
    this->next.push_back(child);
    this->dir_count++;
}

void Node::addChild(Node *child) {
    this->next.push_back(child);
}

class BPB
{
    uint16_t BPB_BytsPerSec; //每扇区字节数
    uint8_t BPB_SecPerClus;  //每簇扇区数
    uint16_t BPB_RsvdSecCnt; //Boot记录占用的扇区数
    uint8_t BPB_NumFATs;		//FAT表个数
    uint16_t BPB_RootEntCnt; //根目录最大文件数
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16; //FAT扇区数
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32; //如果BPB_FATSz16为0，该值为FAT扇区数
public:
    BPB() {};

    void init(FILE *fat12);
};

void BPB::init(FILE *fat12) {
    fseek(fat12, 11, SEEK_SET);   //BPB从偏移11个字节处开始
    fread(this, 1, 25, fat12); //BPB长度为25字节

    BytsPerSec = this->BPB_BytsPerSec; //初始化各个全局变量
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
    fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数=boot+fat1&2的总字节数
    dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    BytsPerClus = SecPerClus * BytsPerSec; //每簇的字节数
}

//BPB至此结束，长度25字节
//根目录条目
class RootEntry
{
    char DIR_Name[11];
    uint8_t DIR_Attr; //文件属性
    char reserved[10];
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClus; //开始簇号
    uint32_t DIR_FileSize;
public:
    RootEntry() {};

    void initRootEntry(FILE *fat12, Node *root);

    bool isValidNameAt(int i);

    bool isEmptyName();

    bool isInvalidName();

    bool isFile();

    void generateFileName(char name[12]);

    void generateDirName(char name[12]);

    uint32_t getFileSize();

    uint16_t getFstClus() { return DIR_FstClus; }
};

int getFATValue(FILE *fat12, int num) {
    int base = RsvdSecCnt * BytsPerSec;
    int pos = base + num * 3 / 2;
    int type = num % 2;

    uint16_t bytes;
    uint16_t *bytesPtr = &bytes;
    fseek(fat12, pos, SEEK_SET);
    fread(bytesPtr, 1, 2, fat12);

    if (type == 0) {
        bytes = bytes << 4;
    }
    return bytes >> 4;
}

void RetrieveContent(FILE *fat12, int startClus, Node *child) {
    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int currentClus = startClus;
    int value = 0;
    char *pointer = child->getContent();

    if (startClus == 0) return;

    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("坏啦！");
            break;
        }

        int size = SecPerClus * BytsPerSec;
        char *str = (char*)malloc(size);
        char *content = str;
        int startByte = base + (currentClus - 2)*SecPerClus*BytsPerSec;

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


void readChildren(FILE *fat12, int startClus, Node *root) {

    int base = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

    int currentClus = startClus;
    int value = 0;
    while (value < 0xFF8) {
        value = getFATValue(fat12, currentClus);
        if (value == 0xFF7) {
            myPrint("坏啦！");
            break;
        }

        int startByte = base + (currentClus - 2) * SecPerClus * BytsPerSec;

        int size = SecPerClus * BytsPerSec;
        int loop = 0;
        while (loop < size) {
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
                RetrieveContent(fat12, rootEntry->getFstClus(), child);
            } else {
                rootEntry->generateDirName(tmpName);
                Node *child = new Node();
                child->setName(tmpName);
                child->setPath(root->getPath() + tmpName + "/");
                root->addDirChild(child);
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
            RetrieveContent(fat12, this->DIR_FstClus, child);
        } else {
            generateDirName(realName);
            Node *child = new Node();
            child->setName(realName);
            child->setPath(root->getPath() + realName + "/");
            root->addDirChild(child);
            readChildren(fat12, this->getFstClus(), child);
        }
    }
}

bool RootEntry::isValidNameAt(int j) {
    return ((this->DIR_Name[j] >= 'a') && (this->DIR_Name[j] <= 'z'))
           ||((this->DIR_Name[j] >= 'A') && (this->DIR_Name[j] <= 'Z'))
           ||((this->DIR_Name[j] >= '0') && (this->DIR_Name[j] <= '9'))
           ||((this->DIR_Name[j] == ' '));
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

uint32_t RootEntry::getFileSize() {
    return DIR_FileSize;
}

void formatPath(string &s) {
    if (s[0] != '/') {
        s = "/" + s;
    }
}

void outputCat(Node *root, string p, int &exist) {
    formatPath(p);
    if (p == root->getPath() + root->getName()) {
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
    if (p.length() <= root->getPath().length()) {
        return;
    }
    string tmp = p.substr(0, root->getPath().length());
    if (tmp == root->getPath()) {
        for (Node *q : root->getNext()) {
            outputCat(q, p, exist);
        }
    }
}

void handleCAT(vector<string> commands, Node* root) {
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
    string str;
    Node *p = r;
    if (p->getIsFile()) {
        return;
    }
    else {
        str = p->getPath() + ":\n";
        const char *strPtr = str.c_str();
        myPrint(strPtr);
        str.clear();
        //打印每个next
        Node *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                string out = "\033[31m" + q->getName() + "\033[0m" + "  ";
                myPrint(stringToChar(out));
            } else {
                myPrint(stringToChar(q->getName()));
                myPrint(" ");
            }
        }
        myPrint(stringToChar("\n"));
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                outputLS(p->getNext()[i]);
            }
        }
    }
}

void outputLSL(Node *r) {
    string str;
    Node *p = r;
    if (p->getIsFile()) {
        return;
    }
    else {
        /*
         * 算一下
         */
        int fileNum = 0;
        int dirNum = 0;
        for (int j = 0; j < p->getNext().size(); ++j) {
            if (p->getNext()[j]->getName() == "." || p->getNext()[j]->getName() == "..") {
                continue;
            }
            if (p->getNext()[j]->getIsFile()) {
                fileNum++;
            } else {
                dirNum++;
            }
        }
        
        string out = p->getPath() + " " + to_string(dirNum) + " " + to_string(fileNum) + "\n";
        myPrint(stringToChar(out));
        str.clear();
        //打印每个next
        Node *q;
        int len = p->getNext().size();
        for (int i = 0; i < len; i++) {
            q = p->getNext()[i];
            if (!q->getIsFile()) {
                if (q->getName() == "." || q->getName() == "..") {
                    string out = "\033[31m" + q->getName() + "\033[0m" + " " + "\n";
                    myPrint(stringToChar(out));
                } else {
                    fileNum = 0;
                    dirNum = 0;
                    for (int j = 2; j < q->getNext().size(); ++j) {
                        if (q->getNext()[j]->getIsFile()) {
                            fileNum++;
                        } else {
                            dirNum++;
                        }
                    }
                    string out = "\033[31m" + q->getName() + "\033[0m" + " " + to_string(dirNum) + " " + to_string(fileNum) + "\n";
                    myPrint(stringToChar(out));
                }
            } else {
                string out = q->getName() + " " + to_string(q->getFileSize()) + "\n";
                myPrint(stringToChar(out));
            }
        }
        myPrint(stringToChar("\n"));
        for (int i = 0; i < len; ++i) {
            if (p->getNext()[i]->getIsVal()) {
                outputLSL(p->getNext()[i]);
            }
        }
    }
}

bool isL(string &s) {
    if (s[0] != '-') {
        return false;
    } else {
        for (int i=1; i<s.size(); i++) {
            if (s[i] != 'l') return false;
        }
    }
    return true;
}
Node* findByName(Node *root, vector<string> dirs) {
    if (dirs.empty()) return root;
    string name = dirs[dirs.size()-1];
    for (int i=0; i<root->getNext().size(); i++) {
        if (name == root->getNext()[i]->getName()) {
            dirs.pop_back();
            return findByName(root->getNext()[i], dirs);
        }
    }
    return nullptr;
}

Node* isDir(string &s, Node *root) {
    vector<string> tmp = split(s, "/");
    vector<string> dirs;
    while (!tmp.empty()) {
        dirs.push_back(tmp[tmp.size()-1]);
        tmp.pop_back();
    }
    return findByName(root, dirs);
}

void handleLS(vector<string> commands, Node* root) {
    if (commands.size() == 1) {
        outputLS(root);
    } else {
        // handle -l
        bool hasL = false;
        bool hasDir = false;
        Node *toFind = root;
        for (int i=1; i<commands.size(); i++) {
            Node* newRoot = isDir(commands[i], root);
            if (isL(commands[i])) {
                hasL = true;
            } else if (!hasDir && newRoot != nullptr) {
                hasDir = true;
                toFind = newRoot;
            } else {
                myPrint(PARAM_WRONG);
                return;
            }
        }
        if (hasL) {
            outputLSL(toFind);
        }
    }
}

int main()
{
    FILE *fat12;
    fat12 = fopen("./a.img", "rb"); //打开FAT12的映像文件

    BPB *bpb = new BPB();
    bpb->init(fat12);
    Node *root = new Node();
    root->setName("");
    root->setPath("/");
    RootEntry *rootEntry = new RootEntry();
    rootEntry->initRootEntry(fat12, root);

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
            handleCAT(commands, root);
        } else if (commands[0] == "ls") {
            handleLS(commands, root);
        } else {
            myPrint(COMMAND_ERR);
            continue;
        }
    }

}

