/**
 * @file SearchAlgo.cpp
 * @brief Сравнение времени поиска по названию услуги различными структурами данных.
 * @details Реализованы: линейный поиск, бинарное дерево поиска (BST),
 *          красно-чёрное дерево (RBT), хеш-таблица с цепочками и std::multimap.
 *          Для каждого размера массива (от 100 до 1 000 000) выполняется 1000
 *          случайных запросов, замеряется время поиска (без учёта построения структур).
 *          Для хеш-таблицы подсчитывается реальное число коллизий.
 * @author Kseniia Prokopovich
 * @date 2026-05-14
 */

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <chrono>
#include <map>           

using namespace std;

/**
 * @brief Структура IT-услуги
 */
struct ITService {
    string name;
    double cost;
    int    duration;
    double prepayment;
};

/**
 * @brief Чтение CSV-файла с услугами
 * @param path Путь к входному файлу
 * @return Вектор прочитанных услуг
 * @details Использует fscanf для прямого чтения в поля, заголовок пропускается
 */
vector<ITService> readCSV(const string& path) { ... }
vector<ITService> readCSV(const string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) { printf("Ошибка открытия: %s\n", path.c_str()); exit(1); }

    vector<ITService> data;
    char nameBuf[256];
    ITService s;

    fscanf(f, "%*[^\n]\n");  // пропускаем заголовок
    while (fscanf(f, "%[^,],%lf,%d,%lf\n", nameBuf, &s.cost, &s.duration, &s.prepayment) == 4) {
        s.name = nameBuf;
        data.push_back(s);
    }
    fclose(f);
    return data;
}

/**
 * @brief Линейный поиск всех вхождений ключа
 * @param data Массив услуг
 * @param key Ключ поиска (название)
 * @return Количество найденных записей
 */
int linearSearch(const vector<ITService>& data, const string& key) {
    int count = 0;
    for (const auto& s : data)
        if (s.name == key) ++count;
    return count;
}

/// Узел бинарного дерева поиска
struct BSTNode {
    string key;
    const ITService* value;
    BSTNode *left, *right;
    BSTNode(const string& k, const ITService* v)
        : key(k), value(v), left(nullptr), right(nullptr) {}
};

/**
 * @brief Бинарное дерево поиска
 * @details Хранит указатели на объекты ITService.
 *          Допускает дубликаты ключей.
 */
class BSTree {
    BSTNode* root = nullptr;

    void clearRecursive(BSTNode* node) {
        if (node) {
            clearRecursive(node->left);
            clearRecursive(node->right);
            delete node;
        }
    }

    void searchRecursive(BSTNode* node, const string& key,
                         vector<const ITService*>& out) const {
        if (!node) return;
        if (key < node->key) {
            searchRecursive(node->left, key, out);
        } else if (key > node->key) {
            searchRecursive(node->right, key, out);
        } else {
            out.push_back(node->value);
            // Продолжаем поиск в обоих поддеревьях для сбора дубликатов
            searchRecursive(node->left, key, out);
            searchRecursive(node->right, key, out);
        }
    }

public:
    BSTree() = default;                         
    BSTree(const BSTree&) = delete;             
    BSTree& operator=(const BSTree&) = delete;  

    ~BSTree() { clearRecursive(root); }         

    void insert(const string& key, const ITService* obj) {
        BSTNode** pp = &root;
        while (*pp) {
            if (key < (*pp)->key)
                pp = &(*pp)->left;
            else
                pp = &(*pp)->right;  
        }
        *pp = new BSTNode(key, obj);
    }

    void search(const string& key, vector<const ITService*>& out) const {
        out.clear();
        searchRecursive(root, key, out);
    }
};


enum class Color { RED, BLACK };
/// Узел красно-черного дерева
struct RBNode {
    string key;
    const ITService* value;
    Color color;
    RBNode *left, *right, *parent;
    RBNode(const string& k, const ITService* v)
        : key(k), value(v), color(Color::RED),
          left(nullptr), right(nullptr), parent(nullptr) {}
};

/**
 * @brief Красно-чёрное дерево
 * @details Гарантирует O(log n) высоты. Допускает дубликаты ключей.
 */
class RBTree {
    RBNode* root = nullptr;

    void clearRecursive(RBNode* node) {
        if (node) {
            clearRecursive(node->left);
            clearRecursive(node->right);
            delete node;
        }
    }

    void rotateLeft(RBNode* x) {
        RBNode* y = x->right;
        x->right = y->left;
        if (y->left) y->left->parent = x;
        y->parent = x->parent;
        if (!x->parent) root = y;
        else if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;
        y->left = x;
        x->parent = y;
    }

