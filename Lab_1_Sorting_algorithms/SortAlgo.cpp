/**
 * @file SortAlgo.cpp
 * @brief Замеры времени выполнения трёх квадратичных сортировок и std::sort
 * @details Читает данные из CSV, сортирует пузырьком, вставками, шейкером и std::sort,
 *          замеряет время для каждого размера (100...100000), выводит таблицу и сохраняет
 *          отсортированные файлы.
 * @author Kseniia Prokopovich
 * @date 2026-05-13
 */

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <filesystem>

using namespace std;


/**
 * @brief Структура IT-услуги с перегруженными операторами сравнения
 * @details Сравнение выполняется последовательно по полям:
 *          cost -> prepayment -> name.
 */

struct ITService {
    string name;
    double cost;
    int duration;
    double prepayment;

    /**
     * @brief Оператор меньше для сортировки
     * @param o сравниваемый объект
     * @return true если *this < o
     */

    bool operator<(const ITService& o) const {
        if (cost != o.cost) return cost < o.cost;
        if (prepayment != o.prepayment) return prepayment < o.prepayment;
        return name < o.name;
    }
    bool operator>(const ITService& o) const { return o < *this; }
    bool operator<=(const ITService& o) const { return !(o < *this); }
    bool operator>=(const ITService& o) const { return !(*this < o); }
};

/**
 * @brief Чтение CSV-файла с данными услуг
 * @param path путь к входному файлу
 * @return вектор прочитанных записей ITService
 * @details Использует fscanf для прямого чтения в поля без создания промежуточных строк.
 *          Ожидает заголовок в первой строке, который пропускается.
 */

vector<ITService> readCSV(const string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) { printf("Ошибка открытия: %s\n", path.c_str()); exit(1); }

    vector<ITService> data;
    char nameBuf[256];
    ITService s;

    fscanf(f, "%*[^\n]\n");

    while (fscanf(f, "%[^,],%lf,%d,%lf\n", nameBuf, &s.cost, &s.duration, &s.prepayment) == 4) {
        s.name = nameBuf;
        data.push_back(s);
    }
    fclose(f);
    return data;
}

/**
 * @brief Запись отсортированного массива в CSV-файл
 * @param path путь к выходному файлу
 * @param data вектор услуг для записи
 */

void writeCSV(const string& path, const vector<ITService>& data) {
    FILE* f = fopen(path.c_str(), "w");
    if (!f) { printf("Ошибка записи: %s\n", path.c_str()); exit(1); }

    fprintf(f, "name,cost,duration_days,prepayment\n");
    for (const auto& s : data)
        fprintf(f, "%s,%.2f,%d,%.2f\n", s.name.c_str(), s.cost, s.duration, s.prepayment);
    fclose(f);
}

/**
 * @brief Сортировка пузырьком
 * @param a вектор для сортировки (по ссылке)
 */

void bubbleSort(vector<ITService>& a) {
    long n = a.size();
    for (long i = 0; i < n; i++)
        for (long j = n - 1; j > i; j--)
            if (a[j - 1] > a[j])
                swap(a[j - 1], a[j]);
}

/**
 * @brief Сортировка простыми вставками
 * @param a вектор для сортировки (по ссылке)
 */

void insertSort(vector<ITService>& a) {
    long n = a.size();
    for (long i = 1; i < n; i++) {
        ITService x = a[i];
        long j = i - 1;
        for (; j >= 0 && a[j] > x; j--)
            a[j + 1] = a[j];
        a[j + 1] = x;
    }
}

/**
 * @brief Шейкер-сортировка 
 * @param a вектор для сортировки (по ссылке)
 */

void shakerSort(vector<ITService>& a) {
    long lb = 1, ub = a.size() - 1, k = ub;
    do {
        for (long j = ub; j > 0; j--)
            if (a[j - 1] > a[j]) { swap(a[j - 1], a[j]); k = j; }
        lb = k + 1;
        for (long j = 1; j <= ub; j++)
            if (a[j - 1] > a[j]) { swap(a[j - 1], a[j]); k = j; }
        ub = k - 1;
    } while (lb < ub);
}

using ms = chrono::duration<double, milli>;

/**
 * @brief Замер времени выполнения произвольной функции сортировки
 * @param fn указатель на функцию сортировки
 * @param data копия вектора для сортировки
 * @return время выполнения в миллисекундах
 */

double measure(void (*fn)(vector<ITService>&), vector<ITService> data) {
    auto t0 = chrono::high_resolution_clock::now();
    fn(data);
    auto t1 = chrono::high_resolution_clock::now();
    return ms(t1 - t0).count();
}

/**
 * @brief Замер времени выполнения std::sort
 * @param data копия вектора для сортировки
 * @return время выполнения в миллисекундах
 */


double measureStd(vector<ITService> data) {
    auto t0 = chrono::high_resolution_clock::now();
    sort(data.begin(), data.end());
    auto t1 = chrono::high_resolution_clock::now();
    return ms(t1 - t0).count();
}

/**
 * @brief Главная функция – проводит замеры и сохраняет результаты
 * @return 0 при успешном завершении
 */

int main() {
    const int SIZES[] = {
        100, 250, 500, 1000, 2000, 3000, 5000,
        7500, 10000, 15000, 20000, 35000, 50000, 75000, 100000
    };
    const int numSizes = sizeof(SIZES) / sizeof(SIZES[0]);

    filesystem::create_directories("sorted");

    // Создаём файл с замерами
    FILE* log = fopen("timing_results.csv", "w");
    if (log) {
        fprintf(log, "size,bubble_ms,insert_ms,shaker_ms,std_ms\n");
    } else {
        printf("Не удалось создать timing_results.csv\n");
    }

    printf("%10s%14s%14s%14s%14s\n", "Size", "Bubble(ms)", "Insert(ms)", "Shaker(ms)", "Std(ms)");
    for (int i = 0; i < numSizes; i++) {
        int sz = SIZES[i];
        string inPath  = "datasets/services_" + to_string(sz) + ".csv";
        string outPath = "sorted/sorted_"     + to_string(sz) + ".csv";

        auto data = readCSV(inPath);

        double tBubble = measure(bubbleSort, data);
        double tInsert = measure(insertSort, data);
        double tShaker = measure(shakerSort, data);
        double tStd    = measureStd(data);

        // Сохраняем отсортированный файл
        sort(data.begin(), data.end());
        writeCSV(outPath, data);

        printf("%10d%14.1f%14.1f%14.1f%14.1f\n", sz, tBubble, tInsert, tShaker, tStd);

        if (log) {
            fprintf(log, "%d,%.1f,%.1f,%.1f,%.1f\n", sz, tBubble, tInsert, tShaker, tStd);
        }
    }

    if (log) fclose(log);

    printf("\nГотово. Результаты замеров выведены выше и сохранены в timing_results.csv\n");
    return 0;
}