    void rotateRight(RBNode* x) {
        RBNode* y = x->left;
        x->left = y->right;
        if (y->right) y->right->parent = x;
        y->parent = x->parent;
        if (!x->parent) root = y;
        else if (x == x->parent->right) x->parent->right = y;
        else x->parent->left = y;
        y->right = x;
        x->parent = y;
    }

    void fixInsert(RBNode* z) {
        while (z->parent && z->parent->color == Color::RED) {
            if (z->parent == z->parent->parent->left) {
                RBNode* y = z->parent->parent->right;
                if (y && y->color == Color::RED) {
                    z->parent->color = Color::BLACK;
                    y->color = Color::BLACK;
                    z->parent->parent->color = Color::RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->right) {
                        z = z->parent;
                        rotateLeft(z);
                    }
                    z->parent->color = Color::BLACK;
                    z->parent->parent->color = Color::RED;
                    rotateRight(z->parent->parent);
                }
            } else {
                RBNode* y = z->parent->parent->left;
                if (y && y->color == Color::RED) {
                    z->parent->color = Color::BLACK;
                    y->color = Color::BLACK;
                    z->parent->parent->color = Color::RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->left) {
                        z = z->parent;
                        rotateRight(z);
                    }
                    z->parent->color = Color::BLACK;
                    z->parent->parent->color = Color::RED;
                    rotateLeft(z->parent->parent);
                }
            }
        }
        root->color = Color::BLACK;
    }

    void searchRecursive(RBNode* node, const string& key,
                         vector<const ITService*>& out) const {
        if (!node) return;
        if (key < node->key) {
            searchRecursive(node->left, key, out);
        } else if (key > node->key) {
            searchRecursive(node->right, key, out);
        } else {
            out.push_back(node->value);
            
            searchRecursive(node->left, key, out);
            searchRecursive(node->right, key, out);
        }
    }

public:
    RBTree() = default;                        
    RBTree(const RBTree&) = delete;             
    RBTree& operator=(const RBTree&) = delete;  

    ~RBTree() { clearRecursive(root); }         

    void insert(const string& key, const ITService* obj) {
        RBNode* z = new RBNode(key, obj);
        RBNode* y = nullptr;
        RBNode* x = root;
        while (x) {
            y = x;
            if (key < x->key) x = x->left;
            else x = x->right;  // равные ключи направо
        }
        z->parent = y;
        if (!y) root = z;
        else if (key < y->key) y->left = z;
        else y->right = z;
        fixInsert(z);
    }

    void search(const string& key, vector<const ITService*>& out) const {
        out.clear();
        searchRecursive(root, key, out);
    }
};

/// Узел хэш-таблицы
struct HashNode {
    string key;
    const ITService* value;
    HashNode* next;
    HashNode(const string& k, const ITService* v)
        : key(k), value(v), next(nullptr) {}
};

/**
 * @brief Хеш-таблица с цепочками (коллизии разрешаются списком)
 * @details Использует хеш-функцию djb2. Размер таблицы задаётся при создании.
 */
class HashMap {
    vector<HashNode*> buckets;
    size_t num_elements;

public:
    HashMap(size_t bucketCount)
        : buckets(bucketCount, nullptr), num_elements(0) {}

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    ~HashMap() {
        for (auto head : buckets) {
            HashNode* cur = head;
            while (cur) {
                HashNode* next = cur->next;
                delete cur;
                cur = next;
            }
        }
    }

    
    size_t hash(const string& s) const {
        size_t h = 5381;
        for (char c : s) h = ((h << 5) + h) + c;
        return h;
    }

    void insert(const string& key, const ITService* obj) {
        size_t idx = hash(key) % buckets.size();
        HashNode* node = new HashNode(key, obj);
        node->next = buckets[idx];
        buckets[idx] = node;
        ++num_elements;
    }

    void search(const string& key, vector<const ITService*>& out) const {
        out.clear();
        size_t idx = hash(key) % buckets.size();
        for (HashNode* cur = buckets[idx]; cur; cur = cur->next)
            if (cur->key == key) out.push_back(cur->value);
    }

    size_t countRealCollisions() const {
        size_t collisions = 0;
        for (const auto& bucket : buckets) {
            int len = 0;
            for (HashNode* cur = bucket; cur; cur = cur->next) ++len;
            if (len > 1) collisions += (len - 1);
        }
        return collisions;
    }

    size_t size() const { return num_elements; }
};


using ms = chrono::duration<double, milli>;

/**
 * @brief Замер времени линейного поиска
 * @param data Исходный массив (поиск выполняется на месте)
 * @param queries Список запросов (ключей)
 * @return Время в миллисекундах
 */
double measureLinear(const vector<ITService>& data, const vector<string>& queries) {
    volatile int sink = 0;
    auto t0 = chrono::high_resolution_clock::now();
    for (const auto& q : queries)
        sink += linearSearch(data, q);
    return ms(chrono::high_resolution_clock::now() - t0).count();
}

/**
 * @brief Замер времени бинарного дерева
 * @param data Исходный массив (поиск выполняется на месте)
 * @param queries Список запросов (ключей)
 * @return Время в миллисекундах
 */
double measureBST(const BSTree& tree, const vector<string>& queries) {
    volatile int sink = 0;
    vector<const ITService*> out;
    auto t0 = chrono::high_resolution_clock::now();
    for (const auto& q : queries) {
        tree.search(q, out);
        sink += out.size();
    }
    return ms(chrono::high_resolution_clock::now() - t0).count();
}

/**
 * @brief Замер времени красно-черного дерева
 * @param data Исходный массив (поиск выполняется на месте)
 * @param queries Список запросов (ключей)
 * @return Время в миллисекундах
 */
double measureRBT(const RBTree& tree, const vector<string>& queries) {
    volatile int sink = 0;
    vector<const ITService*> out;
    auto t0 = chrono::high_resolution_clock::now();
    for (const auto& q : queries) {
        tree.search(q, out);
        sink += out.size();
    }
    return ms(chrono::high_resolution_clock::now() - t0).count();
}

/**
 * @brief Замер времени хэш-таблицы
 * @param data Исходный массив (поиск выполняется на месте)
 * @param queries Список запросов (ключей)
 * @return Время в миллисекундах
 */
double measureHash(const HashMap& hm, const vector<string>& queries) {
    volatile int sink = 0;
    vector<const ITService*> out;
    auto t0 = chrono::high_resolution_clock::now();
    for (const auto& q : queries) {
        hm.search(q, out);
        sink += out.size();
    }
    return ms(chrono::high_resolution_clock::now() - t0).count();
}

double measureMultimap(const multimap<string, const ITService*>& mm,
                       const vector<string>& queries) {
    volatile int sink = 0;
    auto t0 = chrono::high_resolution_clock::now();
    for (const auto& q : queries) {
        auto range = mm.equal_range(q);
        for (auto it = range.first; it != range.second; ++it)
            ++sink;
    }
    return ms(chrono::high_resolution_clock::now() - t0).count();
}

/**
 * @brief Точка входа
 * @details Для каждого размера из SIZES выполняет:
 *          - чтение данных,
 *          - построение всех структур,
 *          - замер времени 1000 поисков,
 *          - вывод результатов в консоль и CSV-файл.
 * @return 0 при успехе, 1 при ошибке создания лог-файла
 */
int main() {
    const int SIZES[] = {
        100, 500, 1000, 5000, 10000,
        25000, 50000, 100000, 250000, 500000, 1000000
    };
    const int NUM_SIZES = sizeof(SIZES) / sizeof(SIZES[0]);
    const int NUM_QUERIES = 1000;

    FILE* log = fopen("search_times.csv", "w");
    if (!log) { printf("Не удалось создать search_times.csv\n"); return 1; }
    fprintf(log, "size,linear_ms,bst_ms,rbt_ms,hash_ms,multimap_ms,collisions\n");

    printf("%10s%12s%12s%12s%12s%12s%12s\n",
           "Size", "Linear", "BST", "RBT", "Hash", "Multimap", "Collisions");

    srand(2024);  

    for (int i = 0; i < NUM_SIZES; ++i) {
        int sz = SIZES[i];
        string path = "datasets/services_" + to_string(sz) + ".csv";
        auto data = readCSV(path);

        
        vector<string> queries;
        queries.reserve(NUM_QUERIES);
        for (int q = 0; q < NUM_QUERIES; ++q) {
            int idx = rand() % data.size();
            queries.push_back(data[idx].name);
        }

        
        BSTree bst;
        RBTree rbt;
        HashMap hashmap(data.size() * 2); 
        multimap<string, const ITService*> multimapData;

        for (const auto& s : data) {
            const ITService* ptr = &s;
            bst.insert(s.name, ptr);
            rbt.insert(s.name, ptr);
            hashmap.insert(s.name, ptr);
            multimapData.insert({s.name, ptr});
        }

        
        double tLinear   = measureLinear(data, queries);
        double tBST      = measureBST(bst, queries);
        double tRBT      = measureRBT(rbt, queries);
        double tHash     = measureHash(hashmap, queries);
        double tMultimap = measureMultimap(multimapData, queries);
        size_t collisions = hashmap.countRealCollisions();

        
        printf("%10d%12.1f%12.1f%12.1f%12.1f%12.1f%12zu\n",
               sz, tLinear, tBST, tRBT, tHash, tMultimap, collisions);
        fprintf(log, "%d,%.3f,%.3f,%.3f,%.3f,%.3f,%zu\n",
                sz, tLinear, tBST, tRBT, tHash, tMultimap, collisions);
    }

    fclose(log);
    printf("\nРезультаты сохранены в search_times.csv\n");
    return 0;
}